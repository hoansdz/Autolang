#ifndef JS_FUNCTION_HPP
#define JS_FUNCTION_HPP

#include "DefaultClass.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AObject.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/Type.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;
using namespace Autolang::Libs::set;
using namespace Autolang::Libs::map;

namespace Autolang {

inline emscripten::val aobjectToJs(ANotifier &notifier, AObject *obj);
inline AObject *jsValueToAObject(ANotifier &notifier, const val &value,
                                 ClassId elemClassId);
inline AObject *jsSetToAObject(ANotifier &notifier, const val &jsSet,
                               ClassId setClassId, ClassId elemClassId,
                               bool elemNullable);
inline AObject *jsArrayToAObject(ANotifier &notifier, const val &jsArray,
                                 ClassId arrayClassId, ClassId elemClassId,
                                 bool elemNullable);

inline emscripten::val aobjectSetToJS(ANotifier &notifier, AObject *obj) {
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
				arr.set(idx++, aobjectToJs(notifier, v));
			}
			break;
		}
	}

	return val::global("Set").new_(arr);
}

inline emscripten::val aobjectMapToJS(ANotifier &notifier, AObject *obj) {
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
				pair.set(1, aobjectToJs(notifier, v));
				entries.set(idx++, pair);
			}
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, k);
				pair.set(1, aobjectToJs(notifier, v));
				entries.set(idx++, pair);
			}
			break;
		}

		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, std::string(k->str->data));
				pair.set(1, aobjectToJs(notifier, v));
				entries.set(idx++, pair);
			}
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				val pair = val::array();
				pair.set(0, aobjectToJs(notifier, k));
				pair.set(1, aobjectToJs(notifier, v));
				entries.set(idx++, pair);
			}
			break;
		}
	}

	return val::global("Map").new_(entries);
}

inline val aobjectToJs(ANotifier &notifier, AObject *obj) {
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
		case DefaultClass::functionClassId: {
			notifier.throwException(
			    "Unsupported JS conversion: Cannot cast Function to JS Object");
			return val::undefined();
		}
		case DefaultClass::bytesClassId: {
			if (obj->bytes->size == 0) {
				return val::global("Uint8Array").new_();
			}
			val view = emscripten::val(emscripten::typed_memory_view(
			    obj->bytes->size, obj->bytes->data));
			return val::global("Uint8Array").new_(view);
		}
#ifdef __EMSCRIPTEN__
		case DefaultClass::jsObjectClassId: {
			return *obj->jsObject;
		}
#endif
		case DefaultClass::jsonClassId: {
			if (!obj->json)
				return val::null();
			std::string dumpStr = obj->json->dump(-1);
			return val::global("JSON").call<val>("parse", dumpStr);
		}
		default: {
			if (obj->flags & AObject::Flags::OBJ_IS_ARRAY) {
				val arr = val::array();
				for (size_t i = 0; i < obj->member->size; ++i) {
					arr.call<void>("push",
					               aobjectToJs(notifier, obj->member->data[i]));
					if (notifier.hasException())
						return val::undefined();
				}
				return arr;
			}
			if (obj->flags & AObject::Flags::OBJ_IS_SET) {
				return aobjectSetToJS(notifier, obj);
			}
			if (obj->flags & AObject::Flags::OBJ_IS_MAP) {
				return aobjectMapToJS(notifier, obj);
			}

			auto clazz = notifier.vm->data.classes[obj->type];

			if (clazz->classFlags & ClassFlags::CLASS_IS_ENUM) {
				notifier.throwException(
				    "Unsupported JS conversion: Cannot cast Enum to JS Object");
				return val::undefined();
			}

			val jsObj = val::object();

			for (const auto &[memberName, memberPos] : clazz->memberMap) {
				AObject *memberVal = obj->member->data[memberPos];
				jsObj.set(memberName, aobjectToJs(notifier, memberVal));

				if (notifier.hasException()) {
					return val::undefined();
				}
			}
			return jsObj;
		}
	}
}

