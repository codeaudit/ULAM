#include <ctype.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "UlamTypeClassQuark.h"
#include "UlamValue.h"
#include "CompilerState.h"
#include "Util.h"

namespace MFM {

  UlamTypeClassQuark::UlamTypeClassQuark(const UlamKeyTypeSignature key, CompilerState & state) : UlamTypeClass(key, state) { }

  bool UlamTypeClassQuark::isNumericType()
  {
    u32 quti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    return (m_state.quarkHasAToIntMethod(quti));
  } //isNumericType

  ULAMCLASSTYPE UlamTypeClassQuark::getUlamClassType()
  {
    return UC_QUARK;
  }

  bool UlamTypeClassQuark::cast(UlamValue & val, UTI typidx)
  {
    bool brtn = true;
    assert(m_state.getUlamTypeByIndex(typidx) == this); //tobe
    UTI valtypidx = val.getUlamValueTypeIdx();
    UlamType * vut = m_state.getUlamTypeByIndex(valtypidx);
    assert(vut->isScalar() && isScalar());
    ULAMTYPE vetype = vut->getUlamTypeEnum();
    //now allowing atoms to be cast as quarks, as well as elements;
    // also allowing subclasses to be cast as their superclass (u1.2.2)
    if(vetype == UAtom)
      brtn = false; //cast atom to a quark?
    else if(UlamType::compare(valtypidx, typidx, m_state) == UTIC_SAME)
      {
	//if same type nothing to do; if atom, shows as element in eval-land.
	//val.setAtomElementTypeIdx(typidx); //?
      }
    else if(m_state.isClassASubclassOf(valtypidx, typidx))
      {
	//2 quarks, or element (val) inherits from this quark
	ULAMCLASSTYPE vclasstype = vut->getUlamClassType();
	if(vclasstype == UC_ELEMENT)
	  {
	    s32 pos = 0; //ancestors start at first state bit pos
	    s32 len = getTotalBitSize();
	    assert(len != UNKNOWNSIZE);
	    if(len <= MAXBITSPERINT)
	      {
		u32 qdata = val.getDataFromAtom(pos + ATOMFIRSTSTATEBITPOS, len);
		val = UlamValue::makeImmediateQuark(typidx, qdata, len);
	      }
	    else if(len <= MAXBITSPERLONG)
	      {
		assert(0); //quarks are max 32 bits
		u64 qdata = val.getDataLongFromAtom(pos + ATOMFIRSTSTATEBITPOS, len);
		val = UlamValue::makeImmediateLong(typidx, qdata, len);
	      }
	    else
	      assert(0);
	  }
	else
	  {
	    // both left-justified immediate quarks
	    // Coo c = (Coo) f.su; where su is a Soo : Coo
	    s32 vlen = vut->getTotalBitSize();
	    s32 len = getTotalBitSize();
	    u32 vdata = val.getImmediateQuarkData(vlen); //not from element
	    assert((vlen - len) >= 0); //sanity check
	    u32 qdata = vdata >> (vlen - len); //stays left-justified
	    val = UlamValue::makeImmediateQuark(typidx, qdata, len);
	  }
      }
    else
      brtn = false;

    return brtn;
  } //end cast

  const char * UlamTypeClassQuark::getUlamTypeAsSingleLowercaseLetter()
  {
    return "q";
  }

  const std::string UlamTypeClassQuark::getUlamTypeUPrefix()
  {
    if(getArraySize() > 0)
      return "Ut_";

    return "Uq_";
  } //getUlamTypeUPrefix

  const std::string UlamTypeClassQuark::readMethodForCodeGen()
  {
    return "Read"; //quarks only 32 bits
  } //readMethodForCodeGen

  const std::string UlamTypeClassQuark::writeMethodForCodeGen()
  {
    return "Write"; //quarks only 32 bits
  } //writeMethodForCodeGen

  bool UlamTypeClassQuark::needsImmediateType()
  {
    if(!isComplete())
      return false;

    bool rtnb = false;
    if(!isReference())
      {
	rtnb = true;
	u32 id = m_key.getUlamKeyTypeSignatureNameId();
	u32 cuti = m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	SymbolClassName * cnsym = (SymbolClassName *) m_state.m_programDefST.getSymbolPtr(id);
	if(cnsym->isClassTemplate() && ((SymbolClassNameTemplate *) cnsym)->isClassTemplate(cuti))
	  rtnb = false; //the "template" has no immediate, only instances
      }
    return rtnb;
  } //needsImmediateType

  const std::string UlamTypeClassQuark::getTmpStorageTypeAsString()
  {
    return "u32";
  }

  const std::string UlamTypeClassQuark::getLocalStorageTypeAsString()
  {
    std::ostringstream ctype;
    ctype << getUlamTypeImmediateMangledName();
    ctype << "<EC>"; //default local quarks
    return ctype.str();
  } //getLocalStorageTypeAsString

  STORAGE UlamTypeClassQuark::getTmpStorageTypeForTmpVar()
  {
    return UlamType::getTmpStorageTypeForTmpVar();
  } //getTmpStorageTypeForTmpVar

