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

// Reference types for closure variable capturing (Kotlin)
typealias IntRef = Ref<Int>
typealias FloatRef = Ref<Float>
typealias BooleanRef = Ref<Bool>
typealias StringRef = Ref<String>
)###";
