#ifndef CONVERSION_FUNCTIONS_HPP
#define CONVERSION_FUNCTIONS_HPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>

namespace Autolang {
namespace DefaultFunction {

inline std::string to_string(ANotifier &notifier, AObject *obj, std::string space = std::string()) {
	if (!obj) {
		return "c_nullptr";
	}
	uint32_t type = obj->type;
	switch (type) {
		case Autolang::DefaultClass::intClassId:
			return std::to_string(obj->i);
		case Autolang::DefaultClass::charClassId:
			return AString::codePointToUtf8(obj->chr);
		case Autolang::DefaultClass::floatClassId:
			return std::to_string(obj->f);
		case Autolang::DefaultClass::stringClassId:
			return std::string(obj->str->data);
		case Autolang::DefaultClass::nullClassId:
			return "null";
		case DefaultClass::boolClassId:
			return (obj == DefaultClass::trueObject ? "true" : "false");
		case Autolang::DefaultClass::functionClassId: {
			if (!obj->function || !obj->function->function) {
				return "Function";
			}
			auto func = obj->function->function;
			auto &data = notifier.vm->data;
			std::string name = func->getName(data);
			std::string params = "(";
			for (uint32_t i = 0; i < func->argSize; ++i) {
				if (i > 0) params += ", ";
				ClassId paramClassId = (func->args && i < func->argSize) ? func->args[i] : 0;
				if (paramClassId < data.classes.size() && data.classes[paramClassId]) {
					params += data.classes[paramClassId]->getName(data);
				} else {
					params += "Any";
				}
			}
			params += ")";
			std::string returnType = "Void";
			if (func->returnId < data.classes.size() && data.classes[func->returnId]) {
				returnType = data.classes[func->returnId]->getName(data);
			}
			if (func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE) {
				returnType += "?";
			}
			if (name.rfind("Closure@", 0) == 0 || name.empty()) {
				return params + " -> " + returnType;
			}
			return "fun " + name + params + ": " + returnType;
		}
		default:
			if (obj->flags & AObject::Flags::OBJ_IS_ARRAY) {
				return Libs::array::to_string(notifier, obj);
			}
			if (obj->flags & AObject::Flags::OBJ_IS_SET) {
				return Libs::set::to_string(notifier, obj);
			}
			if (obj->flags & AObject::Flags::OBJ_IS_MAP) {
				return Libs::map::to_string(notifier, obj);
			}
			auto clazz = notifier.vm->data.classes[obj->type];
			auto it = clazz->funcMap.find("toString");
			if (it != clazz->funcMap.end()) {
				for (FunctionId id : it->second) {
					auto func = notifier.vm->data.functions[id];
					if (func->argSize != 1 ||
					    (func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
						continue;
					}
					if (func->returnId != DefaultClass::stringClassId) {
						break;
					}
					auto data = notifier.callFunction(func, obj);
					if (notifier.hasException()) {
						return "";
					}
					std::string result = data->str->data;
					notifier.release(data);
					return result;
				}
			}

			std::string newSpace = space + "  ";

			if (obj->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) {
				std::string result =
				    clazz->getName(notifier.vm->data) + " (\n" + newSpace;
				bool isFirst = true;
				for (auto &[memberName, id] : clazz->memberMap) {
					if (isFirst) {
						isFirst = false;
					} else {
						result += ",\n" + newSpace;
					}
					std::string str =
					    to_string(notifier, obj->member->data[id], newSpace);
					result += memberName;
					result += " : ";
					result += std::move(str);
				}
				result += "\n" + space + ")";
				return result;
			}
			std::stringstream ss;
			ss << clazz->getName(notifier.vm->data) << "@" << obj;
			return ss.str();
	}
}

inline AObject *to_int(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			return notifier.createInt(obj->i);
		case Autolang::DefaultClass::charClassId:
			return notifier.createInt(obj->chr);
		case Autolang::DefaultClass::floatClassId:
			return notifier.createInt(static_cast<int64_t>(obj->f));
		case Autolang::DefaultClass::boolClassId:
			return notifier.createInt(static_cast<int64_t>(obj->b));
		case Autolang::DefaultClass::stringClassId: {
			int radix = 10;
			if (argSize >= 2 && args[1] && args[1]->type == DefaultClass::intClassId) {
				radix = static_cast<int>(args[1]->i);
			}
			if (radix < 2 || radix > 36) {
				notifier.throwException("Invalid radix: " + std::to_string(radix));
				return nullptr;
			}
			if (obj->str->size == 0) {
				notifier.throwException("NumberFormatException: For input string: \"\"");
				return nullptr;
			}
			char *end = nullptr;
			int64_t val = std::strtoll(obj->str->data, &end, radix);
			if (end == obj->str->data || *end != '\0') {
				notifier.throwException("NumberFormatException: For input string: \"" + std::string(obj->str->data) + "\"");
				return nullptr;
			}
			return notifier.createInt(val);
		}
		default:
			break;
	}
	notifier.throwException("Cannot cast '" + notifier.getClassName(obj->type) +
	                        "' to 'Int'");
	return nullptr;
}

inline AObject *to_float(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			return notifier.createFloat(static_cast<double>(obj->i));
		case Autolang::DefaultClass::floatClassId:
			return notifier.createFloat(obj->f);
		case Autolang::DefaultClass::boolClassId:
			return notifier.createFloat(static_cast<double>(obj->b));
		case Autolang::DefaultClass::stringClassId: {
			// Check error but unused
			char *end;
			return notifier.createFloat(std::strtod(obj->str->data, &end));
		}
		default:
			break;
	}
	notifier.throwException("Cannot cast '" + notifier.getClassName(obj->type) +
	                        "' to 'Float'");
	return nullptr;
}

inline AObject *to_char(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::charClassId:
			return notifier.createChar(obj->chr);
		case Autolang::DefaultClass::intClassId:
			return notifier.createChar(static_cast<AChar>(obj->i));
		default:
			break;
	}
	notifier.throwException("Cannot cast '" + notifier.getClassName(obj->type) +
	                        "' to 'Char'");
	return nullptr;
}

