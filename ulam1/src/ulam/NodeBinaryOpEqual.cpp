#include "NodeBinaryOpEqual.h"
#include "CompilerState.h"

namespace MFM {

  NodeBinaryOpEqual::NodeBinaryOpEqual(Node * left, Node * right, CompilerState & state) : NodeBinaryOp(left,right,state) {}

  NodeBinaryOpEqual::NodeBinaryOpEqual(const NodeBinaryOpEqual& ref) : NodeBinaryOp(ref) {}

  NodeBinaryOpEqual::~NodeBinaryOpEqual(){}

  Node * NodeBinaryOpEqual::instantiate()
  {
    return new NodeBinaryOpEqual(*this);
  }

  const char * NodeBinaryOpEqual::getName()
  {
    return "=";
  }

  const std::string NodeBinaryOpEqual::prettyNodeName()
  {
    return nodeName(__PRETTY_FUNCTION__);
  }

  const std::string NodeBinaryOpEqual::methodNameForCodeGen()
  {
    return "_Equal_Stub";
  }

  UTI NodeBinaryOpEqual::checkAndLabelType()
  {
    assert(m_nodeLeft && m_nodeRight);

    UTI newType = Nav; //init
    UTI leftType = m_nodeLeft->checkAndLabelType();
    UTI rightType = m_nodeRight->checkAndLabelType();

    if(!m_state.isComplete(leftType) || !m_state.isComplete(rightType))
      {
	setNodeType(Nav);
	return Nav; //not quietly
      }

    if(!m_nodeLeft->isStoreIntoAble())
      {
	std::ostringstream msg;
	msg << "Invalid lefthand side of equals: <" << m_nodeLeft->getName();
	msg << ">, type: " << m_state.getUlamTypeNameByIndex(leftType).c_str();
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	setNodeType(Nav);  //was newType that wasn't Nav
	setStoreIntoAble(false);
	return Nav; //newType
      }

    PACKFIT lpacked = m_state.determinePackable(leftType);
    PACKFIT rpacked = m_state.determinePackable(rightType);
    bool unpackedArrayLeft = !WritePacked(lpacked) && !m_state.isScalar(leftType);
    bool unpackedArrayRight = !WritePacked(rpacked) && !m_state.isScalar(rightType);

    if(unpackedArrayLeft || unpackedArrayRight)
      {
	if(unpackedArrayLeft)
	  {
	    std::ostringstream msg;
	    msg << "Lefthand side of equals requires UNPACKED array support: <";
	    msg << m_nodeLeft->getName();
	    msg << ">, type: " << m_state.getUlamTypeNameByIndex(leftType).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	  }
	if(unpackedArrayRight)
	  {
	    std::ostringstream msg;
	    msg << "Righthand side of equals requires UNPACKED array support: <";
	    msg << m_nodeRight->getName();
	    msg << ">, type: " << m_state.getUlamTypeNameByIndex(rightType).c_str();
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	  }
	setNodeType(Nav);  //was newType that wasn't Nav
	return Nav; //newType
      }

    newType = leftType;

    //cast RHS if necessary
    if(UlamType::compare(newType, rightType, m_state) != UTIC_SAME)
      {
	if(checkForSafeImplicitCasting(leftType, rightType, newType)) //ref
	  {
	    if(!makeCastingNode(m_nodeRight, newType, m_nodeRight))
	      newType = Nav; //error
	  }
      }

    setNodeType(newType);
    setStoreIntoAble(true);
    return newType;
  } //checkAndLabelType

  bool NodeBinaryOpEqual::checkForSafeImplicitCasting(UTI lt, UTI rt, UTI& newType)
  {
    bool rtnOK = true;
    ULAMTYPE ltypEnum = m_state.getUlamTypeByIndex(lt)->getUlamTypeEnum();
    ULAMTYPE rtypEnum = m_state.getUlamTypeByIndex(rt)->getUlamTypeEnum();
    if(!NodeBinaryOp::checkAnyConstantsFit(ltypEnum, rtypEnum, newType))
      rtnOK = false;
    else if(!NodeBinaryOp::checkForMixedSignsOfVariables(ltypEnum, rtypEnum, lt, rt, newType))
      rtnOK = false;
    else if(!checkNonBoolToBoolCast(rtypEnum, rt, newType))
      rtnOK = false;
    else if(!checkFromBitsCast(rtypEnum, rt, newType))
      rtnOK = false;
    else if(!checkToUnaryCast(rtypEnum, rt, newType))
      rtnOK = false;
    else if(!checkBitsizeOfCastLast(rtypEnum, rt, newType))
      rtnOK = false;
    return rtnOK;
  } //checkForSafeImplicitCasting

