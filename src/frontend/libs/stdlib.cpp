#ifndef LIBS_STDLIB_CPP
#define LIBS_STDLIB_CPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "shared/DefaultOperator.hpp"
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
	static func String(): String
	
	@native("string_constructor")
	static func String(str: String): String

	@native("string_constructor")
	static func String(str: String, repeatTimes: Int): String

	@native("string_size")
	func size(): Int

	@native("str_is_empty")
	func isEmpty(): Bool

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

	@native("str_trim")
	func trim(): String

	@native("str_contains")
	func contains(sub: String): Bool

	@native("str_index_of")
	func indexOf(sub: String): Int

	@native("str_split")
	func split(delimiter: String, classId: Int = getClassId(Array<String>)): Array<String>

	@native("str_replace")
	func replace(old: String, new: String): String
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

@no_extends
@no_constructor
class Function {

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

	@native("arr_is_empty")
	func isEmpty(): Bool

	@native("arr_get")
	func get(index: Int): T

	@native("arr_set")
	func set(index: Int, value: T)

	@native("arr_clear")
	func clear()

	@native("arr_contains")
	func contains(value: T): Bool

	@native("arr_for_each")
	func forEach(fn: (T) -> Void)

	@native("arr_filter")
	func filter(fn: (T) -> Bool): Array<T>

	@native("arr_sort")
	func sort(comparator: (T, T) -> Int)

	@native("arr_slice")
	func slice(from: Int, to: Int): Array<T>

	@native("arr_index_of")
	func indexOf(value: T): Int

	@native("arr_reserve")
	func reserve(capacity: Int)

	@native("arr_pop")
	func pop(): T?

	@native("arr_insert")
	func insert(index: Int, value: T)

	@native("arr_to_string")
	func toString(): String
}

@no_extends
@no_constructor
class Set<T> {
	@native("set_constructor")
	static func __CLASS__(classId: Int = getClassId(Set<T>), keyId: Int = getClassId(T)): Set<T>

	@native("set_insert")
	func insert(value: T)

	@native("set_remove")
	func remove(value: T)

	@native("set_size")
	func size(): Int

	@native("set_contains")
	func contains(value: T): Bool

	@native("set_clear")
	func clear()

	@native("set_is_empty")
    func isEmpty(): Bool

    @native("set_for_each")
    func forEach(fn: (T) -> Void)

    @native("set_to_array")
    func toArray(classId: Int = getClassId(Array<T>)): Array<T>

	@native("set_union")
    func union(other: Set<T>): Set<T>

    @native("set_intersect")
    func intersect(other: Set<T>): Set<T>

    @native("set_difference")
    func difference(other: Set<T>): Set<T>
	
	@native("set_to_string")
	func toString(): String
}

@no_extends
@no_constructor
class Map<K, V> {
	@native("map_constructor")
	static func __CLASS__(classId: Int = getClassId(Map<K, V>), keyId: Int = getClassId(K)): Map<K, V>
	
	@native("map_get")
	func get(key: K): V?

	@native("map_get_or_default")
	func getOrDefault(key: K, defaultValue: V): V
	
	@native("map_set")
	func set(key: K, value: V)

	@native("map_is_empty")
    func isEmpty(): Bool

    @native("map_contains_key")
    func containsKey(key: K): Bool

	@native("map_size")
	func size(): Int

	@native("map_for_each")
    func forEach(fn: (K, V) -> Void)

	@native("map_keys")
    func keys(classId: Int = getClassId(Array<K>)): Array<K>

	@native("map_values")
    func values(classId: Int = getClassId(Array<V>)): Array<V>

	@native("map_remove")
	func remove(key: K)

	@native("map_clear")
	func clear()

	@native("map_to_string")
	func toString(): String
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
// @wait_input
// @native("input")
// func input(): String
	)###",
	    true,
	    ANativeMap(
	        {{"string_constructor", &DefaultFunction::string_constructor},
	         {"print", &DefaultFunction::print},
	         {"println", &DefaultFunction::println},
	         {"get_refcount", &DefaultFunction::get_refcount},
	         {"str_to_int", &DefaultFunction::to_int},
	         {"str_to_float", &DefaultFunction::to_float},
	         {"str_contains", &DefaultFunction::str_contains},
	         {"str_split", &DefaultFunction::str_split},
	         {"str_trim", &DefaultFunction::str_trim},
	         {"str_is_empty", &DefaultFunction::str_is_empty},
	         {"str_replace", &DefaultFunction::str_replace},
	         {"str_index_of", &DefaultFunction::str_index_of},
	         {"to_string", &DefaultFunction::to_string},
	         {"str_get", &DefaultFunction::str_get},
	         {"str_substr", &DefaultFunction::str_substr},
	         {"input", &DefaultFunction::input_str},
	         {"arr_add", &array::add},
	         {"arr_remove", &array::remove},
	         {"arr_size", &array::size},
	         {"arr_get", &array::get},
	         {"arr_set", &array::set},
	         {"arr_insert", &array::insert},
	         {"arr_pop", &array::pop},
	         {"arr_filter", &array::filter},
	         {"arr_for_each", &array::for_each},
	         {"arr_index_of", &array::index_of},
	         {"arr_is_empty", &array::is_empty},
	         {"arr_slice", &array::slice},
	         {"arr_sort", &array::sort},
	         {"arr_reserve", &array::reserve},
	         {"arr_clear", &array::clear},
	         {"arr_contains", &array::contains},
	         {"arr_to_string", &array::to_string},
	         {"set_constructor", &set::constructor},
	         {"set_insert", &set::insert},
	         {"set_remove", &set::remove},
	         {"set_size", &set::size},
	         {"set_size", &set::size},
	         {"set_contains", &set::contains},
	         {"set_clear", &set::clear},
	         {"set_is_empty", &set::is_empty},
	         {"set_for_each", &set::for_each},
	         {"set_to_array", &set::to_array},
	         {"set_union", &set::set_union},
	         {"set_intersect", &set::intersect},
	         {"set_difference", &set::difference},
	         {"set_to_string", &set::to_string},
	         {"string_size", &DefaultFunction::get_string_size},
	         {"map_constructor", &map::constructor},
	         {"map_clear", &map::clear},
	         {"map_size", &map::size},
	         {"map_is_empty", &map::is_empty},
	         {"map_contains_key", &map::contains_key},
	         {"map_for_each", &map::for_each},
	         {"map_keys", &map::keys},
	         {"map_values", &map::values},
	         {"map_remove", &map::remove},
	         {"map_get", &map::get},
	         {"map_get_or_default", &map::get_or_default},
	         {"map_set", &map::set},
	         {"map_to_string", &map::to_string}}));
}
} // namespace stdlib
} // namespace Libs
} // namespace AutoLang
#endif