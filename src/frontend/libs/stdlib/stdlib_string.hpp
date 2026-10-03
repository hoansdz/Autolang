#ifndef STDLIB_STRING_HPP
#define STDLIB_STRING_HPP

#define STDLIB_STRING_SOURCE STDLIB_STRING_SOURCE_STR
inline constexpr const char* STDLIB_STRING_SOURCE_STR = R"###(
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
	@native("to_string")
	fun toString(): String


	@native("str_is_empty")
	fun isEmpty(): Bool

	@native("str_is_empty")
	fun empty(): Bool

	@native("str_is_empty")
	fun is_empty(): Bool

	@native("str_is_null_or_empty")
	fun String?.isNullOrEmpty(): Bool

	@native("str_is_null_or_blank")
	fun String?.isNullOrBlank(): Bool



	@native("str_to_int")
	fun toInt(): Int

	@native("str_to_int")
	fun parseInt(): Int

	@native("str_to_int")
	fun toLong(): Int

	@native("str_to_float")
	fun toFloat(): Float

	@native("str_to_float")
	fun parseFloat(): Float

	@native("str_to_float")
	fun toDouble(): Float

	@native("str_to_bool")
	fun toBool(): Bool

	@native("str_to_bool")
	fun toBoolean(): Bool

	@native("str_to_bool_or_null")
	fun toBoolOrNull(): Bool?

	@native("str_to_bool_or_null")
	fun toBooleanOrNull(): Bool?

	@native("str_to_char")
	fun toChar(): Char

	@native("str_to_char_or_null")
	fun toCharOrNull(): Char?



	@native("str_get")
	fun get(position: Int): Char

	@native("str_get")
	fun at(position: Int): Char

	@native("str_get")
	fun elementAt(position: Int): Char

	@native("str_first")
	fun first(): Char

	@native("str_last")
	fun last(): Char

	@native("str_first_or_null")
	fun firstOrNull(): Char?

	@native("str_last_or_null")
	fun lastOrNull(): Char?

	// @native("str_set")
	// fun set(position: Int, chr: Int)

	// @native("str_set")
	// fun set(position: Int, str: String)

	@native("str_char_at")
	fun charAt(position: Int): Char

	@native("str_substr")
	fun substr(from: Int): String

	@native("str_substr")
	fun substring(from: Int): String

	@native("str_substr")
	fun slice(from: Int): String

	@native("str_substr")
	fun substr(from: Int, subLength: Int): String

	@native("str_substring_range")
	fun substring(startIndex: Int, endIndex: Int): String

	@native("str_substr")
	fun slice(from: Int, subLength: Int): String



	@native("str_trim")
	fun trim(): String

	@native("str_trim")
	fun strip(): String

	@native("str_trim_start")
	fun trimStart(): String

	@native("str_trim_end")
	fun trimEnd(): String

	@native("str_trim_indent")
	fun trimIndent(): String

	@native("str_trim_margin")
	fun trimMargin(marginPrefix: String = "|"): String

	@native("str_contains")
	fun contains(sub: String, ignoreCase: Bool = false): Bool

	@native("str_contains")
	fun contains(char: Char, ignoreCase: Bool = false): Bool

	@native("str_contains")
	fun includes(sub: String): Bool

	@native("str_index_of")
	fun indexOf(sub: String, startIndex: Int = 0, ignoreCase: Bool = false): Int

	@native("str_index_of")
	fun indexOf(char: Char, startIndex: Int = 0, ignoreCase: Bool = false): Int

	@native("str_index_of")
	fun find(sub: String): Int

	@native("str_split")
	fun split(delimiter: String, ignoreCase: Bool = false, limit: Int = 0): Array<String>

	@native("str_split")
	fun split(delimiter: Char, ignoreCase: Bool = false, limit: Int = 0): Array<String>

	@native("str_split")
	fun split(d1: String, d2: String): Array<String>

	@native("str_split")
	fun split(d1: String, d2: String, d3: String): Array<String>

	@native("str_split")
	fun split(d1: String, d2: String, d3: String, d4: String): Array<String>

	@native("str_split")
	fun split(d1: String, d2: String, d3: String, d4: String, d5: String): Array<String>

	@native("str_split")
	fun split(d1: Char, d2: Char): Array<String>

	@native("str_split")
	fun split(d1: Char, d2: Char, d3: Char): Array<String>

	@native("str_split")
	fun split(d1: Char, d2: Char, d3: Char, d4: Char): Array<String>

	@native("str_split")
	fun split(delimiters: Array<String>): Array<String>

	@native("str_replace")
	fun replace(old: String, new: String, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: Char, new: Char, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: Char, new: String, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: String, new: Char, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replaceAll(old: String, new: String): String

	@native("str_starts_with")
	fun startsWith(prefix: String, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_starts_with")
	fun startsWith(prefix: Char, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endsWith(suffix: String, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endsWith(suffix: Char, ignoreCase: Bool = false): Bool

	@native("str_last_index_of")
	fun lastIndexOf(sub: String, startIndex: Int = -1, ignoreCase: Bool = false): Int

	@native("str_last_index_of")
	fun lastIndexOf(char: Char, startIndex: Int = -1, ignoreCase: Bool = false): Int

	@native("str_last_index_of")
	fun rfind(sub: String): Int

    @native("str_to_lower")
    fun toLowerCase(): String

    @native("str_to_lower")
    fun toLower(): String

    @native("str_to_lower")
    fun lower(): String

    @native("str_to_lower")
    fun lowercase(): String

    @native("str_to_upper")
    fun toUpperCase(): String

    @native("str_to_upper")
    fun toUpper(): String

    @native("str_to_upper")
    fun upper(): String

    @native("str_to_upper")
    fun uppercase(): String

	@native("str_lines")
	fun lines(): Array<String>

	@native("string_constructor")
	fun repeat(n: Int): String

	@native("str_is_blank")
	fun isBlank(): Bool

	@native("str_is_not_blank")
	fun isNotBlank(): Bool

	@native("str_is_not_empty")
	fun isNotEmpty(): Bool

	@native("str_pad_start")
	fun padStart(length: Int, padChar: String = " "): String

	@native("str_pad_start")
	fun padStart(length: Int, padChar: Char): String

	@native("str_pad_end")
	fun padEnd(length: Int, padChar: String = " "): String

	@native("str_pad_end")
	fun padEnd(length: Int, padChar: Char): String

	@native("str_to_int_or_null")
	fun toIntOrNull(): Int?

	@native("str_to_int_or_null")
	fun toLongOrNull(): Int?

	@native("str_to_float_or_null")
	fun toFloatOrNull(): Float?

	@native("str_to_float_or_null")
	fun toDoubleOrNull(): Float?

	@native("str_take")
	fun take(n: Int): String

	@native("str_take_last")
	fun takeLast(n: Int): String

	@native("str_drop")
	fun drop(n: Int): String

	@native("str_drop_last")
	fun dropLast(n: Int): String

	@native("str_remove_prefix")
	fun removePrefix(prefix: String): String

	@native("str_remove_prefix")
	fun removePrefix(prefix: Char): String

	@native("str_remove_suffix")
	fun removeSuffix(suffix: String): String

	@native("str_remove_suffix")
	fun removeSuffix(suffix: Char): String

	@native("str_reversed")
	fun reversed(): String

	@native("str_replace_first")
	fun replaceFirst(oldValue: String, newValue: String, ignoreCase: Bool = false): String

	@native("str_replace_first_char")
	fun replaceFirstChar(transform: (Char) -> String): String

	@native("str_take_while")
	fun takeWhile(predicate: (Char) -> Bool): String

	@native("str_drop_while")
	fun dropWhile(predicate: (Char) -> Bool): String

	@native("str_filter")
	fun filter(predicate: (Char) -> Bool): String

	@native("str_substring_before")
	fun substringBefore(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_before")
	fun substringBefore(delimiter: Char, missingDelimiterValue: String = ""): String

	@native("str_substring_after")
	fun substringAfter(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_after")
	fun substringAfter(delimiter: Char, missingDelimiterValue: String = ""): String

	@native("str_substring_before_last")
	fun substringBeforeLast(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_before_last")
	fun substringBeforeLast(delimiter: Char, missingDelimiterValue: String = ""): String

	@native("str_substring_after_last")
	fun substringAfterLast(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_after_last")
	fun substringAfterLast(delimiter: Char, missingDelimiterValue: String = ""): String

	@native("str_last_index")
	fun lastIndex(): Int

	@native("str_filter_not")
	fun filterNot(predicate: (Char) -> Bool): String

	@native("str_is_digit")
	fun isDigit(): Bool

	@native("str_is_letter")
	fun isLetter(): Bool

	@native("str_is_letter_or_digit")
	fun isLetterOrDigit(): Bool

	@native("str_is_whitespace")
	fun isWhitespace(): Bool

	@native("str_is_uppercase")
	fun isUpperCase(): Bool

	@native("str_is_lowercase")
	fun isLowerCase(): Bool

	@native("str_chunked")
	fun chunked(size: Int): Array<String>

	@native("str_zip_with_next")
	fun zipWithNext(pairClassId: Int = getClassId(Pair<Char, Char>)): Array<Pair<Char, Char>>

	@native("str_zip_with_next_transform")
	fun <R> zipWithNext(transform: (Char, Char) -> R): Array<R>

	@native("str_region_matches")
	fun regionMatches(thisOffset: Int, other: String, otherOffset: Int, length: Int, ignoreCase: Bool = false): Bool

	@native("str_compare_to")
	fun compareTo(other: String, ignoreCase: Bool = false): Int

	@native("str_common_prefix_with")
	fun commonPrefixWith(other: String, ignoreCase: Bool = false): String

	@native("str_common_suffix_with")
	fun commonSuffixWith(other: String, ignoreCase: Bool = false): String

	@native("str_content_equals")
	fun contentEquals(other: String): Bool

	@native("str_remove_surrounding")
	fun removeSurrounding(prefix: String, suffix: String): String

	@native("str_remove_surrounding_single")
	fun removeSurrounding(delimiter: String): String

	@native("str_remove_surrounding_char")
	fun removeSurrounding(prefix: Char, suffix: Char): String

	@native("str_remove_surrounding_char_single")
	fun removeSurrounding(delimiter: Char): String

	@native("str_remove_range")
	fun removeRange(startIndex: Int, endIndex: Int): String

	@native("str_replace_range")
	fun replaceRange(startIndex: Int, endIndex: Int, replacement: String): String

	@native("str_indices")
	fun indices(): Array<Int>

	@native("str_is_not_empty")
	fun any(): Bool

	@native("str_any_fn")
	fun any(predicate: (Char) -> Bool): Bool

	@native("str_all_fn")
	fun all(predicate: (Char) -> Bool): Bool

	@native("str_is_empty")
	fun none(): Bool

	@native("str_none_fn")
	fun none(predicate: (Char) -> Bool): Bool

	@native("string_size")
	fun count(): Int

	@native("str_count_fn")
	fun count(predicate: (Char) -> Bool): Int

	@native("str_index_of_first")
	fun indexOfFirst(predicate: (Char) -> Bool): Int

	@native("str_index_of_last")
	fun indexOfLast(predicate: (Char) -> Bool): Int

	@native("str_first_fn")
	fun first(predicate: (Char) -> Bool): Char

	@native("str_last_fn")
	fun last(predicate: (Char) -> Bool): Char

	@native("str_first_or_null_fn")
	fun firstOrNull(predicate: (Char) -> Bool): Char?

	@native("str_last_or_null_fn")
	fun lastOrNull(predicate: (Char) -> Bool): Char?

	@native("str_first_or_null_fn")
	fun find(predicate: (Char) -> Bool): Char?

	@native("str_last_or_null_fn")
	fun findLast(predicate: (Char) -> Bool): Char?

	@native("str_for_each")
	fun forEach(action: (Char) -> Void)

	@native("str_for_each_indexed")
	fun forEachIndexed(action: (Int, Char) -> Void)

	@native("str_on_each")
	fun onEach(action: (Char) -> Void): String

	@native("str_on_each_indexed")
	fun onEachIndexed(action: (Int, Char) -> Void): String

	@native("str_filter_indexed")
	fun filterIndexed(predicate: (Int, Char) -> Bool): String

	@native("str_take_last_while")
	fun takeLastWhile(predicate: (Char) -> Bool): String

	@native("str_drop_last_while")
	fun dropLastWhile(predicate: (Char) -> Bool): String

	@native("str_reduce")
	fun reduce(operation: (String, String) -> String): String

	@native("str_running_reduce")
	fun runningReduce(operation: (String, String) -> String): Array<String>

	@native("str_windowed")
	fun windowed(size: Int, step: Int = 1, partialWindows: Bool = false): Array<String>

	@native("str_to_list")
	fun toList(): Array<Char>

	@native("str_to_list")
	fun toCharArray(): Array<Char>

	@native("str_to_list")
	fun asSequence(): Array<Char>

	@native("str_to_list")
	fun asIterable(): Array<Char>

	@native("str_trim_start_fn")
	fun trimStart(predicate: (Char) -> Bool): String

	@native("str_trim_end_fn")
	fun trimEnd(predicate: (Char) -> Bool): String

	@native("str_trim_fn")
	fun trim(predicate: (Char) -> Bool): String

	@native("str_trim_start_char")
	fun trimStart(char: Char): String

	@native("str_trim_end_char")
	fun trimEnd(char: Char): String

	@native("str_trim_char")
	fun trim(char: Char): String

	@native("str_trim_start_str")
	fun trimStart(chars: String): String

	@native("str_trim_end_str")
	fun trimEnd(chars: String): String

	@native("str_trim_str")
	fun trim(chars: String): String

	@native("str_step")
	fun step(step: Int): String

	@native("str_join")
	fun <T> join(elements: Array<T>): String

	@native("str_join")
	fun join(elements: Array<Any?>): String

	@native("str_count_str")
	fun count(sub: String): Int

	@native("str_capitalize")
	fun capitalize(): String

	@native("str_starts_with")
	fun startswith(prefix: String, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_starts_with")
	fun startswith(prefix: Char, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endswith(suffix: String, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endswith(suffix: Char, ignoreCase: Bool = false): Bool

	@native("str_trim_start")
	fun lstrip(): String

	@native("str_trim_end")
	fun rstrip(): String

	@native("str_is_digit")
	fun isdigit(): Bool

	@native("str_is_letter")
	fun isalpha(): Bool

	@native("str_is_letter_or_digit")
	fun isalnum(): Bool

	@native("str_is_whitespace")
	fun isspace(): Bool
}
)###";

#endif