  bool NodeBinaryOpEqual::checkNonBoolToBoolCast(ULAMTYPE rtypEnum, UTI rt, UTI& newType)
  {
    ULAMTYPE ntypEnum = m_state.getUlamTypeByIndex(newType)->getUlamTypeEnum();
    if(ntypEnum != Bool)
      return true;

    bool rtnOK = false;
    if(!m_nodeRight->isAConstant() && rtypEnum != ntypEnum)
      {
	if(m_state.getBitSize(rt) == 1 && (rtypEnum == Unsigned || rtypEnum == Unary))
	  rtnOK = true;
      }
    else
      rtnOK = true; //bools of any size are safe to cast

    if(!rtnOK)
      {
	std::ostringstream msg;
	msg << "Attempting to implicitly cast a non-Bool type, RHS: ";
	msg << m_state.getUlamTypeNameByIndex(rt).c_str();
	msg << ", to a Bool type: ";
	msg << m_state.getUlamTypeNameByIndex(newType).c_str();
	msg << " for binary operator" << getName() << " without casting";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	newType = Nav;
      }
    return rtnOK;
  } //checkNonBoolToBoolCast

  bool NodeBinaryOpEqual::checkFromBitsCast(ULAMTYPE rtypEnum, UTI rt, UTI& newType)
  {
    bool rtnOK = false;
    if(rtypEnum == Bits)
      {
	if(rt == newType)
	  rtnOK = true;
      }
    else
      rtnOK = true; //not casting From Bits

    if(!rtnOK)
      {
	std::ostringstream msg;
	msg << "Attempting to implicitly cast from a Bits type, RHS: ";
	msg << m_state.getUlamTypeNameByIndex(rt).c_str();
	msg << ", to type: ";
	msg << m_state.getUlamTypeNameByIndex(newType).c_str();
	msg << " for binary operator" << getName() << " without casting";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	newType = Nav;
      }
    return rtnOK;
  } //checkFromBitsCast

  bool NodeBinaryOpEqual::checkToUnaryCast(ULAMTYPE rtypEnum, UTI rt, UTI& newType)
  {
    UlamType * nit = m_state.getUlamTypeByIndex(newType);
    ULAMTYPE ntypEnum = nit->getUlamTypeEnum();
    if(ntypEnum != Unary)
      return true; //not to unary

    bool rtnOK = false;
    if(!m_nodeRight->isAConstant())
      {
	UlamType * rit = m_state.getUlamTypeByIndex(rt);
	if((rit->getMax() <= nit->getMax()) && (rit->getMin() == 0))
	  rtnOK = true;
      }
    else
      rtnOK = true;

    if(!rtnOK)
      {
	std::ostringstream msg;
	msg << "Attempting to implicitly cast from RHS type: ";
	msg << m_state.getUlamTypeNameByIndex(rt).c_str();
	msg << ", to Unary type: ";
	msg << m_state.getUlamTypeNameByIndex(newType).c_str();
	msg << " for binary operator" << getName() << " without casting";
	MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	newType = Nav;
      }
    return rtnOK;
  } //checkToUnaryCast

  bool NodeBinaryOpEqual::checkBitsizeOfCastLast(ULAMTYPE rtypEnum, UTI rt, UTI& newType)
  {
    bool rtnOK = true;
    ULAMTYPE ntypEnum = m_state.getUlamTypeByIndex(newType)->getUlamTypeEnum();
    // constants already checked; Any size Bool to Bool safe.
    // Atom may be larger than an element.
    if(!m_nodeRight->isAConstant() && (ntypEnum != Bool && rtypEnum != Bool) && (rtypEnum != UAtom))
      {
	if(m_state.getBitSize(rt) > m_state.getBitSize(newType))
	  {
	    std::ostringstream msg;
	    msg << "Attempting to implicitly cast from a larger bitsize, RHS type: ";
	    msg << m_state.getUlamTypeNameByIndex(rt).c_str();
	    msg << ", to a smaller bitsize type: ";
	    msg << m_state.getUlamTypeNameByIndex(newType).c_str();
	    msg << " for binary operator" << getName() << " without casting";
	    MSG(getNodeLocationAsString().c_str(), msg.str().c_str(), ERR);
	    newType = Nav;
	    rtnOK = false;
	  }
      }
    return rtnOK;
  } //checkBitsizeOfCastLast

  UTI NodeBinaryOpEqual::calcNodeType(UTI lt, UTI rt)
  {
    assert(0);
    return Nav;
  }