inline AObject *int_to_char(NativeFuncInData) {
	return notifier.createChar(static_cast<AChar>(args[0]->i));
}

inline AObject *to_string(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			return notifier.createString(AString::from(obj->i));
		case Autolang::DefaultClass::charClassId: {
			std::string s = AString::codePointToUtf8(obj->chr);
			return notifier.createString(AString::from(s));
		}
		case Autolang::DefaultClass::floatClassId:
			return notifier.createString(AString::from(obj->f));
		case Autolang::DefaultClass::stringClassId:
			return notifier.createString(AString::copy(obj->str));
		case Autolang::DefaultClass::boolClassId:
			return notifier.createString(obj == Autolang::DefaultClass::trueObject ? "true" : "false");
		case Autolang::DefaultClass::functionClassId: {
			std::string s = to_string(notifier, obj);
			return notifier.createString(AString::from(s));
		}
		default: {
			if (obj == Autolang::DefaultClass::nullObject) {
				return notifier.createString("null");
			}
			char buf[64];
			snprintf(buf, sizeof(buf), "%s@%p", notifier.getClassName(obj->type).c_str(), (void *)obj);
			return notifier.createString(buf);
		}
	}
	return nullptr;
}

inline AObject *int_coerce_in(NativeFuncInData) {
	int64_t val = args[0]->i;
	int64_t minVal = args[1]->i;
	int64_t maxVal = args[2]->i;
	if (val < minVal) return notifier.createInt(minVal);
	if (val > maxVal) return notifier.createInt(maxVal);
	return notifier.createInt(val);
}

inline AObject *int_coerce_at_least(NativeFuncInData) {
	int64_t val = args[0]->i;
	int64_t minVal = args[1]->i;
	return notifier.createInt(val < minVal ? minVal : val);
}

