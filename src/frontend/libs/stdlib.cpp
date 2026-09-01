#ifndef LIBS_STDLIB_CPP
#define LIBS_STDLIB_CPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/libs/bytes.hpp"
#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"

#ifdef __PYBIND11__
#define AUTOLANG_NATIVE_CLASS "@no_extends\n@no_constructor\n@py_object\nclass PyObject {}"
#elif __EMSCRIPTEN__
#define AUTOLANG_NATIVE_CLASS "@no_extends\n@no_constructor\n@js_object\nclass JsObject {}"
#endif

namespace Autolang {
namespace Libs {
namespace stdlib {
void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "stdlib", R"###(
@no_extends
@no_constructor
class Int {
	@native("to_string")
	fun toString(): String
}
@no_extends
@no_constructor
class Float {
	@native("to_string")
	fun toString(): String
}
@no_extends
@no_constructor
class Bool {
	@native("to_string")
	fun toString(): String
}
@no_extends
@no_constructor
class String {
	@native("string_constructor")
	static fun String(): String
	
	@native("string_constructor")
	static fun String(str: String): String

	@native("string_constructor")
	static fun String(str: String, repeatTimes: Int): String

	@native("string_size")
	fun size(): Int

	@native("string_size")
	fun length(): Int



	@native("str_is_empty")
	fun isEmpty(): Bool

	@native("str_is_empty")
	fun empty(): Bool

	@native("str_is_empty")
	fun is_empty(): Bool



	@native("str_to_int")
	fun toInt(): Int

	@native("str_to_int")
	fun parseInt(): Int



	@native("str_to_float")
	fun toFloat(): Float

	@native("str_to_float")
	fun parseFloat(): Float

	@native("str_to_float")
	fun toDouble(): Float



	@native("str_get")
	fun get(position: Int): String

	@native("str_get")
	fun at(position: Int): String

	// @native("str_set")
	// fun set(position: Int, chr: Int)

	// @native("str_set")
	// fun set(position: Int, str: String)

	@native("str_char_at")
	fun charAt(position: Int): Int

	@native("str_substr")
	fun substr(from: Int): String

	@native("str_substr")
	fun substring(from: Int): String

	@native("str_substr")
	fun slice(from: Int): String

	@native("str_substr")
	fun substr(from: Int, subLength: Int): String

	@native("str_substr")
	fun substring(from: Int, subLength: Int): String

	@native("str_substr")
	fun slice(from: Int, subLength: Int): String



	@native("str_trim")
	fun trim(): String

	@native("str_trim")
	fun strip(): String



	@native("str_contains")
	fun contains(sub: String): Bool

	@native("str_contains")
	fun includes(sub: String): Bool



	@native("str_index_of")
	fun indexOf(sub: String): Int

	@native("str_index_of")
	fun find(sub: String): Int



	@native("str_split")
	fun split(delimiter: String): Array<String>



	@native("str_replace")
	fun replace(old: String, new: String): String

	@native("str_replace")
	fun replaceAll(old: String, new: String): String



	@native("str_starts_with")
    fun startsWith(prefix: String): Bool



    @native("str_ends_with")
    fun endsWith(suffix: String): Bool



    @native("str_last_index_of")
    fun lastIndexOf(sub: String): Int

    @native("str_last_index_of")
    fun rfind(sub: String): Int



    @native("str_to_lower")
    fun toLowerCase(): String

    @native("str_to_lower")
    fun toLower(): String

    @native("str_to_lower")
    fun lower(): String



    @native("str_to_upper")
    fun toUpperCase(): String

    @native("str_to_upper")
    fun toUpper(): String

    @native("str_to_upper")
    fun upper(): String


}

@no_constructor
@no_extends
class Bytes {

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

@no_constructor
@no_extends
class Array<T> {

	static fun __CLASS__(): Array<T> = <T>[]

	@native("arr_add")
	fun add(value: T)

	@native("arr_add")
	fun append(value: T)

	@native("arr_add")
	fun push(value: T)

	@native("arr_add")
	fun push_back(value: T)

	@native("arr_remove")
	fun remove(index: Int)

	@native("arr_remove")
	fun removeAt(index: Int)

	@native("arr_remove")
	fun erase(index: Int)

	@native("arr_size")
	fun size(): Int

	@native("arr_size")
	fun length(): Int

	@native("arr_size")
	fun len(): Int

	@native("arr_is_empty")
	fun isEmpty(): Bool

	@native("arr_is_empty")
	fun empty(): Bool

