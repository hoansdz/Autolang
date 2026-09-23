#pragma once

#define STDLIB_SET_SOURCE STDLIB_SET_SOURCE_STR
inline constexpr const char* STDLIB_SET_SOURCE_STR = R"###(
@no_extends
@no_constructor
class Set<T> {
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

    @native("set_to_array")
    fun toMutableList(): Array<T>

	@native("set_union")
    fun union(other: Set<T>): Set<T>



    @native("set_intersect")
    fun intersect(other: Set<T>): Set<T>

    @native("set_intersect")
    fun intersection(other: Set<T>): Set<T>

    @native("set_difference")
    fun difference(other: Set<T>): Set<T>

	@native("set_union")
	operator fun plus(other: Set<T>): Set<T>

	@native("set_difference")
	operator fun minus(other: Set<T>): Set<T>

	@native("set_is_not_empty")
	fun isNotEmpty(): Bool

	@native("set_plus_element")
	fun plus(element: T): Set<T>

	@native("set_plus_element")
	fun plusElement(element: T): Set<T>

	@native("set_minus_element")
	fun minus(element: T): Set<T>

	@native("set_minus_element")
	fun minusElement(element: T): Set<T>

	@native("set_contains")
	fun includes(value: T): Bool

	@native("set_clone")
	fun clone(): Set<T>

	@native("set_clone")
	fun toSet(): Set<T>

	@native("set_clone")
	fun toMutableSet(): Set<T>

	@native("set_clone")
	fun toHashSet(): Set<T>

	@native("set_size")
	fun count(): Int

	@native("set_filter")
	fun filter(fn: (T) -> Bool): Set<T>

	fun filterNot(predicate: (T) -> Bool): Set<T> = this.toArray().filterNot(predicate).toSet()

	fun filterNotNull(): Set<T> = this.toArray().filterNotNull().toSet()

	fun distinct(): Set<T> = this

	@native("set_map")
	fun <R> map(fn: (T) -> R): Array<R>

	@native("set_first")
	fun first(): T

	@native("set_first_or_null")
	fun firstOrNull(): T?

	@native("set_any")
	fun any(): Bool

	@native("set_any_fn")
	fun any(fn: (T) -> Bool): Bool

	@native("set_all_fn")
	fun all(fn: (T) -> Bool): Bool

	@native("set_none")
	fun none(): Bool

	@native("set_none_fn")
	fun none(fn: (T) -> Bool): Bool

	@native("set_to_string")
	fun toString(): String

	@native("set_subtract")
	fun subtract(other: Set<T>): Set<T>

	@native("set_join_to_string")
	fun joinToString(separator: String = ", ", prefix: String = "", postfix: String = "", limit: Int = -1, truncated: String = "..."): String

	@native("set_contains_all")
	fun containsAll(elements: Set<T>): Bool

	@native("set_add_all")
	fun addAll(elements: Set<T>): Bool

	@native("set_remove_all")
	fun removeAll(elements: Set<T>): Bool

	@native("set_retain_all")
	fun retainAll(elements: Set<T>): Bool

	@native("set_count_fn")
	fun count(predicate: (T) -> Bool): Int

	@native("set_sum_of")
	fun sumOf(selector: (T) -> Int): Int

	@native("set_max_or_null")
	fun maxOrNull(): T?

	@native("set_max_or_null")
	fun max(): T

	@native("set_min_or_null")
	fun minOrNull(): T?

	@native("set_min_or_null")
	fun min(): T

	fun sorted(): Array<T> = this.toArray().sorted()

	fun sortedDescending(): Array<T> = this.toArray().sortedDescending()

	fun sum(): Int = this.toArray().sum()

	fun average(): Float = this.toArray().average()

	fun <R> maxOf(selector: (T) -> R): R = this.map<R>(selector).max()

	fun <R> minOf(selector: (T) -> R): R = this.map<R>(selector).min()

	fun <R> maxOfOrNull(selector: (T) -> R): R? = this.map<R>(selector).maxOrNull()

	fun <R> minOfOrNull(selector: (T) -> R): R? = this.map<R>(selector).minOrNull()

	@native("set_group_by")
	fun <K> groupBy(keySelector: (T) -> K): Map<K, Array<T>>

	@native("set_associate_by")
	fun <K> associateBy(keySelector: (T) -> K): Map<K, T>
}
)###";
