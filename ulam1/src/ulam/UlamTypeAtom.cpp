#include <sstream>
#include <stdio.h>
#include "UlamTypeAtom.h"
#include "UlamValue.h"
#include "CompilerState.h"

namespace MFM {

  UlamTypeAtom::UlamTypeAtom(const UlamKeyTypeSignature key, CompilerState & state) : UlamType(key, state)
  {
    m_wordLengthTotal = calcWordSize(getTotalBitSize());
    m_wordLengthItem = calcWordSize(getBitSize());
  }

  ULAMTYPE UlamTypeAtom::getUlamTypeEnum()
  {
    return UAtom;
  }

  ULAMCLASSTYPE UlamTypeAtom::getUlamClassType()
  {
    //return UC_ATOM; //llok into this!!
    return UC_NOTACLASS;
  }

  bool UlamTypeAtom::needsImmediateType()
  {
    return true; //isComplete();
  }

  const std::string UlamTypeAtom::getArrayItemTmpStorageTypeAsString()
  {
    return "T";
  }

  const std::string UlamTypeAtom::getTmpStorageTypeAsString()
  {
    if(!isScalar())
      {
	std::ostringstream cstr;
	cstr << "BitVector<" << getTotalBitSize() << ">";
	return cstr.str();
      }
    return "T";
  } //getTmpStorageTypeAsString

  const std::string UlamTypeAtom::getLocalStorageTypeAsString()
  {
    std::ostringstream ctype;
    ctype << getUlamTypeImmediateMangledName() << "<EC>";
    return ctype.str();
  }

  STORAGE UlamTypeAtom::getTmpStorageTypeForTmpVar()
  {
    return TMPTATOM;
  }

  bool UlamTypeAtom::isMinMaxAllowed()
  {
    return false;
  }

  PACKFIT UlamTypeAtom::getPackable()
  {
    return UNPACKED;
  }

  bool UlamTypeAtom::cast(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    assert(m_state.getUlamTypeByIndex(typidx) == this);
    UTI valtypidx = val.getUlamValueTypeIdx();
    UlamType * vut = m_state.getUlamTypeByIndex(valtypidx);
    AssertBool isScalars = (vut->isScalar() && isScalar());
    assert(isScalars);

    // what change is to be made ????
    // atom type vs. class type
    // how can it be both in an UlamValue?
    // what of its contents?
    // val = UlamValue::makeAtom(valtypidx);

    if((vut->getUlamClassType() == UC_ELEMENT) || m_state.isAtom(valtypidx) || (vut->getUlamTypeEnum() == Class && vut->isReference()))
      val.setUlamValueTypeIdx(typidx); //try this
    else
      brtn = false;
    return brtn;
  } //end cast

  FORECAST UlamTypeAtom::safeCast(UTI typidx)
  {
    FORECAST scr = UlamType::safeCast(typidx);
    if(scr != CAST_CLEAR)
      return scr;

    if(m_state.isAtom(typidx))
      return CAST_CLEAR; //atom to atom

    //casting from quark or quark ref, requires explicit casting
    UlamType * vut = m_state.getUlamTypeByIndex(typidx);
    return (vut->getUlamClassType() == UC_ELEMENT) ? CAST_CLEAR : CAST_BAD;
   } //safeCast

  FORECAST UlamTypeAtom::explicitlyCastable(UTI typidx)
  {
    //    return safeCast(typidx);
    FORECAST scr = UlamType::explicitlyCastable(typidx); //default, arrays checked
    if(scr == CAST_CLEAR)
      {
	UlamType * vut = m_state.getUlamTypeByIndex(typidx);
	ULAMCLASSTYPE vclasstype = vut->getUlamClassType();
	if(vut->isPrimitiveType())
	  scr = CAST_BAD;
	else if(( vclasstype == UC_QUARK) && !vut->isReference())
	  scr = CAST_BAD; //quark ref possibly ok
	//else atom, element, quark ref (possibly), are acceptable
      }
    return scr;
  } //explicitlyCastable

  const std::string UlamTypeAtom::castMethodForCodeGen(UTI nodetype)
  {
    std::ostringstream rtnMethod;
    UlamType * nut = m_state.getUlamTypeByIndex(nodetype);

    //base types e.g. Int, Bool, Unary, Foo, Bar..
    u32 sizeByIntBitsToBe = getTotalWordSize();
    u32 sizeByIntBits = nut->getTotalWordSize();

    assert(sizeByIntBitsToBe == sizeByIntBits);
    assert(nut->getUlamClassType() == UC_ELEMENT);  //quarks only cast toInt

    rtnMethod << "_" << "Element"  << sizeByIntBits << "To";
    rtnMethod << getUlamTypeNameOnly().c_str() << sizeByIntBitsToBe;
    return rtnMethod.str();
  } //castMethodForCodeGen

  void UlamTypeAtom::genUlamTypeMangledAutoDefinitionForC(File * fp)
  {
    if(!isScalar())
      return genUlamTypeMangledUnpackedArrayAutoDefinitionForC(fp);

    m_state.m_currentIndentLevel = 0;
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());
    fp->write(" : public UlamRefAtom<EC>");
    fp->write("\n");
    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //constructor for ref (auto)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(AtomBitStorage<EC>& targ, const UlamContext<EC>& uc) : UlamRefAtom<EC>(targ, 0u, uc.LookupElementTypeFromContext(targ.GetType())) { }\n");
    //fp->write("(BitStorage<EC>& targ, u32 origin, const UlamContext<EC>& uc) : UlamRefAtom<EC>(targ, origin, uc.LookupElementTypeFromContext(targ.ReadAtom(origin, BPA).GetType())) { }\n");

