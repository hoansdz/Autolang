#ifndef LIBS_STDLIB_CPP
#define LIBS_STDLIB_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"

namespace AutoLang {
namespace Libs {
namespace stdlib {
void init(ACompiler &compiler) {
	compiler.registerFromSource(
	    "stdlib", R"###(
@no_extends
@no_constructor
class Int {
	@native("to_string")
	func toString(): String
}
@no_extends
@no_constructor
class Float {
	@native("to_string")
	func toString(): String
}
@no_extends
@no_constructor
class Bool {
	
}
@no_extends
@no_constructor
class String {
	@native("string_constructor")
	static func String(str: String): String
	@native("string_constructor")
	static func String(str: String, repeatTimes: Int): String
	@native("string_size")
	func size(): Int
	@native("str_to_int")
	func toInt(): Int
	@native("str_to_float")
	func toFloat(): Float
	@native("str_get")
	func [](position: Int): String
	@native("str_get")
	func get(position: Int): String
}
@no_extends
@no_constructor
class Null {

}
@no_extends
@no_constructor
class Any {

}
@no_extends
@no_constructor
class Void {

}
class Exception(val message: String) {
	
}
@native("print")
func print(value: Any?)
@native("println")
func println(value: Any?)
@native("get_refcount")
func getRefCount(value: Any?): Int
@wait_input
@native("input")
func input(): String
	)###",
	    true,
	    ANativeMap(
	        {{"string_constructor", &DefaultFunction::string_constructor},
	         {"print", &DefaultFunction::print},
	         {"println", &DefaultFunction::println},
	         {"get_refcount", &DefaultFunction::get_refcount},
	         {"str_to_int", &DefaultFunction::to_int},
	         {"str_to_float", &DefaultFunction::to_float},
	         {"to_string", &DefaultFunction::to_string},
	         {"str_get", &DefaultFunction::str_get},
	         {"input", &DefaultFunction::input_str},
	         {"string_size", &DefaultFunction::get_string_size}}));
}
} // namespace stdlib
} // namespace Libs
} // namespace AutoLang
#endif