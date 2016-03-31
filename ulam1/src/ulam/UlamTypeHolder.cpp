#include <iostream>
#include <stdio.h>
#include "UlamTypeHolder.h"
#include "UlamValue.h"
#include "CompilerState.h"

namespace MFM {

  UlamTypeHolder::UlamTypeHolder(const UlamKeyTypeSignature key, CompilerState & state) : UlamType(key, state) {}

  ULAMTYPE UlamTypeHolder::getUlamTypeEnum()
  {
    return Holder;
  }

  bool UlamTypeHolder::needsImmediateType()
  {
    return false;
  }

  ULAMCLASSTYPE UlamTypeHolder::getUlamClassType()
  {
    return UC_NOTACLASS;
  }

  const std::string UlamTypeHolder::getLocalStorageTypeAsString()
  {
    assert(0);
    return "holder";
  }

  const std::string UlamTypeHolder::castMethodForCodeGen(UTI nodetype)
  {
    assert(0);
    return "holder";
  }

  bool UlamTypeHolder::isHolder()
  {
    return true;
  }

  bool UlamTypeHolder::isComplete()
  {
    return false;
  }

  bool UlamTypeHolder::isMinMaxAllowed()
  {
    return isScalar(); //minof/maxof allowed in ulam
  }

} //end MFM