inline AObject *int_coerce_at_most(NativeFuncInData) {
	int64_t val = args[0]->i;
	int64_t maxVal = args[1]->i;
	return notifier.createInt(val > maxVal ? maxVal : val);
}

inline AObject *float_coerce_in(NativeFuncInData) {
	double val = args[0]->f;
	double minVal = args[1]->f;
	double maxVal = args[2]->f;
	if (val < minVal) return notifier.createFloat(minVal);
	if (val > maxVal) return notifier.createFloat(maxVal);
	return notifier.createFloat(val);
}

inline AObject *float_coerce_at_least(NativeFuncInData) {
	double val = args[0]->f;
	double minVal = args[1]->f;
	return notifier.createFloat(val < minVal ? minVal : val);
}

inline AObject *float_coerce_at_most(NativeFuncInData) {
	double val = args[0]->f;
	double maxVal = args[1]->f;
	return notifier.createFloat(val > maxVal ? maxVal : val);
}

inline AObject *float_round_to_int(NativeFuncInData) {
	double val = args[0]->f;
	if (std::isnan(val)) return notifier.createInt(0);
	return notifier.createInt(static_cast<int64_t>(std::round(val)));
}

inline AObject *float_round_to_long(NativeFuncInData) {
	double val = args[0]->f;
	if (std::isnan(val)) return notifier.createInt(0);
	return notifier.createInt(static_cast<int64_t>(std::round(val)));
}

inline AObject *float_round(NativeFuncInData) {
	return notifier.createFloat(std::round(args[0]->f));
}

inline AObject *float_truncate(NativeFuncInData) {
	return notifier.createFloat(std::trunc(args[0]->f));
}

inline AObject *float_floor(NativeFuncInData) {
	return notifier.createFloat(std::floor(args[0]->f));
}

inline AObject *float_ceil(NativeFuncInData) {
	return notifier.createFloat(std::ceil(args[0]->f));
}

inline AObject *float_abs(NativeFuncInData) {
	return notifier.createFloat(std::abs(args[0]->f));
}

inline AObject *float_sign(NativeFuncInData) {
	double val = args[0]->f;
	if (std::isnan(val) || val == 0.0) return notifier.createFloat(val);
	return notifier.createFloat(val > 0.0 ? 1.0 : -1.0);
}

inline AObject *float_sqrt(NativeFuncInData) {
	double val = (args[0]->type == DefaultClass::intClassId) ? static_cast<double>(args[0]->i) : args[0]->f;
	return notifier.createFloat(std::sqrt(val));
}

inline AObject *float_pow(NativeFuncInData) {
	double base = (args[0]->type == DefaultClass::intClassId) ? static_cast<double>(args[0]->i) : args[0]->f;
	double exp = (args[1]->type == DefaultClass::intClassId) ? static_cast<double>(args[1]->i) : args[1]->f;
	return notifier.createFloat(std::pow(base, exp));
}

inline AObject *float_is_finite(NativeFuncInData) {
	return notifier.createBool(std::isfinite(args[0]->f));
}

inline AObject *float_is_infinite(NativeFuncInData) {
	return notifier.createBool(std::isinf(args[0]->f));
}

inline AObject *float_is_nan(NativeFuncInData) {
	return notifier.createBool(std::isnan(args[0]->f));
}

inline AObject *int_abs(NativeFuncInData) {
	return notifier.createInt(std::abs(args[0]->i));
}

inline AObject *int_sign(NativeFuncInData) {
	int64_t val = args[0]->i;
	return notifier.createInt(val > 0 ? 1 : (val < 0 ? -1 : 0));
}

inline AObject *int_pow(NativeFuncInData) {
	int64_t base = args[0]->i;
	int64_t exp = args[1]->i;
	if (exp < 0) {
		notifier.throwException("Exponent must be non-negative");
		return nullptr;
	}
	int64_t result = 1;
	while (exp > 0) {
		if (exp & 1) result *= base;
		base *= base;
		exp >>= 1;
	}
	return notifier.createInt(result);
}

inline AObject *int_shl(NativeFuncInData) {
	return notifier.createInt(args[0]->i << args[1]->i);
}