  EvalStatus NodeBinaryOpEqual::eval()
  {
    assert(m_nodeLeft && m_nodeRight);

    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    evalNodeProlog(0); //new current frame pointer on node eval stack

    makeRoomForSlots(1); //always 1 slot for ptr

    EvalStatus evs = m_nodeLeft->evalToStoreInto();
    if(evs != NORMAL)
      {
	evalNodeEpilog();
	return evs;
      }

    u32 slot = makeRoomForNodeType(nuti);
    evs = m_nodeRight->eval(); //a Node Function Call here
    if(evs != NORMAL)
      {
	evalNodeEpilog();
	return evs;
      }

    //assigns rhs to lhs UV pointer (handles arrays);
    //also copy result UV to stack, -1 relative to current frame pointer
    if(slot)
      doBinaryOperation(1, 2, slot);

    evalNodeEpilog();
    return NORMAL;
  } //eval

  EvalStatus NodeBinaryOpEqual::evalToStoreInto()
  {
    UTI nuti = getNodeType();
    if(nuti == Nav)
      return ERROR;

    evalNodeProlog(0);

    makeRoomForSlots(1); //always 1 slot for ptr
    EvalStatus evs = m_nodeLeft->evalToStoreInto();
    if(evs != NORMAL)
      {
	evalNodeEpilog();
	return evs;
      }

    UlamValue luvPtr = UlamValue::makePtr(1, EVALRETURN, nuti, m_state.determinePackable(nuti), m_state); //positive to current frame pointer

    assignReturnValuePtrToStack(luvPtr);

    evalNodeEpilog();
    return NORMAL;
  } //evalToStoreInto

  void NodeBinaryOpEqual::doBinaryOperation(s32 lslot, s32 rslot, u32 slots)
  {
    assert(slots);
    UTI nuti = getNodeType();
    UlamValue pluv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(lslot);
    UlamValue ruv;

    if(m_state.isScalar(nuti))
      {
	ruv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(rslot); //immediate scalar
      }
    else
      {
	PACKFIT packed = m_state.determinePackable(nuti);
	if(WritePacked(packed))
	  {
	    ruv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(rslot); //packed/PL array
	  }
	else
	  {
	    // unpacked array requires a ptr
	    ruv = UlamValue::makePtr(rslot, EVALRETURN, nuti, packed, m_state); //ptr
	  }
      }

    m_state.assignValue(pluv,ruv);

    //also copy result UV to stack, -1 relative to current frame pointer
    assignReturnValueToStack(ruv);
  } //dobinaryoperation

  void NodeBinaryOpEqual::doBinaryOperationImmediate(s32 lslot, s32 rslot, u32 slots)
  {
    assert(slots == 1);
    UTI nuti = getNodeType();
    u32 len = m_state.getTotalBitSize(nuti);

    // 'pluv' is where the resulting sum needs to be stored
    UlamValue pluv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(lslot); //a Ptr
    assert(pluv.getUlamValueTypeIdx() == Ptr && pluv.getPtrTargetType() == nuti);

    assert(slots == 1);
    UlamValue luv = m_state.getPtrTarget(pluv);  //no eval!!
    UlamValue ruv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(rslot); //immediate value
    UlamValue rtnUV;

    u32 wordsize = m_state.getTotalWordSize(nuti);
    if(wordsize == MAXBITSPERINT)
      {
	u32 ldata = luv.getDataFromAtom(pluv, m_state);
	u32 rdata = ruv.getImmediateData(len);
	rtnUV = makeImmediateBinaryOp(nuti, ldata, rdata, len);
      }
    else if(wordsize == MAXBITSPERLONG)
      {
	u64 ldata = luv.getDataLongFromAtom(pluv, m_state);
	u64 rdata = ruv.getImmediateDataLong(len);
	rtnUV = makeImmediateLongBinaryOp(nuti, ldata, rdata, len);
      }
    else
      assert(0);

    m_state.assignValue(pluv,rtnUV);

    //also copy result UV to stack, -1 relative to current frame pointer
    m_state.m_nodeEvalStack.storeUlamValueInSlot(rtnUV, -1);
  } //dobinaryopImmediate

