#ifndef JS_FUNCTION_HPP
#define JS_FUNCTION_HPP

#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AObject.hpp"
#include "shared/Type.hpp"
#include <emscripten/bind.h>

using namespace emscripten;
using namespace AutoLang::Libs::set;
using namespace AutoLang::Libs::map;

namespace AutoLang {

inline emscripten::val aobjectToJs(AObject *obj);

inline emscripten::val aobjectSetToJS(AObject *obj) {
	using namespace emscripten;

	val arr = val::array();

	auto unorderedSetData = static_cast<AUnorderedSet *>(obj->data->data);
	int idx = 0;

	switch (unorderedSetData->type) {

		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t v : *set) {
				arr.set(idx++, static_cast<double>(v));
			}
			break;
		}

		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double v : *set) {
				arr.set(idx++, v);
			}
			break;
		}

		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			for (AObject *v : *set) {
				arr.set(idx++, std::string(v->str->data));
			}
			break;
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (AObject *v : *set) {
				arr.set(idx++, aobjectToJs(v));
			}
			break;
		}
	}

	return val::global("Set").new_(arr);
}

inline emscripten::val aobjectMapToJS(AObject *obj) {
	using namespace emscripten;

	val entries = val::array();
	int idx = 0;

	auto hashMapData = static_cast<AHashMap *>(obj->data->data);

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, static_cast<double>(k));
				pair.set(1, aobjectToJs(v));
				entries.set(idx++, pair);
			}
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, k);
				pair.set(1, aobjectToJs(v));
				entries.set(idx++, pair);
			}
			break;
		}

		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, std::string(k->str->data));
				pair.set(1, aobjectToJs(v));
				entries.set(idx++, pair);
			}
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, aobjectToJs(k));
				pair.set(1, aobjectToJs(v));
				entries.set(idx++, pair);
			}
			break;
		}
	}

	return val::global("Map").new_(entries);
}

inline val aobjectToJs(AObject *obj) {
	switch (obj->type) {
		case DefaultClass::intClassId: {
			return val(static_cast<double>(obj->i));
		}
		case DefaultClass::floatClassId: {
			return val(obj->f);
		}
		case DefaultClass::stringClassId: {
			return val(std::string(obj->str->data));
		}
		case DefaultClass::boolClassId: {
			return val((bool)obj->b);
		}
		case DefaultClass::nullClassId: {
			return val::null();
		}
		default: {
			if (obj->flags & AObject::Flags::OBJ_IS_ARRAY) {
				val arr = val::array();
				for (size_t i = 0; i < obj->member->size; ++i) {
					arr.call<void>("push", aobjectToJs(obj->member->data[i]));
				}
				return arr;
			}
			if (obj->flags & AObject::Flags::OBJ_IS_SET) {
				return aobjectSetToJS(obj);
			}
			if (obj->flags & AObject::Flags::OBJ_IS_MAP) {
				return aobjectMapToJS(obj);
			}
			return val(static_cast<double>(reinterpret_cast<uintptr_t>(obj)));
		}
	}
}

inline AObject *returnJsObjectToAObject(ANotifier &notifier, val value) {

	ClassId classId = notifier.callFrame->func->returnId;
	auto flags = notifier.callFrame->func->functionFlags;

	if (value.isUndefined()) {
		if (classId == DefaultClass::voidClassId) {
			return nullptr;
		}

		notifier.throwException(
		    "JS function returned undefined (missing return?)");
		return nullptr;
	}

	if (value.isNull()) {
		if (classId == DefaultClass::voidClassId) {
			notifier.throwException(
			    "Void function should not return a value (got null)");
			return nullptr;
		}

		if (flags & FunctionFlags::FUNC_RETURN_NULLABLE) {
			return DefaultClass::nullObject;
		}

		notifier.throwException("Expected non-null value");
		return nullptr;
	}

	switch (classId) {

		case DefaultClass::intClassId: {
			if (!value.isNumber()) {
				notifier.throwException("Expected Int (number)");
				return nullptr;
			}
			double v = value.as<double>();

			if (floor(v) != v) {
				notifier.throwException("Expected Int but got Float");
				return nullptr;
			}

			return notifier.createInt((int64_t)v);
		}

		case DefaultClass::floatClassId: {
			if (!value.isNumber()) {
				notifier.throwException("Expected Float (number)");
				return nullptr;
			}
			return notifier.createFloat(value.as<double>());
		}

		case DefaultClass::stringClassId: {
			if (!value.isString()) {
				notifier.throwException("Expected String");
				return nullptr;
			}
			return notifier.createString(value.as<std::string>());
		}

		case DefaultClass::boolClassId: {
			if (!(value.isTrue() || value.isFalse())) {
				notifier.throwException("Expected Bool");
				return nullptr;
			}
			return notifier.createBool(value.as<bool>());
		}

		case DefaultClass::voidClassId: {
			notifier.throwException("Void function should not return a value");
			return nullptr;
		}

		default: {
			notifier.throwException("Unsupported JS return type");
			return nullptr;
		}
	}
}

inline AObject *callJSFunction(val *jsFunction, NativeFuncInData) {
	val jsArgsArray = val::array();

	if (notifier.callFrame->func->functionFlags &
	    FunctionFlags::FUNC_IS_STATIC) {
		for (size_t i = 0; i < argSize; ++i) {
			jsArgsArray.set(i, aobjectToJs(args[i]));
		}

		val result =
		    jsFunction->call<val>("apply", val::undefined(), jsArgsArray);

		if (notifier.hasException()) {
			return nullptr;
		}

		return returnJsObjectToAObject(notifier, result);
	}

	for (size_t i = 1; i < argSize; ++i) {
		jsArgsArray.set(i - 1, aobjectToJs(args[i]));
	}
	val result =
	    jsFunction->call<val>("apply", aobjectToJs(args[0]), jsArgsArray);

	if (notifier.hasException()) {
		return nullptr;
	}

	return returnJsObjectToAObject(notifier, result);
}

} // namespace AutoLang

#endif