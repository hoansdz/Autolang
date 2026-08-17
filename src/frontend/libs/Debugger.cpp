#ifndef LIBS_DEBUGGER_CPP
#define LIBS_DEBUGGER_CPP

#include "Debugger.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultFunction.hpp"


namespace Autolang {
namespace Libs {
namespace Debugger {
void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/debugger", R"###(
		class Debugger {
			
		}
	)###",
	    LibraryConfig(),
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
	if (argSize > 0 && args[0]) {
		return notifier.createString(AString::from(notifier.getClassName(args[0]->type)));
	}
	return nullptr;
}

AObject *getClassId(NativeFuncInData) {
	if (argSize > 0 && args[0]) {
		return notifier.createInt(static_cast<int64_t>(args[0]->type));
	}
	return nullptr;
}
} // namespace Debugger
} // namespace Libs
} // namespace Autolang

#endif