inline AObject *int_shr(NativeFuncInData) {
	return notifier.createInt(args[0]->i >> args[1]->i);
}

inline AObject *int_ushr(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(static_cast<uint64_t>(args[0]->i) >> args[1]->i));
}

inline AObject *int_xor(NativeFuncInData) {
	return notifier.createInt(args[0]->i ^ args[1]->i);
}

inline AObject *int_inv(NativeFuncInData) {
	return notifier.createInt(~args[0]->i);
}

inline AObject *int_to_binary_string(NativeFuncInData) {
	if (argSize == 0 || !args[0] || args[0]->type != DefaultClass::intClassId) {
		notifier.throwException("Expected Int argument for toBinaryString");
		return nullptr;
	}
	uint64_t val = static_cast<uint64_t>(args[0]->i);
	if (val == 0) {
		return notifier.createString("0");
	}
	std::string s;
	while (val > 0) {
		s.push_back((val & 1) ? '1' : '0');
		val >>= 1;
	}
	std::reverse(s.begin(), s.end());
	return notifier.createString(std::move(s));
}

inline AObject *float_cbrt(NativeFuncInData) {
	return notifier.createFloat(std::cbrt(args[0]->f));
}

inline AObject *float_next_up(NativeFuncInData) {
	return notifier.createFloat(std::nextafter(args[0]->f, std::numeric_limits<double>::infinity()));
}

inline AObject *float_next_down(NativeFuncInData) {
	return notifier.createFloat(std::nextafter(args[0]->f, -std::numeric_limits<double>::infinity()));
}

inline AObject *float_ulp(NativeFuncInData) {
	double val = args[0]->f;
	if (std::isnan(val)) return notifier.createFloat(std::numeric_limits<double>::quiet_NaN());
	if (std::isinf(val)) return notifier.createFloat(std::numeric_limits<double>::infinity());
	if (val == 0.0) return notifier.createFloat(std::numeric_limits<double>::denorm_min());
	double next = std::nextafter(std::abs(val), std::numeric_limits<double>::infinity());
	return notifier.createFloat(next - std::abs(val));
}

inline AObject *float_with_sign(NativeFuncInData) {
	double mag = args[0]->f;
	double s = (args[1]->type == DefaultClass::intClassId) ? static_cast<double>(args[1]->i) : args[1]->f;
	return notifier.createFloat(std::copysign(mag, s));
}

inline AObject *int_with_sign(NativeFuncInData) {
	int64_t mag = std::abs(args[0]->i);
	bool negative = (args[1]->type == DefaultClass::intClassId) ? (args[1]->i < 0) : (args[1]->f < 0);
	return notifier.createInt(negative ? -mag : mag);
}

inline AObject *pair_to_string(NativeFuncInData) {
	auto self = args[0];
	if (!self || !(self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) || !self->member || self->member->size < 2) {
		return notifier.createString("()");
	}
	std::string s1 = to_string(notifier, self->member->data[0]);
	std::string s2 = to_string(notifier, self->member->data[1]);
	return notifier.createString("(" + s1 + ", " + s2 + ")");
}

inline AObject *triple_to_string(NativeFuncInData) {
	auto self = args[0];
	if (!self || !(self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) || !self->member || self->member->size < 3) {
		return notifier.createString("()");
	}
	std::string s1 = to_string(notifier, self->member->data[0]);
	std::string s2 = to_string(notifier, self->member->data[1]);
	std::string s3 = to_string(notifier, self->member->data[2]);
	return notifier.createString("(" + s1 + ", " + s2 + ", " + s3 + ")");
}