inline AObject *jsArrayToAObject(ANotifier &notifier, const val &jsArray,
                                 ClassId arrayClassId, ClassId elemClassId,
                                 bool elemNullable) {
	if (!jsArray.instanceof(val::global("Array"))) {
		notifier.throwException("Expected JS Array");
		return nullptr;
	}

	unsigned len = jsArray["length"].as<unsigned>();

	AObject *newArr = notifier.createArray(arrayClassId);

	for (unsigned i = 0; i < len; ++i) {
		val item = jsArray[i];
		AObject *elem = jsValueToAObject(notifier, item, elemClassId);

		if (notifier.hasException()) {
			notifier.release(newArr);
			return nullptr;
		}

		if (elem == DefaultClass::nullObject && !elemNullable) {
			notifier.throwException(
			    "Array element cannot be null (element type is non-nullable)");
			notifier.release(newArr);
			return nullptr;
		}

		notifier.arrayAdd(newArr, elem);
	}

	return newArr;
}

inline AObject *jsSetToAObject(ANotifier &notifier, const val &jsSet,
                               ClassId setClassId, ClassId elemClassId,
                               bool elemNullable) {
	if (!jsSet.instanceof(val::global("Set"))) {
		notifier.throwException("Expected JS Set");
		return nullptr;
	}

	ClassId storageKeyId =
	    elemNullable ? DefaultClass::anyClassId : elemClassId;

	AObject *newSet =
	    Autolang::Libs::set::constructor(notifier, setClassId, storageKeyId);
	auto unorderedSetData = static_cast<AUnorderedSet *>(newSet->data->data);

	val iterator = jsSet.call<val>("values");
	val next = iterator.call<val>("next");

	while (!next["done"].as<bool>()) {
		val item = next["value"];
		AObject *elem = jsValueToAObject(notifier, item, elemClassId);

		if (notifier.hasException()) {
			notifier.release(newSet);
			return nullptr;
		}

		if (elem == DefaultClass::nullObject && !elemNullable) {
			// Null, Bool value won't never be changed although call retain() or
			// release()
			notifier.throwException(
			    "Set element cannot be null (element type is non-nullable)");
			notifier.release(newSet);
			return nullptr;
		}

		switch (storageKeyId) {
			case DefaultClass::intClassId: {
				auto set = static_cast<IntHashSet *>(unorderedSetData->data);
				set->insert(elem->i);
				notifier.release(elem);
				break;
			}
			case DefaultClass::floatClassId: {
				auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
				set->insert(elem->f);
				notifier.release(elem);
				break;
			}
			case DefaultClass::stringClassId: {
				auto set = static_cast<StringHashSet *>(unorderedSetData->data);
				set->insert(elem);
				elem->retain();
				break;
			}
			default: {
				auto obj = static_cast<ObjectHashSet *>(unorderedSetData->data);
				obj->insert(elem); // HashSet won't never call retain()
				elem->retain();
				break;
			}
		}
		next = iterator.call<val>("next");
	}

	return newSet;
}

