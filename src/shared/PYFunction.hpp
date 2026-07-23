#ifndef PY_FUNCTION_HPP
#define PY_FUNCTION_HPP

#include "DefaultClass.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AObject.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/Type.hpp"
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <string_view>

namespace py = pybind11;
using namespace Autolang::Libs::set;
using namespace Autolang::Libs::map;

namespace Autolang {

inline py::object aobjectToPy(ANotifier &notifier, AObject *obj);
inline AObject *pyValueToAObject(ANotifier &notifier, const py::object &value,
                                 ClassId elemClassId);
inline AObject *pySetToAObject(ANotifier &notifier, const py::object &pySet,
                               ClassId setClassId, ClassId elemClassId,
                               bool elemNullable);
inline AObject *pyArrayToAObject(ANotifier &notifier, const py::object &pyArray,
                                 ClassId arrayClassId, ClassId elemClassId,
                                 bool elemNullable);

inline py::object aobjectSetToPy(ANotifier &notifier, AObject *obj) {
	py::set pySet;

	auto unorderedSetData = static_cast<AUnorderedSet *>(obj->data->data);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t v : *set)
				pySet.add(py::int_(v));
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double v : *set)
				pySet.add(py::float_(v));
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			for (AObject *v : *set)
				pySet.add(py::str(v->str->data));
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (AObject *v : *set)
				pySet.add(aobjectToPy(notifier, v));
			break;
		}
	}

	return pySet;
}

inline py::object aobjectMapToPy(ANotifier &notifier, AObject *obj) {
	py::dict pyDict;

	auto hashMapData = static_cast<AHashMap *>(obj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				pyDict[py::int_(k)] = aobjectToPy(notifier, v);
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				pyDict[py::float_(k)] = aobjectToPy(notifier, v);
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				pyDict[py::str(k->str->data)] = aobjectToPy(notifier, v);
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				pyDict[aobjectToPy(notifier, k)] = aobjectToPy(notifier, v);
			break;
		}
	}

	return pyDict;
}

inline py::object aobjectToPy(ANotifier &notifier, AObject *obj) {
	if (!obj)
		return py::none();

	switch (obj->type) {
		case DefaultClass::intClassId: {
			return py::int_(obj->i);
		}
		case DefaultClass::floatClassId: {
			return py::float_(obj->f);
		}
		case DefaultClass::stringClassId: {
			return py::str(obj->str->data);
		}
		case DefaultClass::boolClassId: {
			return py::bool_((bool)obj->b);
		}
		case DefaultClass::nullClassId: {
			return py::none();
		}
		case DefaultClass::pyObjectClassId: {
			return *obj->pyObject;
		}
		case DefaultClass::bytesClassId: {
			if (obj->bytes->size == 0) {
				return py::bytes("");
			}
			return py::bytes(reinterpret_cast<const char *>(obj->bytes->data),
			                 obj->bytes->size);
		}
		case DefaultClass::functionClassId: {
			notifier.throwException(
			    "Unsupported cast AObject (Function) to Python Object");
			return py::none();
		}
		case DefaultClass::jsonClassId: {
			if (!obj->json)
				return py::none();
			std::string dumpStr = obj->json->dump(-1);
			py::module_ json_mod = py::module_::import("json");
			return json_mod.attr("loads")(dumpStr);
		}
		default: {
			if (obj->flags & AObject::Flags::OBJ_IS_ARRAY) {
				py::list arr;
				for (size_t i = 0; i < obj->member->size; ++i) {
					arr.append(aobjectToPy(notifier, obj->member->data[i]));
				}
				return arr;
			}
			if (obj->flags & AObject::Flags::OBJ_IS_SET) {
				return aobjectSetToPy(notifier, obj);
			}
			if (obj->flags & AObject::Flags::OBJ_IS_MAP) {
				return aobjectMapToPy(notifier, obj);
			}

			auto clazz = notifier.vm->data.classes[obj->type];

			if (clazz->classFlags & ClassFlags::CLASS_IS_ENUM) {
				notifier.throwException(
				    "Unsupported cast AObject (Enum) to Python Object");
				return py::none();
			}

			py::dict pyObj;
			for (const auto &[memberName, memberPos] : clazz->memberMap) {
				AObject *memberVal = obj->member->data[memberPos];
				pyObj[py::str(memberName)] = aobjectToPy(notifier, memberVal);
			}
			return pyObj;
		}
	}
}

