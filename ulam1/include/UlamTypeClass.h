/**                                        -*- mode:C++ -*-
 * UlamTypeClass.h -  Basic handling of the Class UlamType for ULAM
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
  \file UlamTypeClass.h -  Basic handling of the Class UlamType for ULAM
  \author Elenas S. Ackley.
  \author David H. Ackley.
  \date (C) 2014-2016 All rights reserved.
  \gpl
*/


#ifndef ULAMTYPECLASS_H
#define ULAMTYPECLASS_H

#include "Node.h"
#include "UlamType.h"

namespace MFM{

  class CompilerState; //forward

  class UlamTypeClass : public UlamType
  {
  public:

    UlamTypeClass(const UlamKeyTypeSignature key, CompilerState& state);

    virtual ~UlamTypeClass(){}

    virtual ULAMTYPE getUlamTypeEnum();

    virtual bool isNumericType();

    virtual bool isPrimitiveType();

    virtual bool cast(UlamValue& val, UTI typidx);

    virtual FORECAST safeCast(UTI typidx);

    virtual FORECAST explicitlyCastable(UTI typidx);

    virtual const char * getUlamTypeAsSingleLowercaseLetter();

    virtual const std::string getUlamTypeMangledType();

    virtual  const std::string getUlamTypeMangledName();

    virtual const std::string getUlamTypeUPrefix();

    virtual const std::string getUlamTypeNameBrief();

    virtual void getDataAsString(const u32 data, char * valstr, char prefix);

    virtual void getDataLongAsString(const u64 data, char * valstr, char prefix);

    virtual ULAMCLASSTYPE getUlamClassType();

    virtual bool isCustomArray();

    void setCustomArray();

    virtual bool isHolder();

    virtual bool isComplete();

    virtual const std::string readMethodForCodeGen();

    virtual const std::string writeMethodForCodeGen();

    virtual bool needsImmediateType();

    virtual const std::string getUlamTypeImmediateMangledName();

    virtual const std::string getUlamTypeImmediateAutoMangledName();

    virtual const std::string getTmpStorageTypeAsString();

    virtual const std::string getArrayItemTmpStorageTypeAsString();

    virtual const std::string getLocalStorageTypeAsString();

    virtual STORAGE getTmpStorageTypeForTmpVar();

    virtual const std::string castMethodForCodeGen(UTI nodetype);

    virtual void genUlamTypeMangledAutoDefinitionForC(File * fp);

    virtual void genUlamTypeReadDefinitionForC(File * fp);

    virtual void genUlamTypeWriteDefinitionForC(File * fp);

    virtual const std::string readArrayItemMethodForCodeGen();

    virtual const std::string writeArrayItemMethodForCodeGen();

    virtual void genUlamTypeMangledDefinitionForC(File * fp);

    virtual void genUlamTypeMangledUnpackedArrayAutoDefinitionForC(File * fp);

    virtual void genUlamTypeMangledUnpackedArrayDefinitionForC(File * fp);

   private:

    bool m_customArray;

    UTI getCustomArrayType();
    u32 getCustomArrayIndexTypeFor(Node * rnode, UTI& idxuti, bool& hasHazyArgs);

  };

}

#endif //end ULAMTYPECLASS_H
