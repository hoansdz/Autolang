#ifndef CONVERSION_FUNCTIONS_HPP
#define CONVERSION_FUNCTIONS_HPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
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
		case Autolang::DefaultClass::floatClassId:
			return std::to_string(obj->f);
		case Autolang::DefaultClass::stringClassId:
			return std::string(obj->str->data);
		case Autolang::DefaultClass::nullClassId:
			return "null";
		case DefaultClass::boolClassId:
			return (obj == DefaultClass::trueObject ? "true" : "false");
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
		case Autolang::DefaultClass::floatClassId:
			return notifier.createInt(static_cast<int64_t>(obj->f));
		case Autolang::DefaultClass::boolClassId:
			return notifier.createInt(static_cast<int64_t>(obj->b));
		case Autolang::DefaultClass::stringClassId: {
			char *end;
			return notifier.createInt(std::strtoll(obj->str->data, &end, 10));
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

inline AObject *to_string(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			return notifier.createString(AString::from(obj->i));
		case Autolang::DefaultClass::floatClassId:
			return notifier.createString(AString::from(obj->f));
		case Autolang::DefaultClass::stringClassId:
			return notifier.createString(AString::copy(obj->str));
		case Autolang::DefaultClass::boolClassId:
			return notifier.createString(obj == Autolang::DefaultClass::trueObject ? "true" : "false");
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
	return notifier.createFloat(std::sqrt(args[0]->f));
}

inline AObject *float_pow(NativeFuncInData) {
	double base = args[0]->f;
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

} // namespace DefaultFunction
} // namespace Autolang

#endif
