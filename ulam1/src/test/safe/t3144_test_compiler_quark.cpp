#include "TestCase_EndToEndCompiler.h"

namespace MFM {

  BEGINTESTCASECOMPILER(t3144_test_compiler_quark)
  {
    std::string GetAnswerKey()
    {
      return std::string("Uq_Bar { Foo valoo( Int(7) val(0); );  <NOMAIN> }\nExit status: -1");
    }

    std::string PresetTest(FileManagerString * fms)
    {
      // notice the filename must match the class name including capitalization.
      bool rtn2 = fms->add("Bar.ulam","ulam 1;\n use Foo;\n quark Bar {\n Foo valoo;\n }\n");
      bool rtn1 = fms->add("Foo.ulam","ulam 1;\n quark Foo {\nInt(7) val;\n }\n");

      if(rtn2 && rtn1)
	return std::string("Bar.ulam");

      return std::string("");
    }
  }

  ENDTESTCASECOMPILER(t3144_test_compiler_quark)

} //end MFM


