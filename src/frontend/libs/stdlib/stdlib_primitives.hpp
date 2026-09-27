#ifndef STDLIB_PRIMITIVES_HPP
#define STDLIB_PRIMITIVES_HPP

#define STDLIB_PRIMITIVES_SOURCE STDLIB_PRIMITIVES_SOURCE_STR
inline constexpr const char* STDLIB_PRIMITIVES_SOURCE_STR = R"###(
@no_extends
@no_constructor
class Int {
	static val MIN_VALUE: Int = -2147483648
	static val MAX_VALUE: Int = 2147483647

	@native("to_string")
	fun toString(): String

	@native("int_coerce_in")
	fun coerceIn(minimumValue: Int, maximumValue: Int): Int

	@native("int_coerce_at_least")
	fun coerceAtLeast(minimumValue: Int): Int

	@native("int_coerce_at_most")
	fun coerceAtMost(maximumValue: Int): Int

	@native("int_abs")
	fun abs(): Int

	@native("int_abs")
	fun absoluteValue(): Int

	@native("int_sign")
	fun sign(): Int

	@native("int_pow")
	fun pow(exp: Int): Int

	@native("int_with_sign")
	fun withSign(sign: Int): Int

	@native("int_with_sign")
	fun withSign(sign: Float): Int

	@native("int_shl")
	fun shl(bitCount: Int): Int

	@native("int_shr")
	fun shr(bitCount: Int): Int

	@native("int_ushr")
	fun ushr(bitCount: Int): Int

	@native("int_xor")
	fun xor(other: Int): Int

	@native("int_inv")
	fun inv(): Int

	@native("str_to_float")
	fun toFloat(): Float
	@native("str_to_float")
	fun toDouble(): Float
	@native("identity")
	fun toInt(): Int
	@native("identity")
	fun toLong(): Int
	@native("int_to_char")
	fun toChar(): Char
	@native("identity")
	fun toByte(): Int
	@native("identity")
	fun toShort(): Int

	@native("int_to_char_string")
	fun toCharString(): String

	@native("int_is_digit")
	fun isDigit(): Bool
	@native("int_is_letter")
	fun isLetter(): Bool
	@native("int_is_letter_or_digit")
	fun isLetterOrDigit(): Bool
	@native("int_is_whitespace")
	fun isWhitespace(): Bool
	@native("int_is_uppercase")
	fun isUpperCase(): Bool
	@native("int_is_lowercase")
	fun isLowerCase(): Bool

	@native("int_digit_to_int")
	fun digitToInt(): Int

	@native("int_digit_to_int_or_null")
	fun digitToIntOrNull(): Int?

	@native("int_uppercase_char")
	fun uppercaseChar(): Int
	@native("int_lowercase_char")
	fun lowercaseChar(): Int
	@native("int_uppercase_string")
	fun uppercase(): String
	@native("int_lowercase_string")
	fun lowercase(): String

	@native("int_down_to")
	fun downTo(to: Int): Array<Int>
}
@no_extends
@no_constructor
class Float {
	static val MIN_VALUE: Float = 1.4e-45
	static val MAX_VALUE: Float = 3.4028235e+38

	@native("to_string")
	fun toString(): String

	@native("float_coerce_in")
	fun coerceIn(minimumValue: Float, maximumValue: Float): Float

	@native("float_coerce_at_least")
	fun coerceAtLeast(minimumValue: Float): Float

	@native("float_coerce_at_most")
	fun coerceAtMost(maximumValue: Float): Float

	@native("float_round_to_int")
	fun roundToInt(): Int

	@native("float_round_to_long")
	fun roundToLong(): Int

	@native("float_round")
	fun round(): Float

	@native("float_truncate")
	fun truncate(): Float

	@native("float_floor")
	fun floor(): Float

	@native("float_ceil")
	fun ceil(): Float

	@native("float_abs")
	fun abs(): Float

	@native("float_abs")
	fun absoluteValue(): Float

	@native("float_sign")
	fun sign(): Float

	@native("float_sqrt")
	fun sqrt(): Float

	@native("float_pow")
	fun pow(n: Float): Float

	@native("float_pow")
	fun pow(n: Int): Float

	@native("float_is_finite")
	fun isFinite(): Bool

	@native("float_is_infinite")
	fun isInfinite(): Bool

	@native("float_is_nan")
	fun isNaN(): Bool

	@native("float_cbrt")
	fun cbrt(): Float

	@native("float_next_up")
	fun nextUp(): Float

	@native("float_next_down")
	fun nextDown(): Float

	@native("float_ulp")
	fun ulp(): Float

	@native("float_with_sign")
	fun withSign(sign: Float): Float

	@native("float_with_sign")
	fun withSign(sign: Int): Float

	@native("float_round_to_int")
	fun toInt(): Int
	@native("float_round_to_long")
	fun toLong(): Int
	@native("identity")
	fun toFloat(): Float
	@native("identity")
	fun toDouble(): Float
	@native("float_round_to_int")
	fun toChar(): Int
	@native("float_round_to_int")
	fun toByte(): Int
	@native("float_round_to_int")
	fun toShort(): Int
}
@no_extends
@no_constructor
class Bool {
	@native("to_string")
	fun toString(): String
}
@no_extends
@no_constructor
class Char {
	@native("char_to_string")
	fun toString(): String

	@native("char_code")
	fun code(): Int

	@native("char_code")
	fun toInt(): Int
	@native("identity")
	fun toChar(): Char

	@native("char_is_digit")
	fun isDigit(): Bool
	@native("char_is_letter")
	fun isLetter(): Bool
	@native("char_is_letter_or_digit")
	fun isLetterOrDigit(): Bool
	@native("char_is_whitespace")
	fun isWhitespace(): Bool
	@native("char_is_uppercase")
	fun isUpperCase(): Bool
	@native("char_is_lowercase")
	fun isLowerCase(): Bool

	@native("char_digit_to_int")
	fun digitToInt(): Int

	@native("char_digit_to_int_or_null")
	fun digitToIntOrNull(): Int?

	@native("char_uppercase_char")
	fun uppercaseChar(): Char

	@native("char_lowercase_char")
	fun lowercaseChar(): Char

	@native("char_uppercase_string")
	fun uppercase(): String
	@native("char_lowercase_string")
	fun lowercase(): String

	@native("char_compare_to")
	fun compareTo(other: Char): Int
	@native("char_equals")
	fun equals(other: Char): Bool
}
)###";

#endif
