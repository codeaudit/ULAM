/**                                        -*- mode:C++ -*-
 * NodeControlIf.h - Node for handling If-Then-Else for ULAM
 *
 * Copyright (C) 2014-2016 The Regents of the University of New Mexico.
 * Copyright (C) 2014-2016 Ackleyshack LLC.
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
  \file NodeControlIf.h - Node for handling If-Then-Else for ULAM
  \author Elenas S. Ackley.
  \author David H. Ackley.
  \date (C) 2014-2016 All rights reserved.
  \gpl
*/


#ifndef NODECONTROLIF_H
#define NODECONTROLIF_H

#include "NodeControl.h"

namespace MFM{

  class NodeControlIf : public NodeControl
  {
  public:

    NodeControlIf(Node * condNode, Node * trueNode, Node * falseNode, CompilerState & state);
    NodeControlIf(const NodeControlIf& ref);
    virtual ~NodeControlIf();

    virtual Node * instantiate();

    virtual void updateLineage(NNO pno);

    virtual bool exchangeKids(Node * oldnptr, Node * newnptr);

    virtual bool findNodeNo(NNO n, Node *& foundNode);

    virtual void checkAbstractInstanceErrors();

    virtual void printPostfix(File * fp);

    virtual void print(File * fp);

    virtual UTI checkAndLabelType();

    virtual void calcMaxDepth(u32& depth, u32& maxdepth, s32 base);

    virtual void countNavHzyNoutiNodes(u32& ncnt, u32& hcnt, u32& nocnt);

    virtual EvalStatus eval();

    virtual const char * getName();

    virtual const std::string prettyNodeName();

    virtual void genCode(File * fp, UVPass& uvpass);

  protected:

    Node * m_nodeElse;

  };

}

#endif //end NODECONTROLIF_H