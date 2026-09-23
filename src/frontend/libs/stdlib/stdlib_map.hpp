#pragma once

#define STDLIB_MAP_SOURCE STDLIB_MAP_SOURCE_STR
inline constexpr const char* STDLIB_MAP_SOURCE_STR = R"###(
@no_extends
@no_constructor
class Map<K, V> {
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

	@native("map_contains_key")
	fun contains(key: K): Bool

	@native("map_contains_key")
	fun has(key: K): Bool

	@native("map_clone")
	fun clone(): Map<K, V>

	@native("map_clone")
	fun toMap(): Map<K, V>

	@native("map_clone")
	fun toMutableMap(): Map<K, V>

	@native("map_clone")
	fun toHashMap(): Map<K, V>

	@native("map_size")
	fun count(): Int

	@native("map_filter")
	fun filter(fn: (K, V) -> Bool): Map<K, V>

	@native("map_to_string")
	fun toString(): String

	@native("map_plus")
	operator fun plus(other: Map<K, V>): Map<K, V>

	@native("map_is_not_empty")
	fun isNotEmpty(): Bool

	@native("map_contains_value")
	fun containsValue(value: V): Bool

	@native("map_minus")
	operator fun minus(key: K): Map<K, V>

	@native("map_get_or_else")
	fun getOrElse(key: K, defaultBlock: () -> V): V

	@native("map_get_or_put")
	fun getOrPut(key: K, defaultValue: () -> V): V

	@native("map_filter_keys")
	fun filterKeys(predicate: (K) -> Bool): Map<K, V>

	@native("map_filter_values")
	fun filterValues(predicate: (V) -> Bool): Map<K, V>

	@native("map_map_values")
	fun <R> mapValues(transform: (V) -> R): Map<K, R>

	@native("map_map_keys")
	fun <R> mapKeys(transform: (K) -> R): Map<R, V>

	@native("map_plus_pair")
	fun plus(pair: Any): Map<K, V>

	@native("map_entries")
	fun entries(): Array<Any>

	@native("map_to_list")
	fun toList(): Array<Any>

	@native("map_put_all")
	fun putAll(from: Map<K, V>)

	@native("map_remove_pair")
	fun remove(key: K, value: V): Bool

	@native("map_any_fn")
	fun any(predicate: (K, V) -> Bool): Bool

	@native("map_all_fn")
	fun all(predicate: (K, V) -> Bool): Bool

	@native("map_none_fn")
	fun none(predicate: (K, V) -> Bool): Bool

	@native("map_filter_not")
	fun filterNot(predicate: (K, V) -> Bool): Map<K, V>
}
)###";
