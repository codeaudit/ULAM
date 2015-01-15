/**                                        -*- mode:C++ -*-
 * NodeConditional.h - Basic Node for handling Conditional Expressions for ULAM
 *
 * Copyright (C) 2014 The Regents of the University of New Mexico.
 * Copyright (C) 2014 Ackleyshack LLC.
 *
 * This file is part of the ULAM programming language compilation system.
 *
 * The ULAM programming language compilation system is free software:
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * The ULAM programming language compilation system is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the ULAM programming language compilation system
 * software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

/**
  \file NodeConditional.h - Basic Node for handling Conditional Expressions for ULAM
  \author Elenas S. Ackley.
  \author David H. Ackley.
  \date (C) 2014 All rights reserved.
  \gpl
*/


#ifndef NODECONDITIONAL_H
#define NODECONDITIONAL_H

#include "Node.h"
#include "NodeCast.h"

namespace MFM{

  class NodeConditional : public Node
  {
  public:

    NodeConditional(Node * leftNode, Token typeTok, CompilerState & state);
    ~NodeConditional();

    virtual void updateLineage(Node * p);

    virtual void print(File * fp);

    virtual void printPostfix(File * fp);

    virtual void printOp(File * fp);

    virtual const char * getName() = 0;

    virtual const std::string prettyNodeName() = 0;

    //virtual UTI checkAndLabelType();

    virtual void countNavNodes(u32& cnt);

    virtual const std::string methodNameForCodeGen() = 0;

    //virtual EvalStatus eval();

    //virtual void genCode(File * fp, UlamValue& uvpass);

    //TODO:
    //virtual void genCodeToStoreInto(File * fp, UlamValue& uvpass);

    Token getTypeToken();

  protected:
    Node * m_nodeLeft;
    Token m_typeTok;  //right
    UTI m_utypeRight;
  };

}

#endif //end NODECONDITIONAL_H