inline AObject *jsMapToAObject(ANotifier &notifier, const val &jsMap,
                               ClassId mapClassId, ClassId keyClassId,
                               bool keyNullable, ClassId valueClassId,
                               bool valueNullable) {
	if (!jsMap.instanceof(val::global("Map"))) {
		notifier.throwException("Expected JS Map");
		return nullptr;
	}

	ClassId storageKeyId = keyNullable ? DefaultClass::anyClassId : keyClassId;

	AObject *newMap =
	    Autolang::Libs::map::constructor(notifier, mapClassId, storageKeyId);
	auto hashMapData = static_cast<AHashMap *>(newMap->data->data);

	val iterator = jsMap.call<val>("entries");
	val next = iterator.call<val>("next");

	while (!next["done"].as<bool>()) {
		val entry = next["value"];
		val jsKey = entry[0];
		val jsVal = entry[1];

		AObject *keyObj = jsValueToAObject(notifier, jsKey, keyClassId);
		if (notifier.hasException()) {
			notifier.release(newMap);
			return nullptr;
		}

		if (keyObj == DefaultClass::nullObject && !keyNullable) {
			notifier.throwException(
			    "Map key cannot be null (key type is non-nullable)");
			notifier.release(newMap);
			return nullptr;
		}

		AObject *valObj = jsValueToAObject(notifier, jsVal, valueClassId);
		if (notifier.hasException()) {
			notifier.release(keyObj);
			notifier.release(newMap);
			return nullptr;
		}

		if (valObj == DefaultClass::nullObject && !valueNullable) {
			notifier.throwException(
			    "Map value cannot be null (value type is non-nullable)");
			notifier.release(keyObj);
			notifier.release(newMap);
			return nullptr;
		}

		switch (storageKeyId) {
			case DefaultClass::intClassId: {
				auto map = static_cast<IntHashMap *>(hashMapData->data);
				(*map)[keyObj->i] = valObj;
				valObj->retain();
				notifier.release(keyObj);
				break;
			}
			case DefaultClass::floatClassId: {
				auto map = static_cast<FloatHashMap *>(hashMapData->data);
				(*map)[keyObj->f] = valObj;
				valObj->retain();
				notifier.release(keyObj);
				break;
			}
			case DefaultClass::stringClassId: {
				auto map = static_cast<StringHashMap *>(hashMapData->data);
				(*map)[keyObj] = valObj;
				keyObj->retain();
				valObj->retain();
				break;
			}
			default: {
				auto map = static_cast<ObjectHashMap *>(hashMapData->data);
				(*map)[keyObj] = valObj;
				keyObj->retain();
				valObj->retain();
				break;
			}
		}

		next = iterator.call<val>("next");
	}

	return newMap;
}

inline AObject *jsObjectToAObject(ANotifier &notifier, const val &jsObj,
                                  ClassId classId) {
	if (jsObj.isNull() || jsObj.isUndefined() ||
	    jsObj.typeOf().as<std::string>() != "object") {
		notifier.throwException("Expected JS Object");
		return nullptr;
	}

	auto clazz = notifier.vm->data.classes[classId];
	auto newObj = notifier.createMemberObject(classId, clazz->memberMap.size());
	ClassId *memberId = &notifier.vm->data.getMemberRef(clazz->memberIdOffset);
	size_t nullableOffset = clazz->memberIdOffset;

	for (const auto &[memberName, memberPos] : clazz->memberMap) {
		ClassId memberClassId = memberId[memberPos];
		bool isNullable =
		    notifier.vm->data.allMemberNullable[nullableOffset + memberPos];

		val fieldVal = jsObj[memberName];

		if (fieldVal.isUndefined() || fieldVal.isNull()) {
			if (isNullable) {
				newObj->member->data[memberPos] = notifier.createNull();
			} else {
				notifier.throwException(
				    "JS Object missing required non-nullable field: " +
				    memberName);
				notifier.release(newObj);
				return nullptr;
			}
		} else {
			AObject *val = jsValueToAObject(notifier, fieldVal, memberClassId);

			if (notifier.hasException() || !val) {
				notifier.release(newObj);
				return nullptr;
			}

			if (val == DefaultClass::nullObject && !isNullable) {
				notifier.throwException(
				    "JS Object field cannot be null (field is non-nullable): " +
				    memberName);
				notifier.release(newObj);
				return nullptr;
			}

			newObj->member->data[memberPos] = val;
		}
	}
	return newObj;
}