    //constructor for chain of autorefs (e.g. memberselect with array item)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const UlamRefAtom<EC>& arg, const UlamContext<EC>& uc) : UlamRefAtom<EC>(arg, arg.GetEffectiveSelf()) { }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledAutoDefinitionForC

  //generates immediate for whole atom
  void UlamTypeAtom::genUlamTypeMangledDefinitionForC(File * fp)
  {
    if(!isScalar())
      return genUlamTypeMangledUnpackedArrayDefinitionForC(fp);

    m_state.m_currentIndentLevel = 0;
    const std::string mangledName = getUlamTypeImmediateMangledName();
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write(" : public ");
    fp->write("AtomBitStorage<EC>\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //T
    //fp->write("& targ, const UlamContext<EC>& ucarg) : "); //uc consistent with atomref
    fp->write("& targ) : "); //uc consistent with atomref
    fp->write("AtomBitStorage<EC>");
    fp->write("(targ) { }\n");

    //copy constructor
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write("AtomBitStorage<EC>");
    //fp->write("& d, const UlamContext<EC>& ucarg) : "); //uc consistent with atomref
    fp->write("& d) : "); //uc consistent with atomref
    fp->write("AtomBitStorage<EC>");
    fp->write("(d.ReadAtom()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    genUlamTypeReadDefinitionForC(fp);

    genUlamTypeWriteDefinitionForC(fp);

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledDefinitionForC

  void UlamTypeAtom::genUlamTypeReadDefinitionForC(File * fp)
  {
    return; //uses base class read
  } //genUlamTypeReadDefinitionForC

  void UlamTypeAtom::genUlamTypeWriteDefinitionForC(File * fp)
  {
    return; //uses base class write
  } //genUlamTypeWriteDefinitionForC

  const std::string UlamTypeAtom::readMethodForCodeGen()
  {
    return "ReadAtom"; //GetStorage?
  }

  const std::string UlamTypeAtom::writeMethodForCodeGen()
  {
    return "WriteAtom";
  }

  void UlamTypeAtom::genUlamTypeMangledUnpackedArrayAutoDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    UTI scalarUTI = UAtom;
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalarUTI)->getUlamTypeMangledName();
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(mangledName.c_str());
    fp->write("; //forward \n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("AtomBitStorage<EC> (& m_stgarrayref)[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u];  //ref to BIG storage!\n\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("( ");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : m_stgarrayref(r.m_stgarrayref) { }\n");

    //constructor here (used by tmpVars)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("( ");
    fp->write(mangledName.c_str());
    fp->write("<EC>& d) : m_stgarrayref(d.getStorageRef()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(automangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) { return ");
    fp->write("m_stgarrayref[index].ReadAtom(); /* entire atom item */ }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write("& v, const u32 index, const u32 itemlen) { ");
    fp->write("m_stgarrayref[index].WriteAtom(v); /* entire atom item */ }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitStorage<EC>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index]; }\n");

#if 0
    //couldn't overload according to gcc
    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitStorage<EC>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarrayref[index]; }\n");
#endif

#if 0
    //nolonger a T
    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index].GetStorage(); }\n");
#endif

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index); }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedArrayAutoDefinitionForC

  void UlamTypeAtom::genUlamTypeMangledUnpackedArrayDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;

    //class instance idx is always the scalar uti
    UTI scalaruti =  UAtom;
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 arraysize = getArraySize();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    //Unpacked, storage reference T (&) [N]
    m_state.indent(fp);
    fp->write("typedef AtomBitStorage<EC> TARR[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("];\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("TARR m_stgarr;  //BIG storage here!\n\n");

#if 0
    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) ");
    fp->write("m_stgarr[j].GetStorage().SetEmptyImpl();"); //T::ATOM_EMPTY_TYPE
    fp->write(" }\n");
#endif

    //default constructor (used by local vars) new way!
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { ");
    fp->write("T foo = Element_Empty<EC>::THE_INSTANCE.GetDefaultAtom(); ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) ");
    fp->write("m_stgarr[j].WriteAtom(foo);"); //T::ATOM_EMPTY_TYPE
    fp->write(" }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //T
    fp->write("& targ) { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) ");
    fp->write("writeArrayItem(targ, j, BPA);");
    fp->write(" }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("m_stgarr[index].ReadAtom(); /* entire atom item */ }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //T&
    fp->write("& v, const u32 index, const u32 itemlen) { ");
    fp->write("m_stgarr[index].WriteAtom(v); /* entire atom item */ }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    //fp->write("BitVector<BPA>& ");
    //fp->write("getBits(");
    //fp->write("const u32 index) { return ");
    //fp->write("m_stgarr[index].GetBits(); }\n");
    fp->write("BitStorage<EC>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n"); //see if you can use atombitstorage directly.

#if 0
    //Unpacked, an item T const
    m_state.indent(fp);
    //fp->write("const BitVector<BPA>& ");
    //fp->write("getBits(");
    //fp->write("const u32 index) const { return ");
    //fp->write("m_stgarr[index].GetBits(); }\n");
    fp->write("const BitStorage<EC>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n"); //see if you can use atombitstorage directly.

    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index].GetStorage(); }\n");
#endif

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index); }\n");

    m_state.indent(fp);
    fp->write("TARR& getStorageRef(");
    fp->write(") { return ");
    fp->write("m_stgarr; }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedArrayDefinitionForC

} //end MFM