	@native("arr_is_empty")
	fun is_empty(): Bool

	@native("arr_get")
	fun get(index: Int): T

	@native("arr_get")
	fun at(index: Int): T



	@native("arr_set")
	fun set(index: Int, value: T)



	@native("arr_clear")
	fun clear()



	@native("arr_contains")
	fun contains(value: T): Bool

	@native("arr_contains")
	fun includes(value: T): Bool

	@native("arr_for_each")
	fun forEach(fn: (T) -> Void)

	@native("arr_for_each_with_index")
	fun forEach(fn: (T, Int) -> Void)

	@native("arr_for_each_with_index")
	fun forEachIndexed(fn: (T, Int) -> Void)

	@native("arr_filter")
	fun filter(fn: (T) -> Bool): Array<T>

	@native("arr_filter")
	fun where(fn: (T) -> Bool): Array<T>



	@native("arr_sort")
	fun sort(comparator: (T, T) -> Int)



	@native("arr_slice")
	fun slice(from: Int, to: Int): Array<T>

	@native("arr_slice")
	fun subList(from: Int, to: Int): Array<T>



	@native("arr_index_of")
	fun indexOf(value: T): Int

	@native("arr_index_of")
	fun findIndex(value: T): Int

	@native("arr_index_of")
	fun find(value: T): Int

	@native("arr_reserve")
	fun reserve(capacity: Int)

	@native("arr_reserve")
	fun ensureCapacity(capacity: Int)

	@native("arr_pop")
	fun pop(): T?



	@native("arr_pop")
	fun pop_back(): T?

	@native("arr_insert")
	fun insert(index: Int, value: T)



	@native("arr_to_string")
	fun toString(): String
}

@no_extends
@no_constructor
class Set<T> {
	static fun __CLASS__(): Set<T> = <T>{}

	@native("set_add")
	fun add(value: T)

	@native("set_add")
	fun insert(value: T)



	@native("set_remove")
	fun remove(value: T)

	@native("set_remove")
	fun delete(value: T)

	@native("set_remove")
	fun erase(value: T)

	@native("set_remove")
	fun discard(value: T)

	@native("set_size")
	fun size(): Int



	@native("set_size")
	fun len(): Int

	@native("set_contains")
	fun contains(value: T): Bool

	@native("set_contains")
	fun has(value: T): Bool



	@native("set_clear")
	fun clear()



	@native("set_is_empty")
    fun isEmpty(): Bool

	@native("set_is_empty")
    fun empty(): Bool

	@native("set_is_empty")
    fun is_empty(): Bool

    @native("set_for_each")
    fun forEach(fn: (T) -> Void)



    @native("set_to_array")
    fun toArray(): Array<T>

    @native("set_to_array")
    fun toList(): Array<T>

	@native("set_union")
    fun union(other: Set<T>): Set<T>



    @native("set_intersect")
    fun intersect(other: Set<T>): Set<T>

    @native("set_intersect")
    fun intersection(other: Set<T>): Set<T>

    @native("set_difference")
    fun difference(other: Set<T>): Set<T>


	
	@native("set_to_string")
	fun toString(): String
}

@no_extends
@no_constructor
class Map<K, V> {
	static fun __CLASS__(): Map<K, V> = <K, V>{}
	
	@native("map_get")
	fun get(key: K): V?



	@native("map_get_or_default")
	fun getOrDefault(key: K, defaultValue: V): V


	
	@native("map_set")
	fun set(key: K, value: V)

	@native("map_set")
	fun put(key: K, value: V)



	@native("map_set")
	fun setItem(key: K, value: V)

	@native("map_is_empty")
    fun isEmpty(): Bool

	@native("map_is_empty")
    fun empty(): Bool

	@native("map_is_empty")
    fun is_empty(): Bool

    @native("map_contains_key")
    fun containsKey(key: K): Bool



	@native("map_size")
	fun size(): Int



	@native("map_size")
	fun len(): Int

	@native("map_for_each")
    fun forEach(fn: (K, V) -> Void)



	@native("map_keys")
    fun keys(): Array<K>

	@native("map_keys")
    fun keySet(): Array<K>



	@native("map_values")
    fun values(): Array<V>



	@native("map_remove")
	fun remove(key: K)

	@native("map_remove")
	fun delete(key: K)

	@native("map_remove")
	fun erase(key: K)



	@native("map_clear")
	fun clear()