inline AObject *pair_to_list(NativeFuncInData) {
	auto self = args[0];
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			elemKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto arr = notifier.createArray(returnId, elemKey);
	if (self && (self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && self->member && self->member->size >= 2) {
		notifier.arrayAdd(arr, self->member->data[0]);
		notifier.arrayAdd(arr, self->member->data[1]);
	}
	return arr;
}

inline AObject *triple_to_list(NativeFuncInData) {
	auto self = args[0];
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			elemKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto arr = notifier.createArray(returnId, elemKey);
	if (self && (self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && self->member && self->member->size >= 3) {
		notifier.arrayAdd(arr, self->member->data[0]);
		notifier.arrayAdd(arr, self->member->data[1]);
		notifier.arrayAdd(arr, self->member->data[2]);
	}
	return arr;
}

inline AObject *char_to_string(NativeFuncInData) {
	AChar c = args[0]->chr;
	std::string s = AString::codePointToUtf8(c);
	return notifier.createString(AString::from(s));
}

inline AObject *char_code(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->chr));
}

inline AObject *char_uppercase_char(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 'a' && c <= 'z') c -= 32;
	return notifier.createChar(c);
}

inline AObject *char_lowercase_char(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 'A' && c <= 'Z') c += 32;
	return notifier.createChar(c);
}

inline AObject *identity(NativeFuncInData) {
	return args[0];
}

inline AObject *int_is_digit(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool(v >= 48 && v <= 57);
}

inline AObject *int_is_letter(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool((v >= 65 && v <= 90) || (v >= 97 && v <= 122));
}

inline AObject *int_is_letter_or_digit(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool((v >= 48 && v <= 57) || (v >= 65 && v <= 90) || (v >= 97 && v <= 122));
}

inline AObject *int_is_whitespace(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool(v == 32 || v == 9 || v == 10 || v == 13);
}

inline AObject *int_is_uppercase(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool(v >= 65 && v <= 90);
}

inline AObject *int_is_lowercase(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createBool(v >= 97 && v <= 122);
}

inline AObject *int_digit_to_int(NativeFuncInData) {
	int64_t v = args[0]->i;
	if (v >= 48 && v <= 57) {
		return notifier.createInt(v - 48);
	}
	notifier.throwException("IllegalArgumentException: Char is not a decimal digit");
	return nullptr;
}

inline AObject *int_digit_to_int_or_null(NativeFuncInData) {
	int64_t v = args[0]->i;
	if (v >= 48 && v <= 57) {
		return notifier.createInt(v - 48);
	}
	return DefaultClass::nullObject;
}

inline AObject *int_uppercase_char(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createInt((v >= 97 && v <= 122) ? (v - 32) : v);
}

inline AObject *int_lowercase_char(NativeFuncInData) {
	int64_t v = args[0]->i;
	return notifier.createInt((v >= 65 && v <= 90) ? (v + 32) : v);
}

inline AObject *int_uppercase_string(NativeFuncInData) {
	int64_t v = args[0]->i;
	int64_t up = (v >= 97 && v <= 122) ? (v - 32) : v;
	std::string s = AString::codePointToUtf8(static_cast<AChar>(up));
	return notifier.createString(AString::from(s));
}

inline AObject *int_lowercase_string(NativeFuncInData) {
	int64_t v = args[0]->i;
	int64_t low = (v >= 65 && v <= 90) ? (v + 32) : v;
	std::string s = AString::codePointToUtf8(static_cast<AChar>(low));
	return notifier.createString(AString::from(s));
}

inline AObject *int_down_to(NativeFuncInData) {
	int64_t from = args[0]->i;
	int64_t to = args[1]->i;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	auto arr = notifier.createArray(returnId, DefaultClass::intClassId);
	for (int64_t curr = from; curr >= to; --curr) {
		notifier.arrayAdd(arr, notifier.createInt(curr));
	}
	return arr;
}

inline AObject *char_is_digit(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool(c >= 48 && c <= 57);
}

