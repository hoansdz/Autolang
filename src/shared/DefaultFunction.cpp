#ifndef DEFAULT_FUNCTION_CPP
#define DEFAULT_FUNCTION_CPP

#include "shared/DefaultFunction.hpp"
#include "shared/DefaultClass.hpp"
#include "frontend/ACompiler.hpp"

namespace Autolang {
namespace DefaultFunction {

void init(ACompiler& compiler) {
	// compiler.registerBuiltInLibrary("stdlib", false, R"###(
	// 	@native("print")
	// 	fun print(value: Any?)
	// 	@native("println")
	// 	fun println(value: Any?)
	// 	@native("get_refcount")
	// 	fun getRefCount(value: Any?): Int
	// )###", ANativeMap({
	// 	{"print", &print},
	// 	{"println", &println},
	// 	{"get_refcount", &get_refcount}
	// }));
	// compile.registerFunction(nullptr, true, "print()",
	//                          new ClassId[1]{Autolang::DefaultClass::anyClassId},
	//                          {true}, &print);
	// compile.registerFunction(nullptr, true, "println()",
	//                          new ClassId[1]{Autolang::DefaultClass::anyClassId},
	//                          {true}, &println);
	// compile.registerFunction(nullptr, true, "getRefCount()",
	//                          new ClassId[1]{Autolang::DefaultClass::anyClassId},
	//                          {true}, Autolang::DefaultClass::intClassId, false,
	//                          &get_refcount);
	// auto integer = compile.classes[Autolang::DefaultClass::intClassId];
	// compile.registerFunction(integer, false, "toString()",
	//                          new ClassId[1]{Autolang::DefaultClass::intClassId},
	//                          {false}, Autolang::DefaultClass::stringClassId,
	//                          false, &to_string);
	// compile.registerFunction(integer, false, "toFloat()",
	//                          new ClassId[1]{Autolang::DefaultClass::intClassId},
	//                          {false}, Autolang::DefaultClass::floatClassId,
	//                          false, &Autolang::DefaultFunction::to_float);
	// auto Float = compile.classes[Autolang::DefaultClass::floatClassId];
	// compile.registerFunction(
	//     Float, false, "toInt()",
	//     new ClassId[1]{Autolang::DefaultClass::floatClassId}, {false},
	//     Autolang::DefaultClass::intClassId, false,
	//     &Autolang::DefaultFunction::to_int);
	// compile.registerFunction(
	//     Float, false, "toString()",
	//     new ClassId[1]{Autolang::DefaultClass::floatClassId}, {false},
	//     Autolang::DefaultClass::stringClassId, false,
	//     &Autolang::DefaultFunction::to_string);
	// auto string = compile.classes[Autolang::DefaultClass::stringClassId];
	// compile.registerFunction(
	//     string, false, "toInt()",
	//     new ClassId[1]{Autolang::DefaultClass::stringClassId}, {false},
	//     Autolang::DefaultClass::intClassId, false,
	//     &Autolang::DefaultFunction::to_int);
	// compile.registerFunction(
	//     string, false, "size()",
	//     new ClassId[1]{Autolang::DefaultClass::stringClassId}, {false},
	//     Autolang::DefaultClass::intClassId, false,
	//     &Autolang::DefaultFunction::get_string_size);
	// compile.registerFunction(
	//     string, true, "String()",
	//     new ClassId[2]{Autolang::DefaultClass::stringClassId,
	//                    Autolang::DefaultClass::stringClassId},
	//     {false, false}, Autolang::DefaultClass::stringClassId, false,
	//     &Autolang::DefaultFunction::string_constructor);
	// compile.registerFunction(
	//     string, true, "String()",
	//     new ClassId[3]{Autolang::DefaultClass::stringClassId,
	//                    Autolang::DefaultClass::stringClassId,
	//                    Autolang::DefaultClass::intClassId},
	//     {false, false, false}, Autolang::DefaultClass::stringClassId, false,
	//     &Autolang::DefaultFunction::string_constructor);
}
} // namespace DefaultFunction
} // namespace Autolang

#endif