  void NodeBinaryOpEqual::doBinaryOperationArray(s32 lslot, s32 rslot, u32 slots)
  {
    UlamValue rtnUV;
    UTI nuti = getNodeType();
    s32 arraysize = m_state.getArraySize(nuti); //could be zero length array
    s32 bitsize = m_state.getBitSize(nuti);
    UTI scalartypidx = m_state.getUlamTypeAsScalar(nuti);
    PACKFIT packRtn = m_state.determinePackable(nuti);

    if(WritePacked(packRtn))
      {
	//pack result too. (slot size known ahead of time)
	rtnUV = UlamValue::makeAtom(nuti); //accumulate result here
      }

    // 'pluv' is where the resulting sum needs to be stored
    UlamValue pluv = m_state.m_nodeEvalStack.loadUlamValueFromSlot(lslot); //a Ptr
    assert(pluv.getUlamValueTypeIdx() == Ptr && pluv.getPtrTargetType() == nuti);

    // point to base array slots, packedness determines its 'pos'
    UlamValue lArrayPtr = pluv;
    UlamValue rArrayPtr = UlamValue::makePtr(rslot, EVALRETURN, nuti, packRtn, m_state);

    // to use incrementPtr(), 'pos' depends on packedness
    UlamValue lp = UlamValue::makeScalarPtr(lArrayPtr, m_state);
    UlamValue rp = UlamValue::makeScalarPtr(rArrayPtr, m_state);

    //make immediate result for each element inside loop
    for(s32 i = 0; i < arraysize; i++)
      {
	UlamValue luv = m_state.getPtrTarget(lp);
	UlamValue ruv = m_state.getPtrTarget(rp);

	u32 ldata = luv.getData(lp.getPtrPos(), bitsize); //'pos' doesn't vary for unpacked
	u32 rdata = ruv.getData(rp.getPtrPos(), bitsize); //'pos' doesn't vary for unpacked

	if(WritePacked(packRtn))
	  // use calc position where base [0] is furthest from the end.
	  appendBinaryOp(rtnUV, ldata, rdata, (BITSPERATOM-(bitsize * (arraysize - i))), bitsize);
	else
	  {
	    // else, unpacked array
	    rtnUV = makeImmediateBinaryOp(scalartypidx, ldata, rdata, bitsize);

	    // overwrite lhs copy with result UV
	    m_state.assignValue(lp, rtnUV);

	    //copy result UV to stack, -1 (1st array element deepest) relative to current frame pointer
	    m_state.m_nodeEvalStack.storeUlamValueInSlot(rtnUV, -slots + i);
	  }

	assert(lp.incrementPtr(m_state));
	assert(rp.incrementPtr(m_state));
      } //forloop

    if(WritePacked(packRtn))
      {
	m_state.assignValue(pluv, rtnUV); //overwrite lhs copy with result UV
	m_state.m_nodeEvalStack.storeUlamValueInSlot(rtnUV, -1); //store accumulated packed result
      }
  } //dobinaryoparray

  UlamValue NodeBinaryOpEqual::makeImmediateBinaryOp(UTI type, u32 ldata, u32 rdata, u32 len)
  {
    assert(0); //unused
    return UlamValue();
  }

  UlamValue NodeBinaryOpEqual::makeImmediateLongBinaryOp(UTI type, u64 ldata, u64 rdata, u32 len)
  {
    assert(0); //unused
    return UlamValue();
  }

  void NodeBinaryOpEqual::appendBinaryOp(UlamValue& refUV, u32 ldata, u32 rdata, u32 pos, u32 len)
  {
    assert(0); //unused
  }

  void NodeBinaryOpEqual::genCode(File * fp, UlamValue& uvpass)
  {
    assert(m_nodeLeft && m_nodeRight);
    assert(m_state.m_currentObjSymbolsForCodeGen.empty());

#ifdef TMPVARBRACES
    m_state.indent(fp);
    fp->write("{\n");
    m_state.m_currentIndentLevel++;
#endif

    // generate rhs first; may update current object globals (e.g. function call)
    UlamValue ruvpass;
    m_nodeRight->genCode(fp, ruvpass);

    // restore current object globals
    assert(m_state.m_currentObjSymbolsForCodeGen.empty()); //*************

    // lhs should be the new current object: node member select updates them,
    // but a plain NodeIdent does not!!!  because genCodeToStoreInto has been repurposed
    // to mean "don't read into a TmpVar" (e.g. by NodeCast).
    UlamValue luvpass;
    m_nodeLeft->genCodeToStoreInto(fp, luvpass); //may update m_currentObjSymbol, m_currentSelfSymbol

    // current object globals should pertain to lhs for the write
    Node::genCodeWriteFromATmpVar(fp, luvpass, ruvpass); //uses rhs' tmpvar

    uvpass = ruvpass; //in case we're the rhs of an equals..

#ifdef TMPVARBRACES
    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("}\n"); //close for tmpVar
#endif
    assert(m_state.m_currentObjSymbolsForCodeGen.empty());
  } //genCode

} //end MFM