inline AObject *jsObjectToJson(ANotifier &notifier, const val &jsObj) {
	if (jsObj.isNull() || jsObj.isUndefined()) {
		auto newObj = notifier.createObject(DefaultClass::jsonClassId);
		newObj->json = new nlohmann::json(nullptr);
		return newObj;
	}

	val JSON = val::global("JSON");
	val jsonStringVal = JSON.call<val>("stringify", jsObj);

	if (jsonStringVal.isUndefined()) {
		notifier.throwException("Cannot stringify JS Object to JSON");
		return nullptr;
	}

	std::string jsonStr = jsonStringVal.as<std::string>();

	try {
		auto parsed = new nlohmann::json(nlohmann::json::parse(jsonStr));
		auto newObj = notifier.createObject(DefaultClass::jsonClassId);
		newObj->json = parsed;
		return newObj;
	} catch (const nlohmann::json::exception &e) {
		notifier.throwException(
		    std::string("JSON Parse Error from JS Object: ") + e.what());
		return nullptr;
	}
}

inline AObject *jsValueToAObject(ANotifier &notifier, const val &value,
                                 ClassId elemClassId) {
	if (value.isNull() || value.isUndefined()) {
		return DefaultClass::nullObject;
	}

	switch (elemClassId) {
		case DefaultClass::intClassId: {
			if (!value.isNumber()) {
				notifier.throwException(
				    "Expected Int (number) in array element");
				return nullptr;
			}
			double v = value.as<double>();
			if (floor(v) != v) {
				notifier.throwException(
				    "Expected Int but got Float in array element");
				return nullptr;
			}
			return notifier.createInt((int64_t)v);
		}
		case DefaultClass::floatClassId: {
			if (!value.isNumber()) {
				notifier.throwException(
				    "Expected Float (number) in array element");
				return nullptr;
			}
			return notifier.createFloat(value.as<double>());
		}
		case DefaultClass::stringClassId: {
			if (!value.isString()) {
				notifier.throwException("Expected String in array element");
				return nullptr;
			}
			return notifier.createString(value.as<std::string>());
		}
		case DefaultClass::boolClassId: {
			if (!(value.isTrue() || value.isFalse())) {
				notifier.throwException("Expected Bool in array element");
				return nullptr;
			}
			return notifier.createBool(value.as<bool>());
		}
		case DefaultClass::jsonClassId: {
			return jsObjectToJson(notifier, value);
		}
		case DefaultClass::nullClassId: {
			notifier.throwException("Expected null in array element");
			return nullptr;
		}
		case DefaultClass::functionClassId:
		case DefaultClass::anyClassId: {
			notifier.throwException("Unsupported Js Object to '" +
			                        notifier.getClassName(elemClassId) + "'");
			return nullptr;
		}
		case DefaultClass::bytesClassId: {
			if (!value.instanceof(val::global("Uint8Array"))) {
				notifier.throwException("Expected Uint8Array");
				return nullptr;
			}

			size_t size = value["length"].as<size_t>();
			AObject *bytesObj = notifier.createBytes(size);

			if (size > 0) {
				val heap = val::module_property("HEAPU8");
				heap.call<void>(
				    "set", value,
				    val(reinterpret_cast<uintptr_t>(bytesObj->bytes->data)));
				bytesObj->bytes->size = size;
			}

			return bytesObj;
		}
		default: {
			auto clazz = notifier.vm->data.classes[elemClassId];
			if (clazz->classFlags & ClassFlags::CLASS_IS_ENUM) {
				notifier.throwException("Unsupported JS return type");
				return nullptr;
			}

#ifdef __EMSCRIPTEN__
			if (clazz->classFlags & ClassFlags::CLASS_JS_OBJECT) {
				return notifier.getJsObject(DefaultClass::jsObjectClassId,
				                            new val(value));
			}
#endif
			switch (clazz->genericBaseClassId) {
				case DefaultClass::arrayClassId: {
					ClassId genericElemClassId =
					    notifier.vm->data
					        .allGenericType[clazz->genericType.offset];
					bool genericElemNullable =
					    notifier.vm->data
					        .allGenericTypeNullable[clazz->genericType.offset];

					return jsArrayToAObject(notifier, value, elemClassId,
					                        genericElemClassId,
					                        genericElemNullable);
				}
				case DefaultClass::setClassId: {
					ClassId genericElemClassId =
					    notifier.vm->data
					        .allGenericType[clazz->genericType.offset];
					bool genericElemNullable =
					    notifier.vm->data
					        .allGenericTypeNullable[clazz->genericType.offset];

					return jsSetToAObject(notifier, value, elemClassId,
					                      genericElemClassId,
					                      genericElemNullable);
				}
				case DefaultClass::mapClassId: {
					ClassId genericKeyClassId =
					    notifier.vm->data
					        .allGenericType[clazz->genericType.offset];
					bool genericKeyNullable =
					    notifier.vm->data
					        .allGenericTypeNullable[clazz->genericType.offset];
					ClassId genericValueClassId =
					    notifier.vm->data
					        .allGenericType[clazz->genericType.offset + 1];
					bool genericValueNullable =
					    notifier.vm->data
					        .allGenericTypeNullable[clazz->genericType.offset +
					                                1];
					return jsMapToAObject(notifier, value, elemClassId,
					                      genericKeyClassId, genericKeyNullable,
					                      genericValueClassId,
					                      genericValueNullable);
				}
				default: {
					return jsObjectToAObject(notifier, value, elemClassId);
					// notifier.throwException("Unsupported JS return type");
					// return nullptr;
				}
			}
			notifier.throwException("Unsupported Js Object to '" +
			                        notifier.getClassName(elemClassId) + "'");
			return nullptr;
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

		case DefaultClass::voidClassId: {
			notifier.throwException("Void function should not return a value");
			return nullptr;
		}

		default: {
			return jsValueToAObject(notifier, value, classId);
		}
	}
}

EM_ASYNC_JS(emscripten::EM_VAL, call_and_await_js,
            (EM_VAL func_handle, EM_VAL this_handle, EM_VAL args_handle), {
	            let js_func = Emval.toValue(func_handle);
	            let js_this = Emval.toValue(this_handle);
	            let js_args = Emval.toValue(args_handle);

	            try {
		            let result =
		                await Promise.resolve(js_func.apply(js_this, js_args));
		            return Emval.toHandle({success : true, value : result});
	            } catch (error) {
		            let fullErrorLog = (error instanceof Error && error.stack)
		                                   ? error.stack
		                                   : String(error);

		            return Emval.toHandle(
		                {success : false, error : fullErrorLog});
	            }
            });

inline AObject *callJSFunction(val *jsFunction, NativeFuncInData) {
	val jsArgsArray = val::array();

	if (notifier.callFrame->func->functionFlags &
	    FunctionFlags::FUNC_IS_STATIC) {
		for (size_t i = 0; i < argSize; ++i) {
			jsArgsArray.set(i, aobjectToJs(notifier, args[i]));
			if (notifier.hasException())
				return nullptr;
		}

		val jsUndefined = val::undefined();

		emscripten::EM_VAL raw_handle =
		    call_and_await_js(jsFunction->as_handle(), jsUndefined.as_handle(),
		                      jsArgsArray.as_handle());

		val response = emscripten::val::take_ownership(raw_handle);

		if (!response["success"].as<bool>()) {
			notifier.throwException("JSException: " +
			                        response["error"].as<std::string>());
			return nullptr;
		}

		return returnJsObjectToAObject(notifier, response["value"]);
	}

	for (size_t i = 1; i < argSize; ++i) {
		jsArgsArray.set(i - 1, aobjectToJs(notifier, args[i]));
		if (notifier.hasException())
			return nullptr;
	}

	val jsThis = aobjectToJs(notifier, args[0]);
	if (notifier.hasException())
		return nullptr;

	emscripten::EM_VAL raw_handle = call_and_await_js(
	    jsFunction->as_handle(), jsThis.as_handle(), jsArgsArray.as_handle());

	val response = emscripten::val::take_ownership(raw_handle);

	if (!response["success"].as<bool>()) {
		notifier.throwException("JSException: " +
		                        response["error"].as<std::string>());
		return nullptr;
	}

	return returnJsObjectToAObject(notifier, response["value"]);
}

} // namespace Autolang

#endif