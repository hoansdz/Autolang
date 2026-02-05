#ifndef LIBS_DEBUGGER_CPP
#define LIBS_DEBUGGER_CPP

#include "Debugger.hpp"
#include "frontend/ACompiler.hpp"

namespace AutoLang {
namespace Libs {
namespace Debugger {
void init(ACompiler &compiler) {
	compiler.registerFromSource(
	    "std/debugger", false, R"###(
		class Debugger {
			
		}
	)###",
	    ANativeMap(
	        {{"string_constructor", &DefaultFunction::string_constructor},
	         {"print", &DefaultFunction::print},
	         {"println", &DefaultFunction::println},
	         {"get_refcount", &DefaultFunction::get_refcount},
	         {"str_to_int", &DefaultFunction::to_int},
	         {"str_to_float", &DefaultFunction::to_float},
	         {"to_string", &DefaultFunction::to_string},
	         {"string_size", &DefaultFunction::get_string_size}}));
}
AObject *getClassName(NativeFuncInData) {
	
}

AObject *getClassId(NativeFuncInData) {

}
} // namespace Debugger
} // namespace Libs
} // namespace AutoLang

#endif