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

	fun component1(): A = this.first
	fun component2(): B = this.second
	fun copy(first: A = this.first, second: B = this.second): Pair<A, B> = Pair<A, B>(first, second)
}

fun <A, B> pairOf(first: A, second: B): Pair<A, B> {
	return Pair<A, B>(first, second)
}

class Triple<A, B, C>(val first: A, val second: B, val third: C) {
	@native("triple_to_string")
	fun toString(): String

	@native("triple_to_list")
	fun toList(): Array<Any>

	fun component1(): A = this.first
	fun component2(): B = this.second
	fun component3(): C = this.third
	fun copy(first: A = this.first, second: B = this.second, third: C = this.third): Triple<A, B, C> = Triple<A, B, C>(first, second, third)
}

fun <A, B, C> tripleOf(first: A, second: B, third: C): Triple<A, B, C> {
	return Triple<A, B, C>(first, second, third)
}

class Comparator<T>(val selectors: Array<Any>, val directions: Array<Int>) {
	fun thenBy(selector: (T) -> Float): Comparator<T> {
		this.selectors.add(selector)
		this.directions.add(1)
		return this
	}

	fun thenByDescending(selector: (T) -> Float): Comparator<T> {
		this.selectors.add(selector)
		this.directions.add(-1)
		return this
	}
}

fun <T> compareBy(selector: (T) -> Float): Comparator<T> {
	val selectors: Array<Any> = <Any>[]
	selectors.add(selector)
	val dirs: Array<Int> = <Int>[]
	dirs.add(1)
	return Comparator<T>(selectors, dirs)
}

fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float): Comparator<T> {
	val selectors: Array<Any> = <Any>[]
	selectors.add(s1)
	selectors.add(s2)
	val dirs: Array<Int> = <Int>[]
	dirs.add(1)
	dirs.add(1)
	return Comparator<T>(selectors, dirs)
}

fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float, s3: (T) -> Float): Comparator<T> {
	val selectors: Array<Any> = <Any>[]
	selectors.add(s1)
	selectors.add(s2)
	selectors.add(s3)
	val dirs: Array<Int> = <Int>[]
	dirs.add(1)
	dirs.add(1)
	dirs.add(1)
	return Comparator<T>(selectors, dirs)
}

fun <T> compareBy(s1: (T) -> Float, s2: (T) -> Float, s3: (T) -> Float, s4: (T) -> Float): Comparator<T> {
	val selectors: Array<Any> = <Any>[]
	selectors.add(s1)
	selectors.add(s2)
	selectors.add(s3)
	selectors.add(s4)
	val dirs: Array<Int> = <Int>[]
	dirs.add(1)
	dirs.add(1)
	dirs.add(1)
	dirs.add(1)
	return Comparator<T>(selectors, dirs)
}

fun <T> compareByDescending(selector: (T) -> Float): Comparator<T> {
	val selectors: Array<Any> = <Any>[]
	selectors.add(selector)
	val dirs: Array<Int> = <Int>[]
	dirs.add(-1)
	return Comparator<T>(selectors, dirs)
}

class Ref<T>(var value: T) {
	@native("to_string")
	fun toString(): String
}

fun <T> ref(value: T): Ref<T> = Ref(value)
fun intRef(value: Int): Ref<Int> = Ref(value)
fun floatRef(value: Float): Ref<Float> = Ref(value)
fun booleanRef(value: Bool): Ref<Bool> = Ref(value)
fun stringRef(value: String): Ref<String> = Ref(value)
)###";
