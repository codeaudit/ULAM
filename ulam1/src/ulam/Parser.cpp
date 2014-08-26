#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <assert.h>
#include "Parser.h"
#include "NodeBinaryOpSubtract.h"
#include "NodeBinaryOpAdd.h"
#include "NodeBinaryOpMultiply.h"
#include "NodeBinaryOpDivide.h"
#include "NodeBinaryOpBitwiseAnd.h"
#include "NodeBinaryOpBitwiseOr.h"
#include "NodeBinaryOpBitwiseXor.h"
#include "NodeBinaryOpEqual.h"
#include "NodeBinaryOpPlusEqual.h"
#include "NodeBinaryOpMinusEqual.h"
#include "NodeBinaryOpMultiplyEqual.h"
#include "NodeBinaryOpBitwiseAndEqual.h"
#include "NodeBinaryOpBitwiseOrEqual.h"
#include "NodeBinaryOpBitwiseXorEqual.h"
#include "NodeBinaryOpSquareBracket.h"
#include "NodeBlock.h"
#include "NodeBlockEmpty.h"
#include "NodeBlockFunctionDefinition.h"
#include "NodeCast.h"
#include "NodeControlIf.h"
#include "NodeControlWhile.h"
#include "NodeProgram.h"
#include "NodeReturnStatement.h"
#include "NodeTerminal.h"
#include "NodeTerminalIdent.h"
#include "NodeTypedef.h"
#include "NodeSimpleStatement.h"
#include "NodeStatementEmpty.h"
#include "NodeUnaryOpMinus.h"
#include "NodeUnaryOpPlus.h"
#include "NodeUnaryOpBang.h"
#include "NodeVarDecl.h"
#include "NodeVarDeclList.h"
#include "SymbolFunction.h"
#include "SymbolFunctionName.h"
#include "SymbolVariable.h"
#include "SymbolClass.h"


namespace MFM {

#define QUIETLY true

  Parser::Parser(Tokenizer * izer, CompilerState & state): m_state(state), m_tokenizer(izer)
  {
    // need a block for ST of UlamType names; do this in classblock
    //    init();
  }

  Parser::~Parser()
  {}
 

  // change to return Node * rather than vector; change tests
  Node * Parser::parseProgram(std::string startstr, File * errOutput)
  {
    if(errOutput)
      m_state.m_err.setFileOutput(errOutput);

    // add name of the thing we are compiling to string pool (and node program);
    // dropping the .ulam suffix
    u32 foundSuffix = startstr.find(".ulam");
    if(foundSuffix == std::string::npos)
      {
	std::ostringstream msg;
	msg << "File name <" << startstr << "> doesn't have a valid suffix (.ulam)";
	MSG("",msg.str().c_str() , ERR);
	return NULL;
      }

    std::string compileThis = startstr.substr(0,foundSuffix);
    char c = compileThis.at(0);
    if(!Token::isUpper(c))
      {
	//compileThis.at(0) = 'A' + (c - 'a');  //uppercase
	std::ostringstream msg;
	msg << "File name <" << startstr << "> must match a valid class name (uppercase) to compile";
	MSG("", msg.str().c_str() , ERR);
	return  NULL;
      }

    u32 compileThisId  = m_state.m_pool.getIndexForDataString(compileThis);
    m_state.m_compileThisId = compileThisId;  // for everyone

    initPrimitiveUlamTypes();

    // here's the start (first token)!!  preparser will handle the VERSION_DECL, 
    // as well as USE and LOAD keywords. so the first thing we see may not be 
    // "our guy"..but we'll know.

    do{
      m_state.m_currentBlock = NULL;  //reset for each new class block
    } while(!parseThisClass());

    // here when THIS class is done, or there was an error
    NodeProgram * P = NULL;
    Symbol * thisClassSymbol = NULL;

    if(m_state.m_programDefST.isInTable(compileThisId, thisClassSymbol))
      {
	NodeBlockClass * rootNode = ((SymbolClass *) thisClassSymbol)->getClassBlockNode();	
	if(rootNode)
	  {
	    P = new NodeProgram(compileThisId, m_state);  
	    P->setRootNode(rootNode);
	    P->setNodeLocation(rootNode->getNodeLocation());  
	    assert(m_state.m_classBlock == rootNode);
	  }
      }
    

    if(!P)
      { 
	MSG("", "No parse tree", ERR);
      }

    u32 warns = m_state.m_err.getWarningCount();
    if(warns > 0)
      {
	std::ostringstream msg;
	msg << warns << " warnings during parsing";
	MSG((P ? P->getNodeLocationAsString().c_str() : ""), msg.str().c_str(), INFO);
      }

    u32 errs = m_state.m_err.getErrorCount();
    if(errs > 0)
      {
	std::ostringstream msg;
	msg << errs << " TOO MANY PARSE ERRORS";
	MSG((P ? P->getNodeLocationAsString().c_str() : ""), msg.str().c_str(), INFO);
      }

    return (P);  //ownership transferred to caller
  }

    
  bool Parser::parseThisClass()
  {
    Token pTok;
    getNextToken(pTok);
    
    if( (pTok.m_type != TOK_KW_ELEMENT) && (pTok.m_type != TOK_KW_QUARK) )
      {
	//error!
	return true;  //we're done.
      }

    //each class has its own parse tree; only compileThis has code generated later.
    Token iTok;
    getNextToken(iTok);

    //insure the class name starts with a capital letter, and is not a primitive (e.g. TOK_TYPE_INT)
    //if(!(iTok.m_type == TOK_IDENTIFIER && Token::isTokenAType(iTok,&m_state)))
    if(iTok.m_type != TOK_TYPE_IDENTIFIER)
      {
	//error! 
	return  true; //we're done unless we can gobble the rest up? 
      }


    SymbolClass * cSym = NULL;
    if(!m_state.alreadyDefinedSymbolClass(iTok.m_dataindex, cSym))
      {
	m_state.addIncompleteClassSymbolToProgramTable(iTok.m_dataindex, cSym);
      }
    else
      {
	//if already defined, then must be incomplete, else duplicate!!
	if(cSym->getUlamClassType() != UC_INCOMPLETE)
	  {
	    //error!! duplicate
	    
	    return  true;  //we're done unless we can gobble the rest up? 
	  }
      }


    //set class type in UlamType (through its class symbol) since we know it
    switch(pTok.m_type)
      {
      case TOK_KW_ELEMENT:
	{
	  cSym->setUlamClassType(UC_ELEMENT);
	  break;
	}
      case TOK_KW_QUARK:
	{
	  cSym->setUlamClassType(UC_QUARK);
	  break;
	}
      default:
	//error!!
	assert(0);
      };


    NodeBlockClass * classNode = parseClassBlock();

    if(classNode)
      {
	cSym->setClassBlockNode(classNode);
      }
    else
      {
	//error! reset to incomplete
	cSym->setUlamClassType(UC_INCOMPLETE);
      }

    //return true when we've seen THIS class
    return (iTok.m_dataindex == m_state.m_compileThisId); //parseThisClass
  }