	@native("map_to_string")
	fun toString(): String
}

@no_extends
@no_constructor
class Json {

}
)###"
#ifdef AUTOLANG_NATIVE_CLASS 
AUTOLANG_NATIVE_CLASS
#endif
R"###(
@native("print")
fun print(value: Any? = "")
@native("println")
fun println(value: Any? = "")
@native("get_refcount")
fun getRefCount(value: Any?): Int
@native("assert")
fun assert(condition: Bool, fileName: String, line: Int)
// @wait_input
// @native("input")
// fun input(): String

// AI Error Absorption Typealiases
// Absorb common type naming mistakes from other languages (Java, C#, Python, Kotlin, C++, Rust, Swift, TypeScript)

// Numeric types
typealias Double = Float
typealias Number = Float
typealias Integer = Int
typealias Long = Int

// Boolean
typealias Boolean = Bool

// Character (AutoLang converts 'c' to Int via ASCII code)
typealias Char = Int
typealias Character = Int

// String aliases
typealias Str = String

// Array/List aliases
typealias List<T> = Array<T>
typealias ArrayList<T> = Array<T>
typealias LinkedList<T> = Array<T>
typealias Vector<T> = Array<T>
typealias Vec<T> = Array<T>
typealias MutableList<T> = Array<T>

// Map/Dictionary aliases
typealias HashMap<K, V> = Map<K, V>
typealias Dictionary<K, V> = Map<K, V>
typealias Dict<K, V> = Map<K, V>
typealias TreeMap<K, V> = Map<K, V>
typealias MutableMap<K, V> = Map<K, V>

// Set aliases
typealias HashSet<T> = Set<T>
typealias TreeSet<T> = Set<T>
typealias MutableSet<T> = Set<T>

// Nullable
typealias Optional<T> = T?

// Special types
typealias Object = Any
typealias JSON = Json

// Lowercase primitive and common types (C++, C#, TS, Python)
typealias int = Int
typealias float = Float
typealias double = Float
typealias bool = Bool
typealias boolean = Bool
typealias char = Int
typealias string = String
typealias any = Any
typealias object = Any
typealias void = Void
/*typealias list<T> = Array<T>
typealias dict<K, V> = Map<K, V>
typealias set<T> = Set<T>*/

// Primitive Arrays (Kotlin)
typealias IntArray = Array<Int>
typealias FloatArray = Array<Float>
typealias DoubleArray = Array<Float>
typealias BooleanArray = Array<Bool>
typealias ByteArray = Array<Int>
typealias CharArray = Array<Int>
typealias LongArray = Array<Int>

// Small numeric types (Kotlin)
typealias Byte = Int
typealias Short = Int

// Collection interfaces (Kotlin)
typealias Collection<T> = Array<T>
typealias Iterable<T> = Array<T>

// Special types (Kotlin)
typealias Nothing = Void
	)###",
	    LibraryConfig(true),
	    ANativeMap(
	        {{"string_constructor", &DefaultFunction::string_constructor},
	         {"print", &DefaultFunction::print},
	         {"println", &DefaultFunction::println},
			 {"assert", &DefaultFunction::assert_},
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
	         //  {"str_set", &DefaultFunction::str_set},
	         {"str_char_at", &DefaultFunction::str_char_at},
	         {"str_substr", &DefaultFunction::str_substr},
	         {"str_starts_with", &DefaultFunction::str_starts_with},
	         {"str_ends_with", &DefaultFunction::str_ends_with},
	         {"str_last_index_of", &DefaultFunction::str_last_index_of},
	         {"str_to_lower", &DefaultFunction::str_to_lower},
	         {"str_to_upper", &DefaultFunction::str_to_upper},
	        //  {"input", &DefaultFunction::input_str},
	         {"arr_add", &array::add},
	         {"arr_remove", &array::remove},
	         {"arr_size", &array::size},
	         {"arr_get", &array::get},
	         {"arr_set", &array::set},
	         {"arr_insert", &array::insert},
	         {"arr_pop", &array::pop},
	         {"arr_filter", &array::filter},
	         {"arr_for_each", &array::for_each},
	         {"arr_for_each_with_index", &array::for_each_with_index},
	         {"arr_index_of", &array::index_of},
	         {"arr_is_empty", &array::is_empty},
	         {"arr_slice", &array::slice},
	         {"arr_sort", &array::sort},
	         {"arr_reserve", &array::reserve},
	         {"arr_clear", &array::clear},
	         {"arr_contains", &array::contains},
	         {"arr_to_string", &array::to_string},
	         {"set_add", &set::add},
	         {"set_remove", &set::remove},
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
} // namespace Autolang
#endif
