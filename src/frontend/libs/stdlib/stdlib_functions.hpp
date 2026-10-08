#pragma once

#define STDLIB_FUNCTIONS_SOURCE STDLIB_FUNCTIONS_SOURCE_STR
inline constexpr const char* STDLIB_FUNCTIONS_SOURCE_STR = R"###(
@native("print")
fun print(value: Any? = "")
@native("println")
fun println(value: Any? = "")
@native("get_refcount")
fun getRefCount(value: Any?): Int
@native("assert")
fun assert(condition: Bool, message: String = "Assertion failed")
@native("assert")
fun assert(condition: Bool, fileName: String, line: Int)



@native("repeat")
fun repeat(times: Int, action: (Int) -> Void)

@native("require")
fun require(condition: Bool, message: String = "Requirement failed.")

@native("range")
fun range(stop: Int): Array<Int>
@native("range")
fun range(start: Int, stop: Int, step: Int = 1): Array<Int>

@native("check")
fun check(condition: Bool, message: String = "Check failed.")

@native("require_not_null")
fun <T> requireNotNull(value: T?, message: String = "Required value was null."): T

@native("check_not_null")
fun <T> checkNotNull(value: T?, message: String = "Required value was null."): T

@native("error")
fun error(message: String)

@native("min_of")
fun minOf(a: Int, b: Int): Int
@native("min_of")
fun minOf(a: Float, b: Float): Float
@native("min_of")
fun minOf(a: Int, b: Float): Float
@native("min_of")
fun minOf(a: Float, b: Int): Float
@native("min_of")
fun minOf(a: Int, b: Int, c: Int): Int
@native("min_of")
fun minOf(a: Float, b: Float, c: Float): Float

@native("max_of")
fun maxOf(a: Int, b: Int): Int
@native("max_of")
fun maxOf(a: Float, b: Float): Float
@native("max_of")
fun maxOf(a: Int, b: Float): Float
@native("max_of")
fun maxOf(a: Float, b: Int): Float
@native("max_of")
fun maxOf(a: Int, b: Int, c: Int): Int
@native("max_of")
fun maxOf(a: Float, b: Float, c: Float): Float

@native("clamp")
fun clamp(value: Int, minVal: Int, maxVal: Int): Int
@native("clamp")
fun clamp(value: Float, minVal: Float, maxVal: Float): Float




@native("todo")
fun TODO(reason: String = "An operation is not implemented.")



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

// Character
typealias Character = Char

// String aliases
typealias Str = String

// Array/List aliases
typealias List<T> = Array<T>
typealias ArrayList<T> = Array<T>
typealias LinkedList<T> = Array<T>
typealias Vector<T> = Array<T>
typealias Vec<T> = Array<T>
typealias MutableList<T> = Array<T>
typealias mutableListOf<T> = Array<T>
typealias listOf<T> = Array<T>
typealias arrayOf<T> = Array<T>
typealias arrayListOf<T> = Array<T>
typealias emptyList<T> = Array<T>
typealias emptyArray<T> = Array<T>
typealias listOfNotNull<T> = Array<T>
typealias arrayOfNotNull<T> = Array<T>
typealias mutableListOfNotNull<T> = Array<T>

// Map/Dictionary aliases
typealias HashMap<K, V> = Map<K, V>
typealias Dictionary<K, V> = Map<K, V>
typealias Dict<K, V> = Map<K, V>
typealias TreeMap<K, V> = Map<K, V>
typealias MutableMap<K, V> = Map<K, V>
typealias mapOf<K, V> = Map<K, V>
typealias mutableMapOf<K, V> = Map<K, V>
typealias hashMapOf<K, V> = Map<K, V>
typealias linkedMapOf<K, V> = Map<K, V>
typealias emptyMap<K, V> = Map<K, V>

// Set aliases
typealias HashSet<T> = Set<T>
typealias TreeSet<T> = Set<T>
typealias MutableSet<T> = Set<T>
typealias setOf<T> = Set<T>
typealias mutableSetOf<T> = Set<T>
typealias hashSetOf<T> = Set<T>
typealias linkedSetOf<T> = Set<T>
typealias emptySet<T> = Set<T>
typealias setOfNotNull<T> = Set<T>
typealias mutableSetOfNotNull<T> = Set<T>

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
typealias char = Char
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
typealias ByteArray = Bytes
typealias CharArray = Array<Char>
typealias LongArray = Array<Int>

typealias charArrayOf = Array<Char>

// Small numeric types (Kotlin)
typealias Byte = Int
typealias Short = Int

// Collection interfaces (Kotlin)
typealias Collection<T> = Array<T>
typealias Iterable<T> = Array<T>

// Special types (Kotlin)
typealias Nothing = Void

// Reference types for closure variable capturing (Kotlin)
typealias IntRef = Ref<Int>
typealias FloatRef = Ref<Float>
typealias BooleanRef = Ref<Bool>
typealias StringRef = Ref<String>

