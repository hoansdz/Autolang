#ifndef LIBS_VM_CPP
#define LIBS_VM_CPP

#include "array.hpp"
#include "frontend/ACompiler.hpp"
#include <chrono>
#include <string>


namespace AutoLang {
class ACompiler;
namespace Libs {
namespace vm {

AObject *count_area_object(NativeFuncInData) {
	return notifier.createInt(
	    notifier.vm->data.manager.areaAllocator.countObject);
}

void init(ACompiler &compiler) {
	compiler.registerFromSource("std/vm", R"###(
@no_constructor
class VM {
	@native("count_area_object")
	static func getCountAreaAllocatorObject(): Int
}
	)###",
	                            true,
	                            ANativeMap({
	                                {"count_area_object", &count_area_object},
	                            }));
}

} // namespace vm
} // namespace Libs
} // namespace AutoLang

#endif