  NodeBlockClass * Parser::parseClassBlock()
  {
    Token pTok;
    NodeBlockClass * rtnNode = NULL;

    if(!getExpectedToken(TOK_OPEN_CURLY, pTok))
      return NULL;


    if(getExpectedToken(TOK_CLOSE_CURLY))
      {
	return NULL;
      }

    NodeBlock * prevBlock = m_state.m_currentBlock;
    assert(prevBlock == NULL); //this is the class' first block

    rtnNode = new NodeBlockClass(prevBlock, m_state);
    rtnNode->setNodeLocation(pTok.m_locator); 

    // current, this block's symbol table added to parse tree stack
    //          for validating and finding scope of program/block variables

    m_state.m_currentBlock = rtnNode;
    m_state.m_classBlock = rtnNode;    //2 ST:functions and data member decls, separate

    //keep the data member var decls, starting with NodeBlockClass, keeping it for return
    NodeStatements * nextNode = rtnNode;
    NodeStatements * stmtNode = NULL;

    //initPrimitiveUlamTypes();

    while( parseDataMember(stmtNode))   //could be false, in case of function def
      {
	if(stmtNode)
	  {
	    nextNode->setNextNode(stmtNode);
	    nextNode = stmtNode;
	    stmtNode = NULL;
	  }
      }

    if(!getExpectedToken(TOK_CLOSE_CURLY))
      {
	    delete rtnNode;
	    rtnNode = NULL;
      }

    //this block's ST is no longer in scope
    m_state.m_currentBlock = prevBlock;

    return rtnNode;  //parseBlockClass
  }


  bool Parser::parseDataMember(NodeStatements *& nextNode)
  {
    bool brtn = false;
    Node * rtnNode = NULL;
    Token pTok, iTok;
    
    getNextToken(pTok);

    //parse Typedef's starting with keyword first
    if(pTok.m_type == TOK_KW_TYPEDEF)
      {
	if((rtnNode = parseTypedef()) )
	  {
	    if(!getExpectedToken(TOK_SEMICOLON))
	      {
		delete rtnNode;
		rtnNode = NULL;
		getTokensUntil(TOK_SEMICOLON);  //does this help?
	      }
	    else
	      {
		brtn = true;  
		nextNode = new NodeStatements(rtnNode, m_state);
		nextNode->setNodeLocation(rtnNode->getNodeLocation());
	      }
	  }
	  return brtn;
      }

    
    if(! Token::isTokenAType(pTok))
      {
	unreadToken();
	return false;
      }

    
    if(getExpectedToken(TOK_IDENTIFIER, iTok))
      {
	//need another token to distinguish a function from a variable declaration (do so quietly)
	if(getExpectedToken(TOK_OPEN_PAREN, QUIETLY))
	  { 
	    //eats the '(' when found
	    rtnNode = makeFunctionSymbol(pTok, iTok); //with params
	    if(!getExpectedToken(TOK_CLOSE_CURLY))
	      {
		//first remove the pointer to this node from its symbol
		if(rtnNode)
		  ((NodeBlockFunctionDefinition *) rtnNode)->getFuncSymbolPtr()->setFunctionNode((NodeBlockFunctionDefinition *) NULL); //deletes node
		rtnNode = NULL;
		MSG(&pTok, "INCOMPLETE Function Definition", ERR);
	      }
	    else
	      {
		if(rtnNode)
		  brtn = true;  //rtnNode belongs to the symbolFunction
	      }
	  }
	else
	  {
	    // not '(', so token is unread, and we know
	    // it's a variable, not a function;
	    // also handles arrays
	    rtnNode = makeVariableSymbol(pTok, iTok);
	    
	    if(rtnNode)
	      rtnNode = parseRestOfDecls(pTok, rtnNode);  
	    
	    if(!getExpectedToken(TOK_SEMICOLON))
	      {
		delete rtnNode;
		rtnNode = NULL;
		getTokensUntil(TOK_SEMICOLON); //does this help?
	      }
	    else
	      {
		if(rtnNode)
		  {
		    brtn = true;  
		    nextNode = new NodeStatements(rtnNode, m_state);
		    nextNode->setNodeLocation(rtnNode->getNodeLocation());
		  }
	      }
	  }
      }
    //else error!
    return brtn;  //parseDataMember
  }


  Node * Parser::parseBlock()
  {
    Token pTok;
    NodeBlock * rtnNode = NULL;

    if(!getExpectedToken(TOK_OPEN_CURLY, pTok))
      return NULL;


    if(getExpectedToken(TOK_CLOSE_CURLY, QUIETLY))
      {
	rtnNode = new NodeBlockEmpty(m_state.m_currentBlock, m_state); // legal
	rtnNode->setNodeLocation(pTok.m_locator);
	return rtnNode;
      }
      
    rtnNode = new NodeBlock(m_state.m_currentBlock, m_state);
    rtnNode->setNodeLocation(pTok.m_locator); 

    // current, this block's symbol table added to parse tree stack
    //          for validating and finding scope of program/block variables
    NodeBlock * prevBlock = m_state.m_currentBlock;
    m_state.m_currentBlock = rtnNode;

    NodeStatements * nextNode = (NodeStatements *) parseStatements();

    if(nextNode)  //could be Null, in case of errors
      rtnNode->setNextNode(nextNode);
    else
      {
	//replace NodeBlock with a NodeBlockEmpty
	delete rtnNode;
	rtnNode = NULL;

	rtnNode = new NodeBlockEmpty(prevBlock, m_state);  	// legal
	rtnNode->setNodeLocation(pTok.m_locator);

	m_state.m_currentBlock = rtnNode; //very temporary
      }

    if(!getExpectedToken(TOK_CLOSE_CURLY))
      {
	delete rtnNode;
	rtnNode = NULL;
	getTokensUntil(TOK_CLOSE_CURLY);
	m_state.m_currentBlock = NULL;   //very temporary
      }

    //this block's ST is no longer in scope
    if(m_state.m_currentBlock)
      m_state.m_currentFunctionBlockDeclSize -= m_state.m_currentBlock->getSizeOfSymbolsInTable();

    m_state.m_currentBlock = prevBlock;

    //sanity check 
    assert(!rtnNode || rtnNode->getPreviousBlockPointer() == prevBlock);

    return rtnNode;  //parseBlock
  }


  Node * Parser::parseStatements()
  {
    Node * sNode = (NodeStatements *) parseStatement();

    if(!sNode)  //e.g. due to an invalid token perhaps
      {
	MSG("", "EMPTY STATEMENT!! when in doubt, count", WARN);

	Token pTok;
	getNextToken(pTok);
	if(pTok.m_type != TOK_CLOSE_CURLY && pTok.m_type != TOK_EOF)
	  {
	    unreadToken();
	    return parseStatements();  //try again
	  }
	else
	  {
	    unreadToken();
	    return NULL; 
	  }
      }


    NodeStatements * rtnNode = new NodeStatements(sNode, m_state);  
    rtnNode->setNodeLocation(sNode->getNodeLocation());

    if(!getExpectedToken(TOK_CLOSE_CURLY, QUIETLY))
      {
	rtnNode->setNextNode((NodeStatements *) parseStatements()); // more statements
      }
    else
      unreadToken();

    return rtnNode; //parseStatements
  }