  const std::string UlamTypeClassQuark::castMethodForCodeGen(UTI nodetype)
  {
    std::ostringstream msg;
    msg << "Quarks only cast 'toInt': value type and size was: ";
    msg << m_state.getUlamTypeByIndex(nodetype)->getUlamTypeName().c_str();
    msg << ", to be: " << getUlamTypeName().c_str();
    MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(),msg.str().c_str(), ERR);
    return "invalidQuarkCastMethodForCodeGen";
  } //castMethodForCodeGen

  void UlamTypeClassQuark::genUlamTypeMangledAutoDefinitionForC(File * fp)
  {
    s32 len = getTotalBitSize(); //could be 0, includes arrays
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayAutoDefinitionForC(fp);

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

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
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    //forward declaration of quark (before struct!)
    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(scalarmangledName.c_str());
    fp->write(";  //forward\n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n"); //quark auto perPos???

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());
    fp->write(" : public UlamRef<EC>\n");
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

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRef"); //was atomicparametertype
    fp->write("<EC> Up_Us;\n");

    // see UlamClass.h for AutoRefBase
    //constructor for conditional-as (auto)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(T& targ, u32 idx, const UlamClass<EC>* effself) : UlamRef<EC>");
    fp->write("(idx, "); //the real pos!!!
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, targ, effself) { }\n");

    //constructor for chain of autorefs (e.g. memberselect with array item)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const UlamRef<EC>& arg, u32 idx, const UlamClass<EC>* effself) : UlamRef<EC>(arg, idx, ");
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, effself) { }\n");

    //copy constructor here; pos relative to exisiting (i.e. same).
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const ");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : UlamRef<EC>(r, 0u, r.GetLen(), r.GetEffectiveSelf()) { }\n");

    //read 'entire quark' method
    genUlamTypeReadDefinitionForC(fp);

    //write 'entire quark' method
    genUlamTypeWriteDefinitionForC(fp);

    // aref/aset calls generated inline for immediates.
    if(isCustomArray())
      {
	m_state.indent(fp);
	fp->write("/* a custom array, btw ('Us' has aref, aset methods) */\n");
      }

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
  } //genUlamTypeQuarkMangledAutoDefinitionForC

  void UlamTypeClassQuark::genUlamTypeReadDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class read method
    if(!isScalar())
      {
	//class instance idx is always the scalar uti
	UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
	//reads an item of array
	//2nd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" readArrayItem(");
	fp->write("const u32 index, const u32 itemlen) const { return "); //was const after )
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //const ref, rel offset
	fp->write("itemlen, &");  //itemlen,
	fp->write(scalarmangledName.c_str()); //primitive effself
	fp->write("<EC>::THE_INSTANCE).Read(); }\n");
      }
  } //genUlamTypeReadDefinitionForC

  void UlamTypeClassQuark::genUlamTypeWriteDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class write method
    if(!isScalar())
      {
	//class instance idx is always the scalar uti
	UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
	const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
	// writes an item of array
	//3rd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("void writeArrayItem(const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" v, const u32 index, const u32 itemlen) { ");
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //rel offset
	fp->write("itemlen, &");  //itemlen,
	fp->write(scalarmangledName.c_str()); //primitive effself
	fp->write("<EC>::THE_INSTANCE).Write(v); }\n");
      }
  } //genUlamTypeQuarkWriteDefinitionForC

  //builds the immediate definition, inheriting from auto
  void UlamTypeClassQuark::genUlamTypeMangledDefinitionForC(File * fp)
  {
    s32 len = getTotalBitSize(); //could be 0, includes arrays
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayDefinitionForC(fp);

    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();

    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.m_currentIndentLevel = 0;

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
    //let's see if gcc can make use of these extra template args for immediate primitives
    fp->write("UlamRefFixed");
    fp->write("<EC, 0u, "); //left-just pos
    fp->write_decimal_unsigned(len);
    fp->write("u>\n");

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

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed"); //was atomicparametertype
    fp->write("<EC, 0u, "); //BITSPERATOM
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u> Up_Us;\n");

    //storage here in atom
    m_state.indent(fp);
    fp->write("T m_stg;  //storage here!\n");

    //default constructor (used by local vars)
    //(unlike element) call build default in case of initialized data members
    u32 dqval = 0;
    bool hasDQ = genUlamTypeDefaultQuarkConstant(fp, dqval);

    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), ");
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    if(hasDQ)
      {
	if(isScalar())
	  {
	    fp->write("Up_Us::Write(DEFAULT_QUARK);");
	  }
	else
	  {
	    //very packed array
	    if(len <= MAXBITSPERINT)
	      {
		u32 bitsize = getBitSize();
		u32 dqarrval = 0;
		u32 pos = 0;
		u32 mask = _GetNOnes32((u32) bitsize);
		u32 arraysize = getArraySize();
		dqval &= mask;
		//similar to build default quark value in NodeVarDeclDM
		for(u32 j = 1; j <= arraysize; j++)
		  {
		    dqarrval |= (dqval << (len - (pos + (j * bitsize))));
		  }

		std::ostringstream qdhex;
		qdhex << "0x" << std::hex << dqarrval;
		fp->write("Up_Us::Write(");
		fp->write(qdhex.str().c_str());
		fp->write(");");
	      }
	    else
	      {
		fp->write("u32 n = ");
		fp->write_decimal(getArraySize());
		fp->write("u; while(n--) { ");
		fp->write("writeArrayItem(DEFAULT_QUARK, n, ");
		fp->write_decimal_unsigned(getBitSize());
		fp->write("u); }");
	      }
	  }
      } //hasDQ
    fp->write(" }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" d) : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    fp->write("Up_Us::Write(d); }\n");

    // assignment constructor
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(mangledName.c_str());
    fp->write("<EC> & arg) : ");
    fp->write("Up_Us(m_stg, &");
    fp->write("Us::THE_INSTANCE), "); //effself
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { "); //for immediate quarks
    fp->write("Up_Us::Write(arg.Read()); }\n");

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
  } //genUlamTypeQuarkMangledDefinitionForC

  //similar to ulamtype primitives, except left-justified per item
  void UlamTypeClassQuark::genUlamTypeMangledUnpackedArrayAutoDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
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

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC");
    fp->write("> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC"); //BITSPERATOM
    fp->write(", 0u, "); //left-just
    fp->write_decimal_unsigned(itemlen); //includes arraysize
    fp->write("u > Up_Us;\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("T (& m_stgarrayref)[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u];  //ref to BIG storage!\n\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : m_stgarrayref(r.m_stgarrayref) { }\n");

    //constructor here (used by tmpVars)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(");
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
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("getBits(index).Read(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("getBits(index).Write(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T&, Or UlamRef??? GetStorage?
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + T::ATOM_FIRST_STATE_BIT"); //left-justified per item
    fp->write("); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(T::ATOM_FIRST_STATE_BIT");  //left-justified per item
    fp->write("); }\n");

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

  //similar to ulamtype primitives, except left-justified per item
  void UlamTypeClassQuark::genUlamTypeMangledUnpackedArrayDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    //class instance idx is always the scalar uti
    UTI scalaruti =  m_key.getUlamKeyTypeSignatureClassInstanceIdx();
    const std::string scalarmangledName = m_state.getUlamTypeByIndex(scalaruti)->getUlamTypeMangledName();

    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
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

    //quark typedef
    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(scalarmangledName.c_str());
    fp->write("<EC");
    fp->write("> Us;\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC"); //BITSPERATOM
    fp->write(", 0u, "); //left-just
    fp->write_decimal_unsigned(itemlen); //includes arraysize
    fp->write("u > Up_Us;\n");

    //Unpacked, storage reference T (&) [N]
    m_state.indent(fp);
    fp->write("typedef T TARR[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("];\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("TARR m_stgarr;  //BIG storage here!\n\n");


    u32 dqval = 0;
    bool hasDQ = genUlamTypeDefaultQuarkConstant(fp, dqval);

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("m_stgarr[j].SetUndefinedImpl();"); //T::ATOM_UNDEFINED_TYPE
    if(hasDQ)
      {
	fp->write("writeArrayItem(j, ");
	fp->write_decimal_unsigned(itemlen);
	fp->write("u, DEFAULT_QUARK);");
      }
    fp->write(" } }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //u32
    fp->write(" d) { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("m_stgarr[j].SetUndefinedImpl(); "); //T::ATOM_UNDEFINED_TYPE
    fp->write("writeArrayItem(j, ");
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, d); } }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) const { return ");
    fp->write("getBits(index).Read(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("getBits(index).Write(T::ATOM_FIRST_STATE_BIT, "); //left-justified per item
    fp->write_decimal_unsigned(itemlen);
    fp->write("u, v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write("getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T&, Or UlamRef??? GetStorage?
    m_state.indent(fp);
    fp->write("T& ");
    fp->write("getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + T::ATOM_FIRST_STATE_BIT"); //left-justified per item
    fp->write("); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write("getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(T::ATOM_FIRST_STATE_BIT");  //left-justified per item
    fp->write("); }\n");

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

  bool UlamTypeClassQuark::genUlamTypeDefaultQuarkConstant(File * fp, u32& dqref)
  {
    bool rtnb = false;
    //always the scalar.
    if(m_state.getDefaultQuark(m_key.getUlamKeyTypeSignatureClassInstanceIdx(), dqref))
      {
	std::ostringstream qdhex;
	qdhex << "0x" << std::hex << dqref;

	m_state.indent(fp);
	fp->write("static const u32 DEFAULT_QUARK = ");
	fp->write(qdhex.str().c_str());
	fp->write("; //=");
	fp->write_decimal_unsigned(dqref);
	fp->write("u\n\n");
	rtnb = true;
      }
    return rtnb;
  } //genUlamTypeDefaultQuarkConstant

} //end MFM