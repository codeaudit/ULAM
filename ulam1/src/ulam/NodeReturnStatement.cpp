#include <stdio.h>
#include "NodeReturnStatement.h"
#include "CompilerState.h"

namespace MFM {

  NodeReturnStatement::NodeReturnStatement(Node * s, CompilerState & state) : Node(state), m_node(s) {}

  NodeReturnStatement::~NodeReturnStatement()
  {
    delete m_node;
    m_node = NULL;
  }

  void NodeReturnStatement::print(File * fp)
  {
    printNodeLocation(fp);  //has same location as it's node
    UTI myut = getNodeType();
    char id[255];
    if(myut == Nav)    
      sprintf(id,"%s<NOTYPE>\n", prettyNodeName().c_str());
    else
      sprintf(id,"%s<%s>\n", prettyNodeName().c_str(), m_state.getUlamTypeNameByIndex(myut).c_str());
    fp->write(id);

    if(m_node) 
      m_node->print(fp);
    else 
      fp->write(" <EMPTYSTMT>\n");

    sprintf(id,"-----------------%s\n", prettyNodeName().c_str());
    fp->write(id);
  }


  void NodeReturnStatement::printPostfix(File * fp)
  {
    assert(m_node);    //e.g. bad decl

    if(m_node)
      m_node->printPostfix(fp);
    else 
      fp->write(" <EMPTYSTMT>");

    fp->write(" ");
    fp->write(getName());
  }


  UTI NodeReturnStatement::checkAndLabelType()
  {
    assert(m_node);

    UTI nodeType = m_node->checkAndLabelType();

    if(nodeType != m_state.m_currentFunctionReturnType)
      {
	m_node = makeCastingNode(m_node, m_state.m_currentFunctionReturnType);
	//m_node = new NodeCast(m_node, m_state.m_currentFunctionReturnType, m_state);
	//m_node->setNodeLocation(getNodeLocation());
	//nodeType = m_node->checkAndLabelType(); //update for return node type
	nodeType = m_node->getNodeType();
      }

    setNodeType(nodeType); //return take type of their node

    m_state.m_currentFunctionReturnNodes.push_back(this); //check later against defined function return type
    return nodeType;
  }


  const char * NodeReturnStatement::getName()
  {
    return "return";
  }


  const std::string NodeReturnStatement::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }


  EvalStatus NodeReturnStatement::eval()
  {
    assert(m_node); 

    evalNodeProlog(0);
    makeRoomForNodeType(getNodeType());
    EvalStatus evs = m_node->eval();
    if(evs != NORMAL)
      {
	assert(evs != CONTINUE && evs != BREAK);
	evalNodeEpilog();
	return evs;
      }

    //end, so copy to -1
    //UlamValue rtnPtr(getNodeType(), 1, true, EVALRETURN);  //positive to current frame pointer
    UlamValue rtnPtr = UlamValue::makePtr(1, EVALRETURN, getNodeType(), m_state.determinePackable(getNodeType()), m_state);  //positive to current frame pointer
    
    assignReturnValueToStack(rtnPtr, STACK); //uses STACK, unlike all the other nodes
    evalNodeEpilog();
    return RETURN;
  }
  
#if 0
  void NodeReturnStatement::GENCODE(File * fp)
  {
    m_state.indent(fp);
    fp->write("return ");
    if(m_node)
      {
	fp->write("(");
	m_node->genCode(fp, uvpass);
	fp->write(")");
      }
    fp->write(";\n");
  }
#endif


  void NodeReturnStatement::genCode(File * fp, UlamValue& uvpass)
  {
    // return for void type has a NodeStatementEmpty m_node
    if(m_node && getNodeType() != Void)
      {
#ifdef TMPVARBRACES
	m_state.indent(fp);
	fp->write("{\n");
	m_state.m_currentIndentLevel++;
#endif
	//m_node->genCodeToStoreInto(fp, uvpass);
	m_node->genCode(fp, uvpass);
	UTI vuti = uvpass.getUlamValueTypeIdx();
	//bool isTerminal = (vuti != Ptr);

	Node::genCodeConvertATmpVarIntoBitVector(fp, uvpass);

	//if(isTerminal)
	//  {
	//    // write out terminal explicitly
	//    m_state.indent(fp);
	//    fp->write("return ");
	//    fp->write("(");
	//    u32 data = uvpass.getImmediateData(m_state);
	//    char dstr[40];
	//    m_state.getUlamTypeByIndex(vuti)->getDataAsString(data, dstr, 'z', m_state);
	//    fp->write(dstr);
	//  }
	//else
	  {
	    m_state.indent(fp);
	    fp->write("return ");
	    fp->write("(");
	    vuti = uvpass.getPtrTargetType();
	    fp->write(m_state.getTmpVarAsString(vuti, uvpass.getPtrSlotIndex(), uvpass.getPtrStorage()).c_str());
	  }

	fp->write(")");
	fp->write(";\n");

#ifdef TMPVARBRACES
	m_state.m_currentIndentLevel--;
	m_state.indent(fp);
	fp->write("}\n");
#endif
      }
    else
      {
	m_state.indent(fp);
	fp->write("return;\n");   //void 
      }
  } //genCode



} //end MFM