// Scope builders (Kotlin)
@native("build_list")
fun <T> buildList(builderAction: (Array<T>) -> Void): Array<T>

@native("build_map")
fun <K, V> buildMap(builderAction: (Map<K, V>) -> Void): Map<K, V>

@native("build_set")
fun <T> buildSet(builderAction: (Set<T>) -> Void): Set<T>

class StringBuilder(var content: String = "") {
	@native("string_builder_append")
	fun append(value: Any?): StringBuilder
	@native("string_builder_append")
	operator fun plus(value: Any?): StringBuilder
	@native("string_builder_append_line")
	fun appendLine(value: Any? = ""): StringBuilder
	@native("string_builder_append_range")
	fun appendRange(value: String, startIndex: Int, endIndex: Int): StringBuilder
	@native("string_builder_length")
	fun length(): Int
	@native("string_builder_clear")
	fun clear(): StringBuilder
	@native("string_builder_to_string")
	fun toString(): String
	@native("string_builder_is_empty")
	fun isEmpty(): Bool
	@native("string_builder_is_not_empty")
	fun isNotEmpty(): Bool
	@native("string_builder_get")
	operator fun get(index: Int): Char
	@native("string_builder_set")
	operator fun set(index: Int, value: Char)
	@native("string_builder_set")
	fun setCharAt(index: Int, value: Char): StringBuilder
	@native("string_builder_last_index")
	fun lastIndex(): Int
	@native("string_builder_insert")
	fun insert(index: Int, value: Any?): StringBuilder
	@native("string_builder_delete")
	fun delete(startIndex: Int, endIndex: Int): StringBuilder
	@native("string_builder_delete_at")
	fun deleteAt(index: Int): StringBuilder
	@native("string_builder_delete_at")
	fun deleteCharAt(index: Int): StringBuilder
	@native("string_builder_replace")
	fun replace(startIndex: Int, endIndex: Int, value: String): StringBuilder
	@native("string_builder_reverse")
	fun reverse(): StringBuilder
	@native("string_builder_set_length")
	fun setLength(newLength: Int): StringBuilder
	@native("string_builder_substring")
	fun substring(startIndex: Int): String
	@native("string_builder_substring")
	fun substring(startIndex: Int, endIndex: Int): String
	@native("string_builder_index_of")
	fun indexOf(str: String, startIndex: Int = 0): Int
	@native("string_builder_index_of")
	fun indexOf(char: Char, startIndex: Int = 0): Int
	@native("string_builder_last_index_of")
	fun lastIndexOf(str: String, startIndex: Int = -1): Int
	@native("string_builder_last_index_of")
	fun lastIndexOf(char: Char, startIndex: Int = -1): Int
	@native("string_builder_contains")
	fun contains(str: String): Bool
	@native("string_builder_contains")
	fun contains(char: Char): Bool
	@native("string_builder_starts_with")
	fun startsWith(prefix: String): Bool
	@native("string_builder_ends_with")
	fun endsWith(suffix: String): Bool
}

@native("build_string")
fun buildString(builderAction: (StringBuilder) -> Void): String

@native("str_partition")
fun String.partition(predicate: (Char) -> Bool): Pair<String, String>

@native("str_zip")
fun String.zip(other: String): Array<Pair<Char, Char>>

@native("str_zip_transform")
fun <R> String.zip(other: String, transform: (Char, Char) -> R): Array<R>

@native("str_associate")
fun <K, V> String.associate(transform: (Char) -> Pair<K, V>): Map<K, V>

@native("str_associate_by")
fun <K> String.associateBy(keySelector: (Char) -> K): Map<K, Char>

@native("str_associate_with")
fun <V> String.associateWith(valueSelector: (Char) -> V): Map<Char, V>

@native("str_group_by")
fun <K> String.groupBy(keySelector: (Char) -> K): Map<K, Array<Char>>

@native("str_map")
fun <R> String.map(transform: (Char) -> R): Array<R>

@native("str_map_indexed")
fun <R> String.mapIndexed(transform: (Int, Char) -> R): Array<R>

@native("str_fold")
fun <R> String.fold(initial: R, operation: (R, Char) -> R): R

@native("str_running_fold")
fun <R> String.runningFold(initial: R, operation: (R, Char) -> R): Array<R>

@native("str_running_fold")
fun <R> String.scan(initial: R, operation: (R, Char) -> R): Array<R>

// Global Math Constants
val PI: Float = 3.141592653589793
val E: Float = 2.718281828459045

// Global Math Functions
@native("min_of")
fun min(a: Int, b: Int): Int
@native("min_of")
fun min(a: Float, b: Float): Float
@native("min_of")
fun min(a: Int, b: Float): Float
@native("min_of")
fun min(a: Float, b: Int): Float