inline AObject *pyArrayToAObject(ANotifier &notifier, const py::object &pyArray,
                                 ClassId arrayClassId, ClassId elemClassId,
                                 bool elemNullable) {
	if (!py::isinstance<py::list>(pyArray) &&
	    !py::isinstance<py::tuple>(pyArray)) {
		notifier.throwException("Expected Python List or Tuple");
		return nullptr;
	}

	size_t len = py::len(pyArray);
	AObject *newArr = notifier.createArray(arrayClassId);

	for (size_t i = 0; i < len; ++i) {
		py::object item = pyArray[py::int_(i)];
		AObject *elem = pyValueToAObject(notifier, item, elemClassId);

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

inline AObject *pySetToAObject(ANotifier &notifier, const py::object &pySet,
                               ClassId setClassId, ClassId elemClassId,
                               bool elemNullable) {
	if (!py::isinstance<py::set>(pySet)) {
		notifier.throwException("Expected Python Set");
		return nullptr;
	}

	ClassId storageKeyId =
	    elemNullable ? DefaultClass::anyClassId : elemClassId;
	AObject *newSet =
	    Autolang::Libs::set::constructor(notifier, setClassId, storageKeyId);
	auto unorderedSetData = static_cast<AUnorderedSet *>(newSet->data->data);

	for (py::handle item_handle : pySet) {
		py::object item = py::reinterpret_borrow<py::object>(item_handle);
		AObject *elem = pyValueToAObject(notifier, item, elemClassId);

		if (notifier.hasException()) {
			notifier.release(newSet);
			return nullptr;
		}

		if (elem == DefaultClass::nullObject && !elemNullable) {
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
				obj->insert(elem);
				elem->retain();
				break;
			}
		}
	}

	return newSet;
}

inline AObject *pyMapToAObject(ANotifier &notifier, const py::object &pyMapObj,
                               ClassId mapClassId, ClassId keyClassId,
                               bool keyNullable, ClassId valueClassId,
                               bool valueNullable) {
	if (!py::isinstance<py::dict>(pyMapObj)) {
		notifier.throwException("Expected Python Dict");
		return nullptr;
	}

	py::dict pyMap = pyMapObj.cast<py::dict>();
	ClassId storageKeyId = keyNullable ? DefaultClass::anyClassId : keyClassId;

	AObject *newMap =
	    Autolang::Libs::map::constructor(notifier, mapClassId, storageKeyId);
	auto hashMapData = static_cast<AHashMap *>(newMap->data->data);

	for (auto item : pyMap) {
		py::object pyKey = py::reinterpret_borrow<py::object>(item.first);
		py::object pyVal = py::reinterpret_borrow<py::object>(item.second);

		AObject *keyObj = pyValueToAObject(notifier, pyKey, keyClassId);
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

		AObject *valObj = pyValueToAObject(notifier, pyVal, valueClassId);
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
	}

	return newMap;
}

inline AObject *pyObjectToAObject(ANotifier &notifier, const py::object &pyObj,
                                  ClassId classId) {
	if (pyObj.is_none()) {
		notifier.throwException("Expected Python Object (dict or instance)");
		return nullptr;
	}

	auto clazz = notifier.vm->data.classes[classId];
	auto newObj = notifier.createMemberObject(classId, clazz->memberMap.size());
	ClassId *memberId = &notifier.vm->data.allMemberId[clazz->memberIdOffset];
	size_t nullableOffset = clazz->memberIdOffset;

	bool isDict = py::isinstance<py::dict>(pyObj);
	py::dict pyDict = isDict ? pyObj.cast<py::dict>() : py::dict();

	for (const auto &[memberName, memberPos] : clazz->memberMap) {
		ClassId memberClassId = memberId[memberPos];
		bool isNullable =
		    notifier.vm->data.allMemberNullable[nullableOffset + memberPos];

		py::object fieldVal = py::none();

		if (isDict) {
			if (pyDict.contains(py::str(memberName))) {
				fieldVal = pyDict[py::str(memberName)];
			}
		} else {
			if (py::hasattr(pyObj, memberName.c_str())) {
				fieldVal = py::getattr(pyObj, memberName.c_str());
			}
		}

		if (fieldVal.is_none()) {
			if (isNullable) {
				newObj->member->data[memberPos] = notifier.createNull();
			} else {
				notifier.throwException(
				    "Python Object missing required non-nullable field: " +
				    memberName);
				notifier.release(newObj);
				return nullptr;
			}
		} else {
			AObject *val = pyValueToAObject(notifier, fieldVal, memberClassId);

			if (notifier.hasException() || !val) {
				notifier.release(newObj);
				return nullptr;
			}

			if (val == DefaultClass::nullObject && !isNullable) {
				notifier.throwException("Python Object field cannot be null "
				                        "(field is non-nullable): " +
				                        memberName);
				notifier.release(newObj);
				return nullptr;
			}

			newObj->member->data[memberPos] = val;
		}
	}
	return newObj;
}

inline AObject *pyObjectToJson(ANotifier &notifier, const py::object &pyObj) {
	if (pyObj.is_none()) {
		auto newObj = notifier.createObject(DefaultClass::jsonClassId);
		newObj->json = new nlohmann::json(nullptr);
		return newObj;
	}

	try {
		py::module_ json_mod = py::module_::import("json");
		py::object jsonStringVal = json_mod.attr("dumps")(pyObj);

		std::string jsonStr = jsonStringVal.cast<std::string>();
		auto parsed = new nlohmann::json(nlohmann::json::parse(jsonStr));
		auto newObj = notifier.createObject(DefaultClass::jsonClassId);
		newObj->json = parsed;
		return newObj;

	} catch (const py::error_already_set &e) {
		notifier.throwException(
		    std::string("Cannot stringify Python Object to JSON: ") + e.what());
		return nullptr;
	} catch (const nlohmann::json::exception &e) {
		notifier.throwException(
		    std::string("JSON Parse Error from Python Object: ") + e.what());
		return nullptr;
	}
}

inline AObject *pyValueToAObject(ANotifier &notifier, const py::object &value,
                                 ClassId elemClassId) {
	if (value.is_none()) {
		return DefaultClass::nullObject;
	}

	switch (elemClassId) {
		case DefaultClass::intClassId: {
			if (!py::isinstance<py::int_>(value)) {
				notifier.throwException("Expected Int in Python object");
				return nullptr;
			}
			return notifier.createInt(value.cast<int64_t>());
		}
		case DefaultClass::floatClassId: {
			if (!py::isinstance<py::float_>(value) &&
			    !py::isinstance<py::int_>(value)) {
				notifier.throwException("Expected Float in Python object");
				return nullptr;
			}
			return notifier.createFloat(value.cast<double>());
		}
		case DefaultClass::stringClassId: {
			if (!py::isinstance<py::str>(value)) {
				notifier.throwException("Expected String in Python object");
				return nullptr;
			}
			return notifier.createString(value.cast<std::string>());
		}
		case DefaultClass::boolClassId: {
			if (!py::isinstance<py::bool_>(value)) {
				notifier.throwException("Expected Bool in Python object");
				return nullptr;
			}
			return notifier.createBool(value.cast<bool>());
		}
		case DefaultClass::jsonClassId: {
			return pyObjectToJson(notifier, value);
		}
		case DefaultClass::bytesClassId: {
			if (!py::isinstance<py::bytes>(value) &&
			    !py::isinstance<py::bytearray>(value)) {
				notifier.throwException("Expected bytes or bytearray");
				return nullptr;
			}
			std::string_view byteView = value.cast<std::string_view>();
			AObject *bytesObj = notifier.createBytes(byteView.size());

			if (!byteView.empty()) {
				std::memcpy(bytesObj->bytes->data, byteView.data(),
				            byteView.size());
				bytesObj->bytes->size = byteView.size();
			}
			return bytesObj;
		}
		case DefaultClass::nullClassId: {
			notifier.throwException("Expected None");
			return nullptr;
		}
		case DefaultClass::anyClassId:
		case DefaultClass::functionClassId: {
			notifier.throwException("Unsupported Python object type to " +
			                        notifier.getClassName(elemClassId));
			return nullptr;
		}
		default: {
			auto clazz = notifier.vm->data.classes[elemClassId];
			if (clazz->classFlags & ClassFlags::CLASS_IS_ENUM) {
				notifier.throwException(
				    "Unsupported Python return type (Enum)");
				return nullptr;
			}

			if (clazz->classFlags & ClassFlags::CLASS_PY_OBJECT) {
				return notifier.getPyObject(DefaultClass::pyObjectClassId,
				                            new py::object(value));
			}

			switch (clazz->genericBaseClassId) {
				case DefaultClass::arrayClassId: {
					ClassId genericElemClassId =
					    notifier.vm->data
					        .allGenericType[clazz->genericType.offset];
					bool genericElemNullable =
					    notifier.vm->data
					        .allGenericTypeNullable[clazz->genericType.offset];
					return pyArrayToAObject(notifier, value, elemClassId,
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
					return pySetToAObject(notifier, value, elemClassId,
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
					return pyMapToAObject(notifier, value, elemClassId,
					                      genericKeyClassId, genericKeyNullable,
					                      genericValueClassId,
					                      genericValueNullable);
				}
				default: {
					return pyObjectToAObject(notifier, value, elemClassId);
				}
			}
			notifier.throwException("Unsupported Python Object to '" +
			                        notifier.getClassName(elemClassId) + "'");
			return nullptr;
		}
	}
}

inline AObject *returnPyObjectToAObject(ANotifier &notifier,
                                        const py::object &value) {
	ClassId classId = notifier.callFrame->func->returnId;
	auto flags = notifier.callFrame->func->functionFlags;

	if (!value || value.is_none()) {
		if (classId == DefaultClass::voidClassId) {
			return nullptr;
		}
		if (flags & FunctionFlags::FUNC_RETURN_NULLABLE) {
			return DefaultClass::nullObject;
		}
		notifier.throwException("Expected non-null value (got None)");
		return nullptr;
	}

	switch (classId) {
		case DefaultClass::voidClassId: {
			notifier.throwException("Void function should not return a value");
			return nullptr;
		}
		default: {
			return pyValueToAObject(notifier, value, classId);
		}
	}
}

inline AObject *callPyFunction(py::object *pyFunction, NativeFuncInData) {
	py::tuple pyArgsTuple;
	py::object pyThis = py::none();
	size_t argsStartIndex = 0;

	if (notifier.callFrame->func->functionFlags &
	    FunctionFlags::FUNC_IS_STATIC) {
		pyArgsTuple = py::tuple(argSize);
		argsStartIndex = 0;
	} else {
		pyArgsTuple = py::tuple(argSize - 1);
		argsStartIndex = 1;
		pyThis = aobjectToPy(notifier, args[0]);
		if (notifier.hasException())
			return nullptr;
	}

	for (size_t i = argsStartIndex; i < argSize; ++i) {
		py::object pyArg = aobjectToPy(notifier, args[i]);
		if (notifier.hasException())
			return nullptr;
		pyArgsTuple[i - argsStartIndex] = pyArg;
	}

	try {
		py::object response;
		if (argsStartIndex == 1) {
			response = (*pyFunction)(pyThis, *pyArgsTuple);
		} else { // Static function call
			response = (*pyFunction)(*pyArgsTuple);
		}

		return returnPyObjectToAObject(notifier, response);

	} catch (const py::error_already_set &e) {
		notifier.throwException("PythonException: " + std::string(e.what()));
		return nullptr;
	}
}

} // namespace Autolang

#endif