inline AObject *char_is_letter(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool((c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}

inline AObject *char_is_letter_or_digit(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}

inline AObject *char_is_whitespace(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool(c == 32 || c == 9 || c == 10 || c == 13);
}

inline AObject *char_is_uppercase(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool(c >= 65 && c <= 90);
}

inline AObject *char_is_lowercase(NativeFuncInData) {
	AChar c = args[0]->chr;
	return notifier.createBool(c >= 97 && c <= 122);
}

inline AObject *char_digit_to_int(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 48 && c <= 57) {
		return notifier.createInt(c - 48);
	}
	notifier.throwException("IllegalArgumentException: Char is not a decimal digit");
	return nullptr;
}

inline AObject *char_digit_to_int_or_null(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 48 && c <= 57) {
		return notifier.createInt(c - 48);
	}
	return DefaultClass::nullObject;
}

inline AObject *char_uppercase_string(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 'a' && c <= 'z') c -= 32;
	std::string s = AString::codePointToUtf8(c);
	return notifier.createString(AString::from(s));
}

inline AObject *char_lowercase_string(NativeFuncInData) {
	AChar c = args[0]->chr;
	if (c >= 'A' && c <= 'Z') c += 32;
	std::string s = AString::codePointToUtf8(c);
	return notifier.createString(AString::from(s));
}

inline AObject *char_compare_to(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->chr) - static_cast<int64_t>(args[1]->chr));
}

inline AObject *char_equals(NativeFuncInData) {
	return notifier.createBool(args[0]->chr == args[1]->chr);
}

