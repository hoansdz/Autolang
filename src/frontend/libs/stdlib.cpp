#ifndef LIBS_STDLIB_CPP
#define LIBS_STDLIB_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/libs/array.hpp"
#include "frontend/libs/map.hpp"
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
	static func String(): String
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
	func get(position: Int): String
	@native("str_substr")
	func substr(from: Int): String
	@native("str_substr")
	func substr(from: Int, length: Int): String
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
@no_extends
class Array<T>() {
	@native("arr_add")
	func add(value: T)
	@native("arr_remove")
	func remove(index: Int)
	@native("arr_size")
	func size(): Int
	@native("arr_get")
	func get(index: Int): T
	@native("arr_set")
	func set(index: Int, value: T)
	@native("arr_clear")
	func clear()
}

@no_constructor
class Map<K, V> {
	@native("map_constructor")
	private static func create(keyId: Int): Map<K, V>
	
	static func __CLASS__() = create(getClassId(K))
	
	@native("map_get")
	func get(key: K): V?
	
	@native("map_set")
	func set(key: K, value: V)

	@native("map_size")
	func size(): Int

	@native("map_remove")
	func remove(key: K)

	@native("map_clear")
	func clear()
}
	
@native("print")
func print(value: Any?)
@native("println")
func println(value: Any?)
@native("get_refcount")
func getRefCount(value: Any?): Int
func assert(condition: Bool, fileName: String, line: Int) {
	if (condition) {
		return
	}
	throw Exception("${fileName}:${line}: Wrong")
}
@wait_input
@native("input")
func input(): String
	)###",
	    true,
	    ANativeMap({
	        {"string_constructor", &DefaultFunction::string_constructor},
	        {"print", &DefaultFunction::print},
	        {"println", &DefaultFunction::println},
	        {"get_refcount", &DefaultFunction::get_refcount},
	        {"str_to_int", &DefaultFunction::to_int},
	        {"str_to_float", &DefaultFunction::to_float},
	        {"to_string", &DefaultFunction::to_string},
	        {"str_get", &DefaultFunction::str_get},
	        {"str_substr", &DefaultFunction::str_substr},
	        {"input", &DefaultFunction::input_str},
	        {"arr_add", &array::add},
	        {"arr_remove", &array::remove},
	        {"arr_size", &array::size},
	        {"arr_get", &array::get},
	        {"arr_set", &array::set},
	        {"arr_clear", &array::clear},
	        {"string_size", &DefaultFunction::get_string_size},
	        {"map_constructor", &map::constructor},
	        {"map_clear", &map::clear},
	        {"map_size", &map::size},
	        {"map_remove", &map::remove},
	        {"map_get", &map::get},
	        {"map_set", &map::set},
	    }));
}
} // namespace stdlib
} // namespace Libs
} // namespace AutoLang
#endif