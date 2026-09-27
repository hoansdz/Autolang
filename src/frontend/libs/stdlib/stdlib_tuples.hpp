#pragma once

#define STDLIB_TUPLES_SOURCE STDLIB_TUPLES_SOURCE_STR
inline constexpr const char* STDLIB_TUPLES_SOURCE_STR = R"###(
@no_extends
@no_constructor
class Json {

}

class Pair<A, B>(val first: A, val second: B) {
	@native("pair_to_string")
	fun toString(): String

	@native("pair_to_list")
	fun toList(): Array<Any>

	@native("tuple_first")
	fun component1(): A
	@native("tuple_second")
	fun component2(): B
	@native("pair_copy")
	fun copy(first: A = this.first, second: B = this.second): Pair<A, B>
}

@native("pair_of")
fun <A, B> pairOf(first: A, second: B): Pair<A, B>

class Triple<A, B, C>(val first: A, val second: B, val third: C) {
	@native("triple_to_string")
	fun toString(): String

	@native("triple_to_list")
	fun toList(): Array<Any>

	@native("tuple_first")
	fun component1(): A
	@native("tuple_second")
	fun component2(): B
	@native("tuple_third")
	fun component3(): C
	@native("triple_copy")
	fun copy(first: A = this.first, second: B = this.second, third: C = this.third): Triple<A, B, C>
}

@native("triple_of")
fun <A, B, C> tripleOf(first: A, second: B, third: C): Triple<A, B, C>

class IndexedValue<T>(val index: Int, val value: T) {
	@native("tuple_first")
	fun component1(): Int
	@native("tuple_second")
	fun component2(): T
	@native("indexed_value_to_string")
	fun toString(): String
}

@native("indexed_value_of")
fun <T> indexedValueOf(index: Int, value: T): IndexedValue<T>

class Comparator<T>(val selectors: Array<Any>, val directions: Array<Int>) {
	@native("comparator_then_by")
	fun thenBy(selector: (T) -> Float): Comparator<T>

	@native("comparator_then_by_descending")
	fun thenByDescending(selector: (T) -> Float): Comparator<T>
}

@native("compare_by_1")
fun <T> compareBy(selector: (T) -> Float): Comparator<T>

@native("compare_by_2")
fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float): Comparator<T>

@native("compare_by_3")
fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float, s3: (T) -> Float): Comparator<T>

@native("compare_by_4")
fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float, s3: (T) -> Float, s4: (T) -> Float): Comparator<T>

@native("compare_by_descending")
fun <T> compareByDescending(selector: (T) -> Float): Comparator<T>

class Ref<T>(var value: T) {
	@native("to_string")
	fun toString(): String
}

@native("ref_create")
fun <T> ref(value: T): Ref<T>
@native("ref_create")
fun intRef(value: Int): Ref<Int>
@native("ref_create")
fun floatRef(value: Float): Ref<Float>
@native("ref_create")
fun booleanRef(value: Bool): Ref<Bool>
@native("ref_create")
fun stringRef(value: String): Ref<String>
)###";
