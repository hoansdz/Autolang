#ifndef CONVERSION_FUNCTIONS_HPP
#define CONVERSION_FUNCTIONS_HPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include <cstdlib>
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
		default:
			break;
	}
	notifier.throwException("Cannot cast '" + notifier.getClassName(obj->type) +
	                        "' to 'String'");
	return nullptr;
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
