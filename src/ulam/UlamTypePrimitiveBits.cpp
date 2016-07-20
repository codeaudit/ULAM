#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include "UlamTypePrimitiveBits.h"
#include "UlamValue.h"
#include "CompilerState.h"

namespace MFM {

  UlamTypePrimitiveBits::UlamTypePrimitiveBits(const UlamKeyTypeSignature key, CompilerState & state) : UlamTypePrimitive(key, state)
  {
    s32 bitsize = getBitSize();
    if(bitsize <= 0)
      {
	m_max = m_min = 0;
      }
    else if(bitsize <= MAXBITSPERINT)
      {
	m_wordLengthTotal = calcWordSize(getTotalBitSize());
	m_wordLengthItem = calcWordSize(bitsize);
	m_max = calcBitsizeUnsignedMax(bitsize);
	m_min = 0;
      }
    else if(bitsize <= MAXBITSPERLONG)
      {
	m_wordLengthTotal = calcWordSize(getTotalBitSize());
	m_wordLengthItem = calcWordSize(bitsize);
	m_max = calcBitsizeUnsignedMaxLong(bitsize);
	m_min = 0;
      }
    else
      assert(0);
  }

   ULAMTYPE UlamTypePrimitiveBits::getUlamTypeEnum()
   {
     return Bits;
   }

   bool UlamTypePrimitiveBits::isMinMaxAllowed()
  {
    return false;
  }

