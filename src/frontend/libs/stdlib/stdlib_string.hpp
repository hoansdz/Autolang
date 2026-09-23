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



	@native("str_get")
	fun get(position: Int): String

	@native("str_get")
	fun at(position: Int): String

	@native("str_get")
	fun elementAt(position: Int): String

	fun first(): String = this.get(0)

	fun last(): String = this.get(this.length() - 1)

	fun firstOrNull(): String? = if (this.isEmpty()) null else this.get(0)

	fun lastOrNull(): String? = if (this.isEmpty()) null else this.get(this.length() - 1)

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
	fun contains(char: Int, ignoreCase: Bool = false): Bool

	@native("str_contains")
	fun includes(sub: String): Bool

	@native("str_index_of")
	fun indexOf(sub: String, startIndex: Int = 0, ignoreCase: Bool = false): Int

	@native("str_index_of")
	fun indexOf(char: Int, startIndex: Int = 0, ignoreCase: Bool = false): Int

	@native("str_index_of")
	fun find(sub: String): Int

	@native("str_split")
	fun split(delimiter: String, ignoreCase: Bool = false, limit: Int = 0): Array<String>

	@native("str_split")
	fun split(delimiter: Int, ignoreCase: Bool = false, limit: Int = 0): Array<String>

	@native("str_replace")
	fun replace(old: String, new: String, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: Int, new: Int, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: Int, new: String, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replace(old: String, new: Int, ignoreCase: Bool = false): String

	@native("str_replace")
	fun replaceAll(old: String, new: String): String

	@native("str_starts_with")
	fun startsWith(prefix: String, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_starts_with")
	fun startsWith(prefix: Int, startIndex: Int = 0, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endsWith(suffix: String, ignoreCase: Bool = false): Bool

	@native("str_ends_with")
	fun endsWith(suffix: Int, ignoreCase: Bool = false): Bool

	@native("str_last_index_of")
	fun lastIndexOf(sub: String, startIndex: Int = -1, ignoreCase: Bool = false): Int

	@native("str_last_index_of")
	fun lastIndexOf(char: Int, startIndex: Int = -1, ignoreCase: Bool = false): Int

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

	@native("str_pad_end")
	fun padEnd(length: Int, padChar: String = " "): String

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

	@native("str_remove_suffix")
	fun removeSuffix(suffix: String): String

	@native("str_reversed")
	fun reversed(): String

	@native("str_replace_first")
	fun replaceFirst(oldValue: String, newValue: String, ignoreCase: Bool = false): String

	@native("str_replace_first_char")
	fun replaceFirstChar(transform: (String) -> String): String

	@native("str_take_while")
	fun takeWhile(predicate: (String) -> Bool): String

	@native("str_drop_while")
	fun dropWhile(predicate: (String) -> Bool): String

	@native("str_filter")
	fun filter(predicate: (String) -> Bool): String

	@native("str_substring_before")
	fun substringBefore(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_before")
	fun substringBefore(delimiter: Int, missingDelimiterValue: String = ""): String

	@native("str_substring_after")
	fun substringAfter(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_after")
	fun substringAfter(delimiter: Int, missingDelimiterValue: String = ""): String

	@native("str_substring_before_last")
	fun substringBeforeLast(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_before_last")
	fun substringBeforeLast(delimiter: Int, missingDelimiterValue: String = ""): String

	@native("str_substring_after_last")
	fun substringAfterLast(delimiter: String, missingDelimiterValue: String = ""): String

	@native("str_substring_after_last")
	fun substringAfterLast(delimiter: Int, missingDelimiterValue: String = ""): String

	fun lastIndex(): Int = this.length() - 1

	@native("str_filter_not")
	fun filterNot(predicate: (String) -> Bool): String

	fun isDigit(): Bool = if (this.length() == 0) false else this.charAt(0).isDigit()

	fun isLetter(): Bool = if (this.length() == 0) false else this.charAt(0).isLetter()

	fun isLetterOrDigit(): Bool = if (this.length() == 0) false else this.charAt(0).isLetterOrDigit()

	fun isWhitespace(): Bool = if (this.length() == 0) false else this.charAt(0).isWhitespace()

	fun isUpperCase(): Bool = if (this.length() == 0) false else this.charAt(0).isUpperCase()

	fun isLowerCase(): Bool = if (this.length() == 0) false else this.charAt(0).isLowerCase()
}
)###";

#endif