inline AObject *tuple_first(NativeFuncInData) {
	auto self = args[0];
	if (self && (self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && self->member && self->member->size > 0) {
		return self->member->data[0];
	}
	return DefaultClass::nullObject;
}

inline AObject *tuple_second(NativeFuncInData) {
	auto self = args[0];
	if (self && (self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && self->member && self->member->size > 1) {
		return self->member->data[1];
	}
	return DefaultClass::nullObject;
}

inline AObject *tuple_third(NativeFuncInData) {
	auto self = args[0];
	if (self && (self->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && self->member && self->member->size > 2) {
		return self->member->data[2];
	}
	return DefaultClass::nullObject;
}

inline AObject *pair_copy(NativeFuncInData) {
	auto self = args[0];
	ClassId pairId = self->type;
	auto obj = notifier.createMemberObject(pairId, 2);
	AObject *firstVal = argSize > 1 ? args[1] : self->member->data[0];
	AObject *secondVal = argSize > 2 ? args[2] : self->member->data[1];
	obj->member->data[0] = firstVal;
	obj->member->data[1] = secondVal;
	firstVal->retain();
	secondVal->retain();
	return obj;
}

inline AObject *pair_of(NativeFuncInData) {
	ClassId pairId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	if (pairId < DefaultClass::builtInObjectSize || pairId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[pairId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Pair");
		if (it != notifier.vm->data.classMap.end()) pairId = it->second;
	}
	auto obj = notifier.createMemberObject(pairId, 2);
	obj->member->data[0] = args[0];
	obj->member->data[1] = args[1];
	args[0]->retain();
	args[1]->retain();
	return obj;
}

inline AObject *triple_copy(NativeFuncInData) {
	auto self = args[0];
	ClassId tripleId = self->type;
	auto obj = notifier.createMemberObject(tripleId, 3);
	AObject *v1 = argSize > 1 ? args[1] : self->member->data[0];
	AObject *v2 = argSize > 2 ? args[2] : self->member->data[1];
	AObject *v3 = argSize > 3 ? args[3] : self->member->data[2];
	obj->member->data[0] = v1;
	obj->member->data[1] = v2;
	obj->member->data[2] = v3;
	v1->retain();
	v2->retain();
	v3->retain();
	return obj;
}

inline AObject *triple_of(NativeFuncInData) {
	ClassId tripleId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	if (tripleId < DefaultClass::builtInObjectSize || tripleId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[tripleId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Triple");
		if (it != notifier.vm->data.classMap.end()) tripleId = it->second;
	}
	auto obj = notifier.createMemberObject(tripleId, 3);
	obj->member->data[0] = args[0];
	obj->member->data[1] = args[1];
	obj->member->data[2] = args[2];
	args[0]->retain();
	args[1]->retain();
	args[2]->retain();
	return obj;
}

inline AObject *indexed_value_of(NativeFuncInData) {
	ClassId ivId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	if (ivId < DefaultClass::builtInObjectSize || ivId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[ivId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("IndexedValue");
		if (it != notifier.vm->data.classMap.end()) ivId = it->second;
	}
	auto obj = notifier.createMemberObject(ivId, 2);
	obj->member->data[0] = args[0];
	obj->member->data[1] = args[1];
	args[0]->retain();
	args[1]->retain();
	return obj;
}

inline AObject *indexed_value_to_string(NativeFuncInData) {
	auto self = args[0];
	std::string s1 = to_string(notifier, self->member->data[0]);
	std::string s2 = to_string(notifier, self->member->data[1]);
	return notifier.createString("(" + s1 + ", " + s2 + ")");
}

inline AObject *map_entry_to_string(NativeFuncInData) {
	auto self = args[0];
	std::string s1 = to_string(notifier, self->member->data[0]);
	std::string s2 = to_string(notifier, self->member->data[1]);
	return notifier.createString(s1 + "=" + s2);
}

inline AObject *ref_create(NativeFuncInData) {
	ClassId refId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	if (refId < DefaultClass::builtInObjectSize || refId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[refId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Ref");
		if (it != notifier.vm->data.classMap.end()) refId = it->second;
	}
	auto obj = notifier.createMemberObject(refId, 1);
	obj->member->data[0] = args[0];
	args[0]->retain();
	return obj;
}

inline AObject *comparator_then_by(NativeFuncInData) {
	auto self = args[0];
	auto selector = args[1];
	auto selectorsArr = self->member->data[0];
	auto dirsArr = self->member->data[1];
	notifier.arrayAdd(selectorsArr, selector);
	notifier.arrayAdd(dirsArr, notifier.createInt(1));
	return self;
}

inline AObject *comparator_then_by_descending(NativeFuncInData) {
	auto self = args[0];
	auto selector = args[1];
	auto selectorsArr = self->member->data[0];
	auto dirsArr = self->member->data[1];
	notifier.arrayAdd(selectorsArr, selector);
	notifier.arrayAdd(dirsArr, notifier.createInt(-1));
	return self;
}

inline AObject *create_comparator_helper(ANotifier &notifier, ClassId compId, const std::vector<AObject*> &selectors, const std::vector<int64_t> &dirs) {
	if (compId < DefaultClass::builtInObjectSize || compId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[compId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Comparator");
		if (it != notifier.vm->data.classMap.end()) compId = it->second;
	}
	auto compObj = notifier.createMemberObject(compId, 2);
	auto selArr = notifier.createArray(DefaultClass::arrayClassId, DefaultClass::anyClassId);
	auto dirArr = notifier.createArray(DefaultClass::arrayClassId, DefaultClass::intClassId);
	for (auto s : selectors) notifier.arrayAdd(selArr, s);
	for (auto d : dirs) notifier.arrayAdd(dirArr, notifier.createInt(d));
	compObj->member->data[0] = selArr;
	compObj->member->data[1] = dirArr;
	selArr->retain();
	dirArr->retain();
	return compObj;
}

inline AObject *compare_by_1(NativeFuncInData) {
	ClassId compId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	return create_comparator_helper(notifier, compId, {args[0]}, {1});
}

inline AObject *compare_by_2(NativeFuncInData) {
	ClassId compId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	return create_comparator_helper(notifier, compId, {args[0], args[1]}, {1, 1});
}

inline AObject *compare_by_3(NativeFuncInData) {
	ClassId compId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	return create_comparator_helper(notifier, compId, {args[0], args[1], args[2]}, {1, 1, 1});
}

inline AObject *compare_by_4(NativeFuncInData) {
	ClassId compId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	return create_comparator_helper(notifier, compId, {args[0], args[1], args[2], args[3]}, {1, 1, 1, 1});
}

inline AObject *compare_by_descending(NativeFuncInData) {
	ClassId compId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	return create_comparator_helper(notifier, compId, {args[0]}, {-1});
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