  Node * Parser::parseStatement()
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    if(pTok.m_type == TOK_OPEN_CURLY)
      {
	unreadToken();
	rtnNode = parseBlock();
      }
    else if(Token::getSpecialTokenWork(pTok.m_type) == TOKSP_CTLKEYWORD)
      {
	unreadToken();
	rtnNode = parseControlStatement(); //last token either a } or ;
      }
    else
      {
	unreadToken();
	rtnNode = parseSimpleStatement(); // may be null (only ;)
      }

    return rtnNode;   //parseStatement
  }


  Node * Parser::parseControlStatement()
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    // should have already known to be true
    assert(Token::getSpecialTokenWork(pTok.m_type) == TOKSP_CTLKEYWORD);
    
    switch(pTok.m_type)
      {
      case TOK_KW_IF:
	rtnNode = parseControlIf(pTok);
	break;
      case TOK_KW_WHILE:
	rtnNode = parseControlWhile(pTok);
	break;
      case TOK_ERROR_CONT:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(), ERR);
	  //eat error token
	}
	break;
      default:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(), ERR);
	  //unreadToken(); leads to infinite loop
	}
	break;
      };
    
    return rtnNode;
  }


  Node * Parser::parseControlIf(Token ifTok)
  {
    
    if(!getExpectedToken(TOK_OPEN_PAREN))  
      {
	return NULL;
      }
    
    Node * condNode = parseAssignExpr(); 
    if(!condNode) 
      return NULL;  //stop this maddness
    
    if(!getExpectedToken(TOK_CLOSE_PAREN))  
      {
	delete condNode;
	return NULL;
      }

    Node * trueNode = parseStatement();
    if(!trueNode)
      {
	delete condNode;
	return NULL;  //stop this maddness
      }

    Node * falseNode = NULL;

    if(getExpectedToken(TOK_KW_ELSE, QUIETLY))
      {
	falseNode = parseStatement();
      }

    Node * rtnNode = new NodeControlIf(condNode, trueNode, falseNode, m_state);
    rtnNode->setNodeLocation(ifTok.m_locator);

    return rtnNode;
  }


  Node * Parser::parseControlWhile(Token wTok)
  {
    
    if(!getExpectedToken(TOK_OPEN_PAREN))  
      {
	return NULL;
      }
    
    Node * condNode = parseAssignExpr(); 
    if(!condNode) 
      return NULL;  //stop this maddness

    if(!getExpectedToken(TOK_CLOSE_PAREN))  
      {
	delete condNode;
	return NULL;
      }

    Node * trueNode = parseStatement();
    if(!trueNode)
      {
	delete condNode;
	return NULL;  //stop this maddness
      }

    Node * rtnNode = new NodeControlWhile(condNode, trueNode, m_state);
    rtnNode->setNodeLocation(wTok.m_locator);

    return rtnNode;
  }


  Node * Parser::parseSimpleStatement()
  {
    Node * rtnNode = NULL;
    Token pTok;
    
    getNextToken(pTok);

    if(pTok.m_type == TOK_SEMICOLON)
      {
	unreadToken();
	rtnNode = new NodeStatementEmpty(m_state);  	// empty statement
	rtnNode->setNodeLocation(pTok.m_locator);
      }
    else if(Token::isTokenAType(pTok))
      {
	unreadToken();
	rtnNode = parseDecl();        //updates symbol table	
      }
    else if(pTok.m_type == TOK_KW_TYPEDEF)
      {
	rtnNode = parseTypedef();
      }
    else if(pTok.m_type == TOK_KW_RETURN)
      {
	unreadToken();               // needs location
	rtnNode = parseReturn();
      }
    else if(pTok.m_type == TOK_ERROR_CONT)
      {
	MSG(&pTok, "Unexpected input!! ERROR Token, Continue",ERR);
	//eat error token
      }
    else if(pTok.m_type == TOK_ERROR_ABORT)
      {
	MSG(&pTok, "Unexpected input!! ERROR Token, Exiting..", ERR);
	exit(1);
      }
    else 
      {
	unreadToken();
	Node * expNode = parseAssignExpr();
	if(expNode)
	  {
	    rtnNode = new NodeSimpleStatement(expNode,m_state); 
	    rtnNode->setNodeLocation(expNode->getNodeLocation());
	  }
      }


    if(!getExpectedToken(TOK_SEMICOLON))
      {
	MSG(&pTok, "Invalid Statement", ERR);
	delete rtnNode;
	rtnNode = NULL;
	getTokensUntil(TOK_SEMICOLON);
      }
    
    return rtnNode;   //parseSimpleStatement
  }


  // Typedefs are not transferred to generated code;
  // they are a short-hand for ulamtypes (e.g. arrays)
  // that may be used as function return types; scope-specific.
  Node * Parser::parseTypedef()
  {
    Node * rtnNode = NULL;
    Token pTok;
    getNextToken(pTok);
      
    if(Token::isTokenAType(pTok))
      {
	Token iTok;
	getNextToken(iTok);
	//insure the typedef name starts with a capital letter
	//if(iTok.m_type == TOK_IDENTIFIER && Token::isTokenAType(iTok,&m_state))
	if(iTok.m_type == TOK_TYPE_IDENTIFIER)
	  {
	    rtnNode = makeTypedefSymbol(pTok, iTok);
	  }
	else
	  {
	    std::ostringstream msg;
	    msg << "Invalid typedef Identifier <" << m_state.m_pool.getDataAsString(iTok.m_dataindex).c_str() << ">, requires capitalization";
	    MSG(&iTok, msg.str().c_str(), ERR);
	  }
      }
    else
      {
	std::ostringstream msg;
	msg << "Invalid typedef Base Type <" << m_state.m_pool.getDataAsString(pTok.m_dataindex).c_str() << ">";
	MSG(&pTok, msg.str().c_str(), ERR);
      }

    return rtnNode;
  }


  // used for data members or local function variables; or 
  // 'singledecl' function parameters; no longer for function defs.
  Node * Parser::parseDecl(bool parseSingleDecl)
  {
    Node * rtnNode = NULL;
    Token pTok, iTok;

    getNextToken(pTok);

    // should have already known to be true
    assert(Token::isTokenAType(pTok));

    if(getExpectedToken(TOK_IDENTIFIER, iTok))
      {
	rtnNode = makeVariableSymbol(pTok, iTok);

	if(rtnNode && !parseSingleDecl) 
	  {
	    // for multi's of same type
	    return parseRestOfDecls(pTok, rtnNode);  
	  }
      }
    
    return rtnNode;  //parseDecl
  }


  Node * Parser::parseReturn()
  {
    Token pTok;    
    getNextToken(pTok);

    Node * rtnNode = NULL;
    Node * rtnExprNode = parseAssignExpr(); // may be NULL
    if(!rtnExprNode)
      {
	rtnExprNode = new NodeStatementEmpty(m_state); //has Nav type
	rtnExprNode->setNodeLocation(pTok.m_locator);
      }

    rtnNode =  new NodeReturnStatement(rtnExprNode, m_state);
    rtnNode->setNodeLocation(pTok.m_locator);

    return rtnNode;
  }

  
  Node * Parser::parseAssignExpr()
  {
    Node * rtnNode = NULL;

    Token iTok;
    if(getExpectedToken(TOK_IDENTIFIER, iTok, QUIETLY))
      {
	// though function calls are not proper lhs values in assign
	// expression; they are parsed here (due to the two token look
	// ahead, which drops the Identifier Token before parseExpression) and is
	// caught during checkAndLabelType as NOT storeIntoAble.
	if(!(rtnNode = parseIdentExpr(iTok)))
	  return parseExpression(); 
      }
    else
      return parseExpression();   //perhaps a number, true or false

    // if nothing else follows, parseRestOfAssignExpr returns its argument
    return parseRestOfAssignExpr(rtnNode);  //parseAssignExpr
  }


  Node * Parser::parseLvalExpr(Token identTok)
  {
    Token pTok;
    getNextToken(pTok);
    // function calls or func defs are not valid
    if(pTok.m_type == TOK_OPEN_PAREN)
      {
	std::ostringstream msg;
	msg << "Unexpected token <" << pTok.getTokenEnumName() << "> indicates a function call or definition; neither are valid here";
	MSG(&pTok, msg.str().c_str(), ERR);

	unreadToken();
	return NULL;
      }

    unreadToken();  //put whatever back

    Symbol * asymptr = NULL;
    
    //may continue when symbol not defined yet (e.g. Decl)
    m_state.alreadyDefinedSymbol(identTok.m_dataindex,asymptr);

    // make a variable;  symbol could be Null!
    Node * rtnNode = new NodeTerminalIdent(identTok, (SymbolVariable *) asymptr, m_state); 
    assert(rtnNode);
    rtnNode->setNodeLocation(identTok.m_locator);
    
    return parseRestOfLvalExpr(rtnNode);  //in case of arrays   
  }


  // lvalExpr + func Calls
  Node * Parser::parseIdentExpr(Token identTok)
  {
    //function call, otherwise lvalExpr
    if(getExpectedToken(TOK_OPEN_PAREN, QUIETLY))
      {
	Symbol * asymptr = NULL;

	//may continue when symbol not defined yet (e.g. FuncCall)
	m_state.alreadyDefinedSymbol(identTok.m_dataindex,asymptr);
	if(asymptr && !asymptr->isFunction())
	  {
	    std::ostringstream msg;
	    msg << "Undefined function <" << m_state.getDataAsString(&identTok).c_str() << "> that has already been declared as a variable";
	    MSG(&identTok, msg.str().c_str(), ERR);
	    return  NULL;
	  }
	
	//function call
	return parseFunctionCall(identTok);
      }

    // if here, we have a variable, not a function call
    return parseLvalExpr(identTok);  //parseIdentExpr
  }


  Node * Parser::parseFunctionCall(Token identTok)
  {
    Symbol * asymptr = NULL;

    // cannot call a function if a local variable name shadows it
    if(m_state.m_currentBlock->isIdInScope(identTok.m_dataindex,asymptr))
      {
	std::ostringstream msg;
	msg << "'" << m_state.m_pool.getDataAsString(asymptr->getId()).c_str() << "' cannot be used as a function, already declared as a variable '" << asymptr->getUlamType()->getUlamTypeNameBrief(&m_state).c_str() << " " << m_state.m_pool.getDataAsString(asymptr->getId()) << "'";
	MSG(&identTok, msg.str().c_str(), ERR);
	return NULL;
      }

    //fill in func symbol during type labeling; supports function overloading
    NodeFunctionCall * rtnNode = new NodeFunctionCall(identTok, NULL, m_state); 
    rtnNode->setNodeLocation(identTok.m_locator);

    if(!parseRestOfFunctionCallArguments(rtnNode))
      {
	delete rtnNode;
	return NULL;
      }

    // can't do any checking since function may not have been seen yet
    return rtnNode;
  }


  bool Parser::parseRestOfFunctionCallArguments(NodeFunctionCall * callNode)
  {
    Node * nextArg = parseAssignExpr();
    if(nextArg)
      callNode->addArgument(nextArg);

    //if close paren we're done, o.w. comma, go again...(eat the token)
    Token pTok;
    getNextToken(pTok);
    if(pTok.m_type == TOK_CLOSE_PAREN)
      return true;

    if(pTok.m_type == TOK_COMMA)      
      return parseRestOfFunctionCallArguments(callNode);
    
    std::ostringstream msg;
    msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
    MSG(&pTok, msg.str().c_str(),ERR);
    return false;
  }


  Node * Parser::parseExpression()
  {
    Node * rtnNode = parseTerm(); 

    if(!rtnNode) 
      return NULL;  //stop this maddness

    // if not addop, parseRestOfExpression returns its arg
    return parseRestOfExpression(rtnNode);  //parseExpression
  }
  
  
  Node * Parser::parseTerm()
  {
    Node * rtnNode = parseFactor();

    if(!rtnNode) 
      return NULL;  //stop this maddness!

    // if not mulop, parseRestOfTerm returns rtnNode
    return parseRestOfTerm(rtnNode); //parseTerm
  }

    
  Node * Parser::parseFactor()
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    switch(pTok.m_type)
      {
      case TOK_IDENTIFIER:
	//unreadToken();
	rtnNode = parseIdentExpr(pTok);
	break;
      case TOK_NUMBER:
      case TOK_KW_TRUE:	
      case TOK_KW_FALSE:
	rtnNode = new NodeTerminal(pTok, m_state);
	rtnNode->setNodeLocation(pTok.m_locator);
	break;
      case TOK_OPEN_PAREN:
	{
	  //if next token is a type this a user cast, o.w. expression
	  Token tTok;
	  getNextToken(tTok);
	  if(Token::isTokenAType(tTok))
	    {
	      rtnNode = makeCastNode(tTok); 
	    }	      
	  else
	    {
	      unreadToken();
	      rtnNode = parseExpression();
	      if(!getExpectedToken(TOK_CLOSE_PAREN)) 
		{
		  delete rtnNode;
		  rtnNode = NULL;
		}
	    }
	}
	break;
      case TOK_MINUS:
      case TOK_PLUS:
      case TOK_BANG:
	unreadToken();
	rtnNode = parseRestOfFactor();  //parseUnop
	break;
      case TOK_EOF:
      case TOK_CLOSE_CURLY:
      case TOK_SEMICOLON:
      case TOK_CLOSE_PAREN:
      case TOK_COMMA:  // for functionall args
	unreadToken();
	break;
      case TOK_ERROR_CONT:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(),ERR);
	  return parseFactor(); //redo
	}
	break;
      default:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(),ERR);
	  //unreadToken(); eat the error token
	}
      };
    
    return rtnNode;  //parseFactor
  }


  Node * Parser::parseRestOfFactor()
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    switch(pTok.m_type)
      {
      case TOK_MINUS:
      case TOK_PLUS:
      case TOK_BANG:
	unreadToken();
	rtnNode = makeFactorNode();
	break;
      case TOK_ERROR_CONT:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(), ERR);
	  //eat token
	}
	break;  
      case TOK_ERROR_ABORT:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">, exiting..";
	  MSG(&pTok, msg.str().c_str(), ERR);
	  //eat token
	  exit(1);
	}
	break;  
      default:
	{
	  unreadToken();
	}
      };
    
    return rtnNode;  //parseRestOfFactor
  }


  Node * Parser::makeCastNode(Token typeTok)
  {
    Node * rtnNode = NULL;
    std::string typeName = m_state.getTokenAsATypeName(typeTok); //either primitive or typedef
    UlamType * typeToBe;

    if(!m_state.getUlamTypeByTypedefName(typeName.c_str(), typeToBe))
      {
	assert(Token::getSpecialTokenWork(typeTok.m_type) == TOKSP_TYPEKEYWORD);
	ULAMTYPE UT = UlamType::getEnumFromUlamTypeString(typeName.c_str());
	typeToBe = m_state.getUlamTypeByIndex(UT);
      }

    if(getExpectedToken(TOK_CLOSE_PAREN)) 
      {
	rtnNode = new NodeCast(parseFactor(),typeToBe,m_state);
	rtnNode->setNodeLocation(typeTok.m_locator);
      }

    return rtnNode;
  }


  Node * Parser::parseRestOfTerm(Node * leftNode)
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);
    
    switch(pTok.m_type)
      {
      case TOK_STAR:
      case TOK_SLASH:
	{
	  unreadToken();
	  rtnNode = makeTermNode(leftNode);
	  rtnNode = parseRestOfTerm(rtnNode);
	  break;
	}
      default:
	{
	  unreadToken();
	  rtnNode = leftNode;
	}
      };
    
    return rtnNode;  //parseRestOfTerm
  }


  Node * Parser::parseRestOfLvalExpr(Node * leftNode)
  {
    Node * rtnNode = NULL;
    Token pTok;

    if(!getExpectedToken(TOK_OPEN_SQUARE, pTok, QUIETLY))
      return leftNode;

    Node * rightNode = parseExpression();
    if(!rightNode)
      {
	MSG(&pTok, "RHS of Open Square is missing->Sq Bracket deleted", ERR);
	delete leftNode;
	rtnNode = NULL;  
      }
    else
      {
	rtnNode = new NodeBinaryOpSquareBracket(leftNode, rightNode, m_state);
	rtnNode->setNodeLocation(pTok.m_locator);
      }
    

    if(!getExpectedToken(TOK_CLOSE_SQUARE))
      {
	delete rtnNode;
	rtnNode = NULL;
      }

    return rtnNode;  //parseRestOfLValExpr
  }


  Node * Parser::parseRestOfDecls(Token typeTok, Node * dNode)
  { 

    if(!getExpectedToken(TOK_COMMA, QUIETLY))
      {
	return dNode;
      }

    Node * rtnNode = dNode;
    Token iTok;
    if(getExpectedToken(TOK_IDENTIFIER, iTok))
      {
	// another decl of same type typeTok
	Node * sNode = makeVariableSymbol(typeTok, iTok);  //a decl
	if (sNode)
	  {
	    rtnNode =  new NodeVarDeclList(dNode, sNode, m_state) ;
	    rtnNode->setNodeLocation(typeTok.m_locator);
	  }
      }
    else 
      {
	//perhaps read until semi-colon
	getTokensUntil(TOK_SEMICOLON);
	unreadToken();
      }

    return parseRestOfDecls(typeTok, rtnNode);
  }
    

  Node * Parser::makeVariableSymbol(Token typeTok, Token identTok)
  {
    assert(! Token::isTokenAType(identTok)); 
#if 0
    // new! done by Lexer
    // first check that the function name begins with a lower case letter
    if(Token::isTokenAType(identTok))
      {	
	std::ostringstream msg;
	msg << "Variable <" << m_state.getDataAsString(&identTok).c_str() << "> is not a valid (lower case) name";
	MSG(&identTok, msg.str().c_str(), ERR);
	return NULL;
      }
#endif

    NodeVarDecl * rtnNode = NULL;
    Node * lvalNode = parseLvalExpr(identTok);
 
    if(lvalNode)
      {
	// lvalNode could be either a NodeTerminalIdent or a NodeBinaryOpSquareBracket
	// process identifier...check if already defined in current scope; if not, add it;
	// returned symbol could be symbolVariable or symbolFunction, detect first.
	Symbol * asymptr = NULL;


	UlamType * ut;
	u32 arraysize = 0;
	if(m_state.getUlamTypeByTypedefName(m_state.getTokenAsATypeName(typeTok).c_str(), ut))
	  arraysize = ut->getArraySize(); //typedef built-in arraysize, no []

	if(!lvalNode->installSymbolVariable(typeTok, arraysize, asymptr))
	  {
	    if(asymptr)
	      {
		std::ostringstream msg;
		msg << m_state.m_pool.getDataAsString(asymptr->getId()).c_str() << " has a previous declaration as '" << asymptr->getUlamType()->getUlamTypeNameBrief(&m_state).c_str() << " " << m_state.m_pool.getDataAsString(asymptr->getId()) << "'";
		MSG(&typeTok, msg.str().c_str(), ERR);
	      }
	    else 
	      {
		// installSymbol failed for other reasons (e.g. problem with []) 
		// rtnNode is NULL;
		std::ostringstream msg;
		msg << "Invalid variable declaration of Type: <" << m_state.getTokenAsATypeName(typeTok).c_str() << "> and Name: <" << m_state.getDataAsString(&identTok).c_str() << "> (missing symbol)";
		MSG(&typeTok, msg.str().c_str(), ERR);
	      }
	  }
	else
	  {
	    rtnNode =  new NodeVarDecl((SymbolVariable *) asymptr, m_state);
	    rtnNode->setNodeLocation(typeTok.m_locator);
	  }
	
	delete lvalNode;  //done with it
      }

    return rtnNode;  //makeVariableSymbol
  } 


  Node * Parser::makeFunctionSymbol(Token typeTok, Token identTok)
  {
    // first check that the function name begins with a lower case letter
    if(Token::isTokenAType(identTok))
      {	
	std::ostringstream msg;
	msg << "Function <" << m_state.getDataAsString(&identTok).c_str() << "> is not a valid (lower case) name";
	MSG(&identTok, msg.str().c_str(), ERR);

	// eat tokens until end of definition ???
	return NULL;
      }

    Symbol * asymptr = NULL;
    // ask current scope class block if this identifier name is there 
    // (checks functions and variables and typedefs); 
    // if not a function, BAIL; check for overloaded function, after parameter types available
    if(m_state.m_classBlock->isIdInScope(identTok.m_dataindex,asymptr) && !asymptr->isFunction())
      {
	std::ostringstream msg;
	msg << m_state.m_pool.getDataAsString(asymptr->getId()).c_str() << " cannot be used again as a function, it has a previous definition as '" << asymptr->getUlamType()->getUlamTypeNameBrief(&m_state).c_str() << " " << m_state.m_pool.getDataAsString(asymptr->getId()).c_str() << "'";
	MSG(&typeTok, msg.str().c_str(), ERR);

	// eat tokens until end of definition ???
	return NULL;
      } 

    // not in scope, or not yet defined, or possible overloading
    // o.w. build symbol for function: return type + name + parameter symbols
    Node * rtnNode = makeFunctionBlock(typeTok, identTok);

    // exclude the declaration & definition from the parse tree 
    // (since in SymbolFunction) & return NULL.

    return rtnNode;  //makeFunctionSymbol
  }


  NodeBlockFunctionDefinition * Parser::makeFunctionBlock(Token typeTok, Token identTok)
  {
    NodeBlockFunctionDefinition * rtnNode = NULL;

    // all functions are defined in this "class" block; 
    // or external 'use' for declarations.
    // this is a block with its own ST
    NodeBlock * prevBlock = m_state.m_currentBlock;  //restore before returning
    assert(prevBlock == m_state.m_classBlock);

    // o.w. build symbol for function: name, return type, plus arg symbols
    UlamType * ut = m_state.getUlamTypeFromToken(typeTok);
    SymbolFunction * fsymptr = new SymbolFunction(identTok.m_dataindex, ut);

    // WAIT for the parameters, so we can add it to the SymbolFunctionName map..
    //m_state.m_classBlock->addFuncIdToScope(fsymptr->getId(), fsymptr); 
    rtnNode =  new NodeBlockFunctionDefinition(fsymptr, prevBlock, m_state);
    rtnNode->setNodeLocation(typeTok.m_locator);
    
    // symbol will have pointer to body (or just decl for 'use');
    fsymptr->setFunctionNode(rtnNode); // tfr ownership
                                       
    m_state.m_currentBlock = rtnNode;  //before parsing the args

    // use space on funcCallStack for return statement.
    // negative for parameters; allot space at top for the return value
    // currently, only scalar; determines start position of first arg "under".
    u32 returnArraySize = fsymptr->getUlamType()->getArraySize();
    returnArraySize = (returnArraySize > 0 ? returnArraySize : 1);
    m_state.m_currentFunctionBlockDeclSize = - (returnArraySize + 1);  
    m_state.m_currentFunctionBlockMaxDepth = 0;

    // parse and add parameters to function symbol
    parseRestOfFunctionParameters(fsymptr);
    
    // Now, look specifically for a function with the same given name defined
    Symbol * fnSym = NULL;
    if(!m_state.m_classBlock->isFuncIdInScope(identTok.m_dataindex, fnSym))
      {
	// first time name used as a function..add symbol function name/type
	fnSym = new SymbolFunctionName(identTok.m_dataindex, ut);
	
	// ownership goes to the class block's ST
	m_state.m_classBlock->addFuncIdToScope(fnSym->getId(), fnSym); 
      }


    // verify return types agree (definitely when new name) --- o.w. error!
    if(fnSym->getUlamType() != fsymptr->getUlamType())
      {
	std::ostringstream msg;
	msg << "Return Type <"  << m_state.getTokenAsATypeName(typeTok).c_str() << "> does not agree with return type of already defined function '" << m_state.m_pool.getDataAsString(fnSym->getId()) << "' with the same name" ;
	MSG(&typeTok, msg.str().c_str(),ERR);
	delete fsymptr;
	rtnNode = NULL;
      }


    if(rtnNode)
      {
	bool isAdded = ((SymbolFunctionName *) fnSym)->overloadFunction(fsymptr, &m_state); //transfers ownership, if added
	if(!isAdded)
	  {
	    //this is a duplicate function definition with same parameters and given name!!
	    std::ostringstream msg;
	    msg << "Duplicate defined function '" << m_state.m_pool.getDataAsString(fsymptr->getId()) << "' with the same parameters" ;
	    MSG(&typeTok, msg.str().c_str(),ERR);
	    delete fsymptr;         //also deletes the NodeBlockFunctionDefinition
	    rtnNode = NULL;
	  }
      }


    if(rtnNode)
      {
	//starts with positive one for local variables
	m_state.m_currentFunctionBlockDeclSize = 1;  
	m_state.m_currentFunctionBlockMaxDepth = 0;
	
	//parse body definition
	if(parseFunctionBody(rtnNode))
	  {
	    rtnNode->setDefinition();
	    rtnNode->setMaxDepth(m_state.m_currentFunctionBlockMaxDepth);
	  }
	else
	  {
	    fsymptr->setFunctionNode((NodeBlockFunctionDefinition *) NULL); //deletes node
	    rtnNode = NULL;
	  }
      }

    // this block's ST is no longer in scope
    m_state.m_currentBlock = prevBlock;
    m_state.m_currentFunctionBlockDeclSize = 0;  //default zero for datamembers
    m_state.m_currentFunctionBlockMaxDepth = 0;  //reset
    
    return rtnNode;  //makeFunctionBlock
  }


 void Parser::parseRestOfFunctionParameters(SymbolFunction * sym)
  {
    Token pTok;
    getNextToken(pTok);
    if(pTok.m_type == TOK_CLOSE_PAREN)
      {
	return;  //done with parameters
      }

    assert(sym);
    // allows function name to be same as arg name 
    // since the function starts a new "block" (i.e. ST);
    // the argument to parseDecl will prevent it from looking
    // for restofdecls
    if(Token::isTokenAType(pTok))
      {
	unreadToken();
	Node * argNode = parseDecl(true);  

	// could be null symbol already in scope
	if(argNode) 
	  {
	    //it IS a variable (declaration).
	    Symbol * argSym;
	    if(argNode->getSymbolPtr(argSym))
	      sym->addParameterSymbol(argSym); //ownership stays with NodeBlockFunctionDefinition's ST
	    else
	      MSG(&pTok, "No symbol from parameter declaration", ERR);
	  }

	delete argNode;    //no longer needed
      }
    else
      {
	std::ostringstream msg;
	msg << "Expected 'A Type' Token!! got Token: <" << pTok.getTokenEnumName() << "> instead";
	MSG(&pTok, msg.str().c_str(),ERR);
	//continue or short-circuit???
      }

    getExpectedToken(TOK_COMMA, QUIETLY); // if so, get next arg; o.w. unread
    return parseRestOfFunctionParameters(sym);
  }


  bool Parser::parseFunctionBody(NodeBlockFunctionDefinition *& funcNode)
  {
    bool brtn = false;
    
    // if next token is { this is a definition; o.w. a declaration, alone invalid
    Token pTok;
    if(getExpectedToken(TOK_OPEN_CURLY, pTok))
      {
	brtn = true;
	//build definition!! (not a new block)
	Token pTok;
	getNextToken(pTok);
	unreadToken();

	NodeStatements * nextNode;
	if(pTok.m_type == TOK_CLOSE_CURLY)
	  {
	    MSG(&pTok, "Empty Function Definition", WARN);
	    nextNode = new NodeBlockEmpty(m_state.m_currentBlock, m_state); //legal
	    nextNode->setNodeLocation(pTok.m_locator);
	  }
	else
	  {
	    nextNode = (NodeStatements *) parseStatements();
	  }

	funcNode->setNextNode(nextNode);
	
	if(!getExpectedToken(TOK_CLOSE_CURLY))
	  {
	    MSG(&pTok, "Not close curly", ERR);
	    assert(0);  //???
	  }
	else
	  unreadToken();
      }

    return brtn;
  }


  Node * Parser::makeTypedefSymbol(Token typeTok, Token identTok)
  {
    NodeTypedef * rtnNode = NULL;
    Node * lvalNode = parseLvalExpr(identTok);
 
    if(lvalNode)
      {
	// lvalNode could be either a NodeTerminalIdent or a NodeBinaryOpSquareBracket
	// process identifier...check if already defined in current scope; if not, add it;
	// returned symbol could be symbolVariable or symbolFunction, detect first.
	Symbol * asymptr = NULL;
	u32 arraysize = 0;

	if(!lvalNode->installSymbolTypedef(typeTok, 0, arraysize, asymptr))
	  {
	    if(asymptr)
	      {
		std::ostringstream msg;
		msg << m_state.m_pool.getDataAsString(asymptr->getId()).c_str() << " has a previous declaration as '" << asymptr->getUlamType()->getUlamTypeNameBrief(&m_state).c_str() << " " << m_state.m_pool.getDataAsString(asymptr->getId()) << "'";
		MSG(&typeTok, msg.str().c_str(), ERR);
	      }
	    else 
	      {
		//installSymbol failed for other reasons (e.g. problem with []) , error already output.
		// rtnNode is NULL;
		std::ostringstream msg;
		msg << "Invalid typedef of Type: <" << m_state.getTokenAsATypeName(typeTok).c_str() << "> and Name: <" << m_state.getDataAsString(&identTok).c_str() << "> (missing symbol)";
		MSG(&typeTok, msg.str().c_str(), ERR);
	      }
	  }
	else
	  {
	    rtnNode =  new NodeTypedef((SymbolTypedef *) asymptr, m_state);
	    rtnNode->setNodeLocation(typeTok.m_locator);
	  }
	
	delete lvalNode;  //done with it
      }

    return rtnNode;  //makeTypedefSymbol
  } 



  Node * Parser::parseRestOfExpression(Node * leftNode)
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    switch(pTok.m_type)
      {
      case TOK_PLUS:
      case TOK_MINUS:
      case TOK_AMP:
      case TOK_PIPE:
      case TOK_HAT:
	unreadToken();
	rtnNode = makeExpressionNode(leftNode);
	rtnNode = parseRestOfExpression(rtnNode);  //recursion of left-associativity
	break;
      case TOK_ERROR_CONT:
	{
	  std::ostringstream msg;
	  msg << "Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">";
	  MSG(&pTok, msg.str().c_str(),ERR);
	  rtnNode = parseRestOfExpression(leftNode);  //redo
	}
	break;
      default:
	{
	  unreadToken();
	  rtnNode = leftNode;
	}
      };
    
    return rtnNode;  //parseRestOfExpression
  }


  Node * Parser::parseRestOfAssignExpr(Node * leftNode)
  {
    Node * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    switch(pTok.m_type)
      {
      case TOK_EQUAL:
      case TOK_PLUS_EQUAL:
      case TOK_MINUS_EQUAL:
      case TOK_STAR_EQUAL:
      case TOK_AMP_EQUAL:
      case TOK_PIPE_EQUAL:
      case TOK_HAT_EQUAL:
	unreadToken();
	rtnNode = makeAssignExprNode(leftNode);
	break;
      case TOK_PLUS:
      case TOK_MINUS:
      case TOK_AMP:
      case TOK_PIPE:
      case TOK_HAT:
	unreadToken();
	rtnNode = parseRestOfExpression(leftNode);  //addOp
	break;
      case TOK_STAR:
      case TOK_SLASH:
	unreadToken();
	rtnNode = parseRestOfTerm(leftNode);  //mulOp
	break;
      case TOK_SEMICOLON:
      case TOK_CLOSE_PAREN:
      default:
	{	  
	  unreadToken();
	  rtnNode = leftNode;
	}
      };
    
    return rtnNode;  //parseRestOfAssignExpr
  }


  NodeBinaryOpEqual * Parser::makeAssignExprNode(Node * leftNode)
  {
    NodeBinaryOpEqual * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);  //some flavor of equal token

    Node * rightNode = parseAssignExpr();
    if(!rightNode)
      {
	std::ostringstream msg;
	msg << "RHS of binary operator" << pTok.getTokenString() << " is missing, Assignment deleted";
	MSG(&pTok, msg.str().c_str(), DEBUG);
	delete leftNode;
      }
    else
      {
	switch(pTok.m_type)
	  {
	  case TOK_EQUAL:
	    rtnNode = new NodeBinaryOpEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_PLUS_EQUAL:
	    rtnNode = new NodeBinaryOpPlusEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_MINUS_EQUAL:
	    rtnNode = new NodeBinaryOpMinusEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_STAR_EQUAL:
	    rtnNode = new NodeBinaryOpMultiplyEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_AMP_EQUAL:
	    rtnNode = new NodeBinaryOpBitwiseAndEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_PIPE_EQUAL:
	    rtnNode = new NodeBinaryOpBitwiseOrEqual(leftNode, rightNode, m_state);
	    break;
	  case TOK_HAT_EQUAL:
	    rtnNode = new NodeBinaryOpBitwiseXorEqual(leftNode, rightNode, m_state);
	    break;
	  default:
	    {
	      std::ostringstream msg;
	      msg << " Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">, aborting";
	      MSG(&pTok, msg.str().c_str(), DEBUG);
	      assert(0);
	    }
	    break;
	  };

	assert(rtnNode);
	rtnNode->setNodeLocation(pTok.m_locator);
      }

    return rtnNode;
  }


  NodeBinaryOp * Parser::makeExpressionNode(Node * leftNode)
  {
    NodeBinaryOp * rtnNode = NULL;
    Token pTok;

    getNextToken(pTok);

    Node * rightNode = parseTerm(); 
    if(!rightNode)
      {
	std::ostringstream msg;
	msg << "RHS of binary operator" << pTok.getTokenString() << " is missing, operation deleted";
	MSG(&pTok, msg.str().c_str(), DEBUG);
	delete leftNode;
      }
    else
      {
	switch(pTok.m_type)
	  {
	  case TOK_PLUS:
	    rtnNode = new NodeBinaryOpAdd(leftNode, rightNode, m_state);
	    break;
	  case TOK_MINUS:
	    rtnNode = new NodeBinaryOpSubtract(leftNode, rightNode, m_state);
	    break;
	  case TOK_AMP:
	    rtnNode = new NodeBinaryOpBitwiseAnd(leftNode, rightNode, m_state);
	    break;
	  case TOK_PIPE:
	    rtnNode = new NodeBinaryOpBitwiseOr(leftNode, rightNode, m_state);
	    break;
	  case TOK_HAT:
	    rtnNode = new NodeBinaryOpBitwiseXor(leftNode, rightNode, m_state);
	    break;
	  default:
	    {
	      std::ostringstream msg;
	      msg << " Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">, aborting";
	      MSG(&pTok, msg.str().c_str(), DEBUG);
	      assert(0);
	    }
	    break;
	  };

	assert(rtnNode);
	rtnNode->setNodeLocation(pTok.m_locator);
      }

    return rtnNode;
  }


  NodeBinaryOp * Parser::makeTermNode(Node * leftNode)
  {
    NodeBinaryOp * rtnNode = NULL;
    Token pTok;
    
    getNextToken(pTok);

    Node * rightNode = parseFactor();
    if(!rightNode)
      {
	std::ostringstream msg;
	msg << "RHS of binary operator" << pTok.getTokenString() << " is missing, operation deleted";
	MSG(&pTok, msg.str().c_str(), DEBUG);
	delete leftNode;
      }
    else
      {
	switch(pTok.m_type)
	  {
	  case TOK_STAR:
	    rtnNode = new NodeBinaryOpMultiply(leftNode, rightNode, m_state);
	    break;
	  case TOK_SLASH:
	    rtnNode = new NodeBinaryOpDivide(leftNode, rightNode, m_state);
	    break;
	  default:
	    {
	      std::ostringstream msg;
	      msg << " Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">, aborting";
	      MSG(&pTok, msg.str().c_str(), DEBUG);
	      assert(0);
	    }
	    break;
	  };

	assert(rtnNode);
	rtnNode->setNodeLocation(pTok.m_locator);
      }

    return rtnNode;
  }


  NodeUnaryOp * Parser::makeFactorNode()
  {
    NodeUnaryOp * rtnNode = NULL;
    Token pTok;
    
    getNextToken(pTok);

    Node * factorNode = parseFactor();
    if(!factorNode)
      {
	std::ostringstream msg;
	msg << "Factor is missing, unary operation " << pTok.getTokenString() << " deleted";
	MSG(&pTok, msg.str().c_str(), DEBUG);
      }
    else
      {
	switch(pTok.m_type)
	  {
	  case TOK_MINUS:
	    rtnNode = new NodeUnaryOpMinus(factorNode, m_state);
	    break;
	  case TOK_PLUS:
	    rtnNode = new NodeUnaryOpPlus(factorNode, m_state);
	    break;
	  case TOK_BANG:
	    rtnNode = new NodeUnaryOpBang(factorNode, m_state);
	    break;
	  default:
	    {
	      std::ostringstream msg;
	      msg << " Unexpected input!! Token: <" << pTok.getTokenEnumName() << ">, aborting";
	      MSG(&pTok, msg.str().c_str(), DEBUG);
	      assert(0);
	    }
	    break;
	  };
	      
	assert(rtnNode);
	rtnNode->setNodeLocation(pTok.m_locator);
      }
    return rtnNode;
  }


  bool Parser::getExpectedToken(TokenType eTokType, bool quietly)
  {
    Token dropTok;
    return getExpectedToken(eTokType, dropTok, QUIETLY);
  }


  bool Parser::getExpectedToken(TokenType eTokType, Token & myTok, bool quietly)
  {
    Token pTok;

    getNextToken(pTok);

    if(pTok.m_type != eTokType)
      {
	unreadToken();

	if(!quietly)
	  {
	    std::ostringstream msg;
	    msg << "Unexpected token <" << pTok.getTokenEnumName() << ">; Expected <" << Token::getTokenAsString(eTokType) << ">";
	    MSG(&pTok, msg.str().c_str(), ERR);
	  }
	return false;
      }

    myTok = pTok;
    return true;
  }


  bool Parser::getNextToken(Token & tok)
  {
    bool brtn = m_tokenizer->getNextToken(tok);

    if(tok.m_type == TOK_ERROR_ABORT)
      {
	std::ostringstream msg;
	msg << "Unexpected token <" << tok.getTokenEnumName() << ">, exiting now";
	MSG(&tok, msg.str().c_str(), ERR);
	exit(1);
      }

    return brtn;
  }

 
  void Parser::unreadToken()
  {
    m_tokenizer->unreadToken();
  }


  void Parser::getTokensUntil(TokenType lastTok)
  {
    Token aTok;
    getNextToken(aTok);
    while(aTok.m_type != lastTok)
      {
	if(aTok.m_type == TOK_EOF)
	  {
	    unreadToken();
	    break;
	  }
	getNextToken(aTok);
      }
    return;
  }


  void Parser::initPrimitiveUlamTypes()
  {
    // initialize primitive UlamTypes, in order!!
    UlamKeyTypeSignature nkey(m_state.m_pool.getIndexForDataString("Nav"), 8);
    UTI nidx = m_state.makeUlamType(nkey, Nav);
    assert(nidx == Nav);  //true for primitives

    UlamKeyTypeSignature vkey(m_state.m_pool.getIndexForDataString("Void"), 0);
    UTI vidx = m_state.makeUlamType(vkey, Void);
    assert(vidx == Void);  //true for primitives

    UlamKeyTypeSignature ikey(m_state.m_pool.getIndexForDataString("Int"), 32);
    UTI iidx = m_state.makeUlamType(ikey, Int);
    assert(iidx == Int);

    UlamKeyTypeSignature fkey(m_state.m_pool.getIndexForDataString("Float"), 32);
    UTI fidx = m_state.makeUlamType(fkey, Float);
    assert(fidx == Float);

    UlamKeyTypeSignature bkey(m_state.m_pool.getIndexForDataString("Bool"), 8);
    UTI bidx = m_state.makeUlamType(bkey, Bool);
    assert(bidx == Bool);

    UlamKeyTypeSignature ckey(m_state.m_pool.getIndexForDataString("Ut_Class"), 0);
    UTI cidx = m_state.makeUlamType(ckey, Class);
    assert(cidx == Class);

    UlamKeyTypeSignature akey(m_state.m_pool.getIndexForDataString("Atom"), BITSPERATOM);
    UTI aidx = m_state.makeUlamType(akey, Atom);
    assert(aidx == Atom);


    //initialize call stack with 'Int' UlamType pointer
    m_state.m_funcCallStack.init(m_state.getUlamTypeByIndex(iidx));
    m_state.m_nodeEvalStack.init(m_state.getUlamTypeByIndex(iidx));
    m_state.m_selectedAtom.init(m_state.getUlamTypeByIndex(iidx)); //necessary?
  }
   

} //end MFM