  bool UlamTypePrimitiveBits::cast(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    assert(m_state.getUlamTypeByIndex(typidx) == this);
    UTI valtypidx = val.getUlamValueTypeIdx();

    if(UlamType::safeCast(valtypidx) != CAST_CLEAR) //bad|hazy
      return false;

    u32 wordsize = getTotalWordSize();
    u32 valwordsize = m_state.getTotalWordSize(valtypidx);
    if(wordsize <= MAXBITSPERINT)
      {
	if(valwordsize <= MAXBITSPERINT)
	  brtn = castTo32(val, typidx);
	else if(valwordsize <= MAXBITSPERLONG)
	  brtn = castTo64(val, typidx); //downcast
	else
	  assert(0);
      }
    else if(wordsize <= MAXBITSPERLONG)
      brtn = castTo64(val, typidx);
    else
      {
	std::ostringstream msg;
	msg << "Casting to an unsupported word size: " << wordsize;
	msg << ", Value Type and bit size was: ";
	msg << valtypidx << "," << m_state.getBitSize(valtypidx);
	msg << " TO: ";
	msg << typidx << "," << getBitSize();
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), DEBUG);
	brtn = false;
      }
    return brtn;
  } //cast

  bool UlamTypePrimitiveBits::castTo32(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    UTI valtypidx = val.getUlamValueTypeIdx();
    u32 data = val.getImmediateData(m_state);

    //no changes to data, only type; unless theres a wordsize difference
    ULAMTYPE valtypEnum = m_state.getUlamTypeByIndex(valtypidx)->getUlamTypeEnum();
    switch(valtypEnum)
      {
      case Void:
      case Int:
      case Unsigned:
      case Bool:
      case Unary:
      case Bits:
	val = UlamValue::makeImmediate(typidx, data, m_state); //overwrite val
	break;
      default:
	//std::cerr << "UlamTypeInt (cast) error! Value Type was: " << valtypidx << std::endl;
	brtn = false;
      };

    return brtn;
  } //castTo32

  bool UlamTypePrimitiveBits::castTo64(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    UTI valtypidx = val.getUlamValueTypeIdx();
    u32 valwordsize = m_state.getTotalWordSize(valtypidx);
    u64 data;

    if(valwordsize <= MAXBITSPERINT)
      data = (u64) val.getImmediateData(m_state);
    else if(valwordsize <= MAXBITSPERLONG)
      data = val.getImmediateDataLong(m_state);
    else
      assert(0);

    //no changes to data, only type
    ULAMTYPE valtypEnum = m_state.getUlamTypeByIndex(valtypidx)->getUlamTypeEnum();
    switch(valtypEnum)
      {
      case Void:
      case Int:
      case Unsigned:
      case Bool:
      case Unary:
      case Bits:
	val = UlamValue::makeImmediateLong(typidx, data, m_state); //overwrite val
	break;
      default:
	//std::cerr << "UlamTypeInt (cast) error! Value Type was: " << valtypidx << std::endl;
	brtn = false;
      };
    return brtn;
  } //castTo64

  FORECAST UlamTypePrimitiveBits::safeCast(UTI typidx)
  {
    FORECAST scr = UlamType::safeCast(typidx);
    if(scr != CAST_CLEAR)
      return scr;

    bool brtn = true;
    UlamType * vut = m_state.getUlamTypeByIndex(typidx);
    s32 valbitsize = vut->getBitSize();
    s32 bitsize = getBitSize();
    ULAMTYPE valtypEnum = vut->getUlamTypeEnum();
    switch(valtypEnum)
      {
      case Unsigned:
      case Unary:
      case Int:
      case Bool:
      case Bits:
	brtn = (bitsize >= valbitsize); //downhill
	break;
      case Void:
      case UAtom:
	brtn = false;
	break;
      case Class:
	{
	  //must be Quark! treat as Int if it has a toInt method
	  if(vut->isNumericType())
	    brtn = (bitsize >= MAXBITSPERINT);
	  else
	    {
	      std::ostringstream msg;
	      msg << "Class: ";
	      msg << m_state.getUlamTypeNameBriefByIndex(typidx).c_str();
	      msg << " is not a numeric type and cannot be safely cast to Bits";
	      MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(),msg.str().c_str(), ERR);
	      brtn = false;
	    }
	}
	break;
      default:
	assert(0);
	//std::cerr << "UlamTypePrimitiveBits (cast) error! Value Type was: " << valtypidx << std::endl;
	brtn = false;
      };
    return brtn ? CAST_CLEAR : CAST_BAD;
  } //safeCast

  void UlamTypePrimitiveBits::getDataAsString(const u32 data, char * valstr, char prefix)
  {
    if(prefix == 'z')
      sprintf(valstr,"%u", data);
    else
      sprintf(valstr,"%c%u", prefix, data);
  }

  void UlamTypePrimitiveBits::getDataLongAsString(const u64 data, char * valstr, char prefix)
  {
    if(prefix == 'z')
      sprintf(valstr,"%s", ToUnsignedDecimal(data).c_str());
    else
      sprintf(valstr,"%c%s", prefix, ToUnsignedDecimal(data).c_str());
  }

  s32 UlamTypePrimitiveBits::getDataAsCs32(const u32 data)
  {
    return _Bits32ToCs32(data, getBitSize());
  }

  u32 UlamTypePrimitiveBits::getDataAsCu32(const u32 data)
  {
    return _Bits32ToCu32(data, getBitSize());
  }

  s64 UlamTypePrimitiveBits::getDataAsCs64(const u64 data)
  {
    return _Bits64ToCs64(data, getBitSize());
  }

  u64 UlamTypePrimitiveBits::getDataAsCu64(const u64 data)
  {
    return _Bits64ToCu64(data, getBitSize());
  }

  s32 UlamTypePrimitiveBits::bitsizeToConvertTypeTo(ULAMTYPE tobUT)
  {
    s32 bitsize = getBitSize();
    s32 tobitsize = UNKNOWNSIZE;
    s32 wordsize = getTotalWordSize();
    switch(tobUT)
      {
      case Bool:
      case Unsigned:
      case Unary:
      case Int:
      case Bits:
	tobitsize = bitsize;
	break;
      case Void:
	tobitsize = 0;
	break;
      case UAtom:
      case Class:
	break;
      default:
	assert(0);
	//std::cerr << "UlamTypePrimitiveBits (convertTo) error! " << tobUT << std::endl;
      };
    return (tobitsize > wordsize ? wordsize : tobitsize);
  } //bitsizeToConvertTypeTo

} //end MFM
