#ifndef STDLIB_PRIMITIVES_HPP
#define STDLIB_PRIMITIVES_HPP

#define STDLIB_PRIMITIVES_SOURCE STDLIB_PRIMITIVES_SOURCE_STR
inline constexpr const char* STDLIB_PRIMITIVES_SOURCE_STR = R"###(
@no_extends
@no_constructor
class Int {
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

	fun toFloat(): Float = this
	fun toDouble(): Float = this
	fun toInt(): Int = this
	fun toLong(): Int = this
	fun toChar(): Int = this
	fun toByte(): Int = this
	fun toShort(): Int = this

	fun isDigit(): Bool = this >= 48 && this <= 57
	fun isLetter(): Bool = (this >= 65 && this <= 90) || (this >= 97 && this <= 122)
	fun isLetterOrDigit(): Bool = (this >= 48 && this <= 57) || (this >= 65 && this <= 90) || (this >= 97 && this <= 122)
	fun isWhitespace(): Bool = this == 32 || this == 9 || this == 10 || this == 13
	fun isUpperCase(): Bool = this >= 65 && this <= 90
	fun isLowerCase(): Bool = this >= 97 && this <= 122

	fun downTo(to: Int): Array<Int> {
		val res: Array<Int> = <Int>[]
		var curr = this
		while (curr >= to) {
			res.add(curr)
			curr = curr - 1
		}
		return res
	}
}
@no_extends
@no_constructor
class Float {
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

	fun toInt(): Int = this
	fun toLong(): Int = this
	fun toFloat(): Float = this
	fun toDouble(): Float = this
	fun toChar(): Int = this
	fun toByte(): Int = this
	fun toShort(): Int = this
}
@no_extends
@no_constructor
class Bool {
	@native("to_string")
	fun toString(): String
}
)###";

#endif