@native("max_of")
fun max(a: Int, b: Int): Int
@native("max_of")
fun max(a: Float, b: Float): Float
@native("max_of")
fun max(a: Int, b: Float): Float
@native("max_of")
fun max(a: Float, b: Int): Float

@native("int_abs")
fun abs(value: Int): Int
@native("float_abs")
fun abs(value: Float): Float

@native("float_sqrt")
fun sqrt(value: Float): Float
@native("float_sqrt")
fun sqrt(value: Int): Float

@native("int_pow")
fun pow(base: Int, exp: Int): Int
@native("float_pow")
fun pow(base: Float, exp: Float): Float
@native("float_pow")
fun pow(base: Int, exp: Float): Float
@native("float_pow")
fun pow(base: Float, exp: Int): Float

@native("float_floor")
fun floor(value: Float): Float
@native("identity")
fun floor(value: Int): Int

@native("float_ceil")
fun ceil(value: Float): Float
@native("identity")
fun ceil(value: Int): Int

@native("float_round")
fun round(value: Float): Float
@native("identity")
fun round(value: Int): Int

// Global Math Class
class Math {
	static val PI: Float = 3.141592653589793
	static val E: Float = 2.718281828459045

	@native("min_of")
	static fun min(a: Int, b: Int): Int
	@native("min_of")
	static fun min(a: Float, b: Float): Float
	@native("min_of")
	static fun min(a: Int, b: Float): Float
	@native("min_of")
	static fun min(a: Float, b: Int): Float

	@native("max_of")
	static fun max(a: Int, b: Int): Int
	@native("max_of")
	static fun max(a: Float, b: Float): Float
	@native("max_of")
	static fun max(a: Int, b: Float): Float
	@native("max_of")
	static fun max(a: Float, b: Int): Float

	@native("int_abs")
	static fun abs(value: Int): Int
	@native("float_abs")
	static fun abs(value: Float): Float

	@native("float_floor")
	static fun floor(value: Float): Float
	@native("identity")
	static fun floor(value: Int): Int

	@native("float_ceil")
	static fun ceil(value: Float): Float
	@native("identity")
	static fun ceil(value: Int): Int

	@native("float_round")
	static fun round(value: Float): Float
	@native("identity")
	static fun round(value: Int): Int

	@native("float_sqrt")
	static fun sqrt(value: Float): Float
	@native("float_sqrt")
	static fun sqrt(value: Int): Float

	@native("int_pow")
	static fun pow(base: Int, exp: Int): Int
	@native("float_pow")
	static fun pow(base: Float, exp: Float): Float
	@native("float_pow")
	static fun pow(base: Int, exp: Float): Float
	@native("float_pow")
	static fun pow(base: Float, exp: Int): Float

	@native("int_sign")
	static fun sign(value: Int): Int
	@native("float_sign")
	static fun sign(value: Float): Float
}

typealias kotlin.math = Math

// Global Pythonic Conversions & Utilities
@native("str_to_int")
fun parseInt(value: String): Int

@native("str_to_int_or_null")
fun parseIntOrNull(value: String): Int?

@native("int_to_binary_string")
fun toBinaryString(value: Int): String

@native("str_to_float")
fun parseFloat(value: String): Float

@native("str_to_float_or_null")
fun parseFloatOrNull(value: String): Float?

@native("str_to_bool")
fun parseBool(value: String): Bool

@native("str_to_bool")
fun parseBoolean(value: String): Bool

@native("str_to_bool_or_null")
fun parseBoolOrNull(value: String): Bool?

@native("str_to_bool_or_null")
fun parseBooleanOrNull(value: String): Bool?

@native("str_to_int")
fun int(value: Any): Int

@native("str_to_float")
fun float(value: Any): Float

@native("to_string")
fun str(value: Any?): String

@native("int_to_char_string")
fun chr(code: Int): String

@native("char_code")
fun ord(char: Char): Int

@native("str_char_code")
fun ord(str: String): Int

@native("arr_size")
fun <T> len(array: Array<T>): Int

@native("arr_size")
fun len(array: Array<Any?>): Int

@native("string_size")
fun len(str: String): Int

@native("map_size")
fun <K, V> len(map: Map<K, V>): Int

@native("map_size")
fun len(map: Map<Any?, Any?>): Int

@native("set_size")
fun <T> len(set: Set<T>): Int

@native("set_size")
fun len(set: Set<Any?>): Int

@native("arr_sum")
fun sum(array: Array<Int>): Int

@native("arr_sum")
fun sum(array: Array<Float>): Float

@native("arr_sorted")
fun <T> sorted(array: Array<T>): Array<T>

@native("arr_with_index")
fun <T> enumerate(array: Array<T>): Array<IndexedValue<T>>

@native("arr_zip")
fun <T, R> zip(a: Array<T>, b: Array<R>): Array<Pair<T, R>>
)###";
