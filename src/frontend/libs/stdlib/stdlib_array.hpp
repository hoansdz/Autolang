#ifndef STDLIB_ARRAY_HPP
#define STDLIB_ARRAY_HPP

#define STDLIB_ARRAY_SOURCE STDLIB_ARRAY_SOURCE_STR
inline constexpr const char* STDLIB_ARRAY_SOURCE_STR = R"###(
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

	@native("arr_remove_element")
	fun remove(element: T): Bool

	@native("arr_remove_at")
	fun removeAt(index: Int): T

	@native("arr_remove_at")
	fun erase(index: Int): T

	@native("arr_add_all")
	fun addAll(elements: Array<T>): Bool

	@native("arr_remove_all")
	fun removeAll(elements: Array<T>): Bool

	@native("arr_retain_all")
	fun retainAll(elements: Array<T>): Bool

	@native("arr_contains_all")
	fun containsAll(elements: Array<T>): Bool

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

	@native("arr_for_each_indexed")
	fun forEachIndexed(fn: (Int, T) -> Void)

	@native("arr_on_each")
	fun onEach(action: (T) -> Void): Array<T>

	@native("arr_on_each_indexed")
	fun onEachIndexed(action: (Int, T) -> Void): Array<T>

	@native("arr_filter")
	fun filter(fn: (T) -> Bool): Array<T>

	@native("arr_filter")
	fun where(fn: (T) -> Bool): Array<T>



	@native("arr_sort_default")
	fun sort()

	@native("arr_sort")
	fun sort(comparator: (T, T) -> Int)

	@native("arr_clone")
	fun clone(): Array<T>

	@native("arr_clone")
	fun toList(): Array<T>

	@native("arr_clone")
	fun toMutableList(): Array<T>

	@native("arr_clone")
	fun toTypedArray(): Array<T>

	@native("arr_sorted")
	fun sorted(): Array<T>

	@native("arr_map")
	fun <R> map(fn: (T) -> R): Array<R>

	@native("arr_map_indexed")
	fun <R> mapIndexed(transform: (Int, T) -> R): Array<R>

	@native("arr_first")
	fun first(): T

	@native("arr_first_or_null")
	fun firstOrNull(): T?

	@native("arr_last")
	fun last(): T

	@native("arr_last_or_null")
	fun lastOrNull(): T?

	@native("arr_size")
	fun count(): Int

	@native("arr_count_fn")
	fun count(fn: (T) -> Bool): Int

	@native("arr_any")
	fun any(): Bool

	@native("arr_any_fn")
	fun any(fn: (T) -> Bool): Bool

	@native("arr_all_fn")
	fun all(fn: (T) -> Bool): Bool

	@native("arr_none")
	fun none(): Bool

	@native("arr_none_fn")
	fun none(fn: (T) -> Bool): Bool

	@native("arr_join_to_string")
	fun joinToString(separator: String = ", ", prefix: String = "", postfix: String = "", limit: Int = -1, truncated: String = "..."): String

	@native("arr_reversed")
	fun reversed(): Array<T>

	@native("arr_take")
	fun take(n: Int): Array<T>

	@native("arr_drop")
	fun drop(n: Int): Array<T>

	@native("arr_reduce")
	fun reduce(operation: (T, T) -> T): T

	@native("arr_reduce")
	fun reduce(operation: (T, T) -> T, initial: T): T

	@native("arr_fold")
	fun <R> fold(initial: R, operation: (R, T) -> R): R

	@native("arr_sum")
	fun sum(): Int

	@native("arr_sum_of")
	fun <R> sumOf(selector: (T) -> R): R

	@native("arr_plus")
	operator fun plus(other: Array<T>): Array<T>

	@native("arr_plus")
	fun plus(element: T): Array<T>

	@native("arr_plus")
	fun plusElement(element: T): Array<T>

	@native("arr_minus")
	fun minus(element: T): Array<T>

	@native("arr_minus")
	fun minusElement(element: T): Array<T>

	@native("arr_minus")
	operator fun minus(elements: Array<T>): Array<T>

	@native("arr_slice")
	fun slice(from: Int, to: Int): Array<T>

	@native("arr_slice")
	fun subList(from: Int, to: Int): Array<T>



	@native("arr_index_of")
	fun indexOf(value: T): Int

	@native("arr_index_of")
	fun findIndex(value: T): Int

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

	@native("arr_average")
	fun average(): Float

	@native("arr_max_or_null")
	fun maxOrNull(): T?

	@native("arr_min_or_null")
	fun minOrNull(): T?

	@native("arr_max_or_null")
	fun max(): T

	@native("arr_min_or_null")
	fun min(): T

	@native("arr_find")
	fun find(predicate: (T) -> Bool): T?

	@native("arr_find_last")
	fun findLast(predicate: (T) -> Bool): T?

	@native("arr_filter_not")
	fun filterNot(predicate: (T) -> Bool): Array<T>

	@native("arr_filter_not_null")
	fun filterNotNull(): Array<T>

	@native("arr_distinct")
	fun distinct(): Array<T>

	@native("arr_take_last")
	fun takeLast(n: Int): Array<T>

	@native("arr_drop_last")
	fun dropLast(n: Int): Array<T>

	@native("arr_take_while")
	fun takeWhile(predicate: (T) -> Bool): Array<T>

	@native("arr_drop_while")
	fun dropWhile(predicate: (T) -> Bool): Array<T>

	@native("arr_chunked")
	fun chunked(size: Int): Array<Any>

	@native("arr_to_set")
	fun toSet(): Set<T>

	@native("arr_to_set")
	fun toMutableSet(): Set<T>

	@native("arr_to_set")
	fun toHashSet(): Set<T>

	@native("arr_is_not_empty")
	fun isNotEmpty(): Bool

	@native("arr_get_or_null")
	fun getOrNull(index: Int): T?

	@native("arr_get")
	fun elementAt(index: Int): T

	@native("arr_get_or_null")
	fun elementAtOrNull(index: Int): T?

	@native("arr_get_or_else")
	fun getOrElse(index: Int, defaultValue: (Int) -> T): T

	@native("arr_find")
	fun firstOrNull(predicate: (T) -> Bool): T?

	@native("arr_find_last")
	fun lastOrNull(predicate: (T) -> Bool): T?

	@native("arr_sorted_descending")
	fun sortedDescending(): Array<T>

	@native("arr_index_of_first")
	fun indexOfFirst(predicate: (T) -> Bool): Int

	@native("arr_index_of_last")
	fun indexOfLast(predicate: (T) -> Bool): Int

	@native("arr_single")
	fun single(): T

	@native("arr_single_or_null")
	fun singleOrNull(): T?

	@native("arr_sorted_by")
	fun <R> sortedBy(selector: (T) -> R): Array<T>

	@native("arr_sorted_by_descending")
	fun <R> sortedByDescending(selector: (T) -> R): Array<T>

	@native("arr_then_by")
	fun <R> thenBy(selector: (T) -> R): Array<T>

	@native("arr_then_by_descending")
	fun <R> thenByDescending(selector: (T) -> R): Array<T>

	@native("arr_sorted_with")
	fun sortedWith(comparator: (T, T) -> Int): Array<T>

	@native("arr_sorted_with")
	fun sortedWith(comparator: Any): Array<T>

	@native("arr_sort_with")
	fun sortWith(comparator: (T, T) -> Int)

	@native("arr_sort_with")
	fun sortWith(comparator: Any)

	@native("arr_min_by_or_null")
	fun <R> minByOrNull(selector: (T) -> R): T?

	@native("arr_max_by_or_null")
	fun <R> maxByOrNull(selector: (T) -> R): T?

	@native("arr_min_by_or_null")
	fun <R> minBy(selector: (T) -> R): T?

	@native("arr_max_by_or_null")
	fun <R> maxBy(selector: (T) -> R): T?

	fun <R> maxOf(selector: (T) -> R): R = this.map<R>(selector).max()

	fun <R> minOf(selector: (T) -> R): R = this.map<R>(selector).min()

	fun <R> maxOfOrNull(selector: (T) -> R): R? = this.map<R>(selector).maxOrNull()

	fun <R> minOfOrNull(selector: (T) -> R): R? = this.map<R>(selector).minOrNull()

	@native("arr_distinct_by")
	fun <K> distinctBy(selector: (T) -> K): Array<T>

	@native("arr_shuffled")
	fun shuffled(): Array<T>

	@native("arr_flatten")
	fun flatten(): Array<Any>

	@native("arr_group_by")
	fun <K> groupBy(keySelector: (T) -> K): Map<K, Array<T>>

	@native("arr_associate_by")
	fun <K> associateBy(keySelector: (T) -> K): Map<K, T>

	@native("arr_flat_map")
	fun flatMap(transform: (T) -> Any): Array<Any>

	@native("arr_zip")
	fun zip(other: Array<Any>): Array<Any>

	@native("arr_unzip")
	fun unzip(): Any

	@native("arr_windowed")
	fun windowed(size: Int, step: Int = 1, partialWindows: Bool = false): Array<Any>

	@native("arr_partition")
	fun partition(predicate: (T) -> Bool): Pair<Array<T>, Array<T>>

	@native("arr_associate")
	fun <K, V> associate(transform: (T) -> Pair<K, V>): Map<K, V>

	@native("arr_associate_with")
	fun <V> associateWith(valueSelector: (T) -> V): Map<T, V>

	@native("arr_map_not_null")
	fun <R> mapNotNull(transform: (T) -> Any): Array<R>

	@native("arr_map_indexed_not_null")
	fun <R> mapIndexedNotNull(transform: (Int, T) -> Any): Array<R>

	@native("arr_filter_indexed")
	fun filterIndexed(predicate: (Int, T) -> Bool): Array<T>

	@native("arr_first_fn")
	fun first(predicate: (T) -> Bool): T

	@native("arr_last_fn")
	fun last(predicate: (T) -> Bool): T

	@native("arr_single_fn")
	fun single(predicate: (T) -> Bool): T

	@native("arr_single_or_null_fn")
	fun singleOrNull(predicate: (T) -> Bool): T?

	@native("arr_last_index_of")
	fun lastIndexOf(element: T): Int

	@native("arr_indices")
	fun indices(): Array<Int>

	@native("arr_last_index")
	fun lastIndex(): Int
}
)###";

#endif
