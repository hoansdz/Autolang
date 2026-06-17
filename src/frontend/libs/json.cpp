#ifndef LIB_JSON_CPP
#define LIB_JSON_CPP

#include "json.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <third_party/nlohmann/json.hpp>
#include <string>

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace json {

struct AJsonHandle {
	nlohmann::json j;
};

static void destroyJson(ANotifier &notifier, void *jsonData) {
	auto handle = static_cast<AJsonHandle *>(jsonData);
	delete handle;
}

inline AObject *parse(NativeFuncInData) {
	ClassId classId = args[0]->i;
	const std::string &text = args[1]->str->data;

	try {
		nlohmann::json parsed = nlohmann::json::parse(text);
		auto handle = new AJsonHandle{parsed};
		return notifier.createNativeData(classId, handle, destroyJson);
	} catch (const nlohmann::json::exception &e) {
		notifier.throwException(std::string("JSON Parse Error: ") + e.what());
		return nullptr;
	}
}

inline AObject *empty_object(NativeFuncInData) {
	ClassId classId = args[0]->i;
	auto handle = new AJsonHandle{nlohmann::json::object()};
	return notifier.createNativeData(classId, handle, destroyJson);
}

inline AObject *empty_array(NativeFuncInData) {
	ClassId classId = args[0]->i;
	auto handle = new AJsonHandle{nlohmann::json::array()};
	return notifier.createNativeData(classId, handle, destroyJson);
}

inline AObject *from_string(NativeFuncInData) {
	ClassId classId = args[0]->i;
	const std::string &val = args[1]->str->data;
	return notifier.createNativeData(classId, new AJsonHandle{val},
	                                 destroyJson);
}

inline AObject *from_int(NativeFuncInData) {
	ClassId classId = args[0]->i;
	int64_t val = args[1]->i;
	return notifier.createNativeData(classId, new AJsonHandle{val},
	                                 destroyJson);
}

inline AObject *from_float(NativeFuncInData) {
	ClassId classId = args[0]->i;
	double val = args[1]->f;
	return notifier.createNativeData(classId, new AJsonHandle{val},
	                                 destroyJson);
}

inline AObject *from_bool(NativeFuncInData) {
	ClassId classId = args[0]->i;
	bool val = args[1]->b;
	return notifier.createNativeData(classId, new AJsonHandle{val},
	                                 destroyJson);
}

inline AObject *stringify(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	int indent = static_cast<int>(args[1]->i);

	std::string result = handle->j.dump(indent);
	return notifier.createString(result);
}

inline AObject *is_object(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_object());
}

inline AObject *is_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_array());
}

inline AObject *is_string(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_string());
}

inline AObject *is_number(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_number());
}

inline AObject *is_bool(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_boolean());
}

inline AObject *is_null(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createBool(handle->j.is_null());
}

inline AObject *get_size(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	return notifier.createInt(static_cast<int64_t>(handle->j.size()));
}

inline AObject *has_key(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	const std::string &key = args[1]->str->data;

	if (!handle->j.is_object())
		return notifier.createBool(false);
	return notifier.createBool(handle->j.contains(key));
}

inline AObject *get_field(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	const std::string &key = args[1]->str->data;

	if (!handle->j.is_object()) {
		notifier.throwException("JSON is not an object");
		return nullptr;
	}
	if (!handle->j.contains(key)) {
		notifier.throwException("JSON Object does not contain key: " + key);
		return nullptr;
	}

	auto j_copy = handle->j.at(key);
	return notifier.createNativeData(args[0]->type, new AJsonHandle{j_copy},
	                                 destroyJson);
}

inline AObject *set_field(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	const std::string &key = args[1]->str->data;
	auto valHandle = static_cast<AJsonHandle *>(args[2]->data->data);

	if (!handle->j.is_object()) {
		notifier.throwException("JSON is not an object");
		return nullptr;
	}

	handle->j[key] = valHandle->j;
	return nullptr;
}

inline AObject *get_index(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	size_t index = static_cast<size_t>(args[1]->i);

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}
	if (index >= handle->j.size()) {
		notifier.throwException("JSON Array index out of bounds");
		return nullptr;
	}

	auto j_copy = handle->j.at(index);
	return notifier.createNativeData(args[0]->type, new AJsonHandle{j_copy},
	                                 destroyJson);
}

inline AObject *add_element(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	auto valHandle = static_cast<AJsonHandle *>(args[1]->data->data);

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	handle->j.push_back(valHandle->j);
	return nullptr;
}

inline AObject *as_string(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	if (!handle->j.is_string()) {
		notifier.throwException("JSON value is not a String");
		return nullptr;
	}
	return notifier.createString(handle->j.get<std::string>());
}

inline AObject *as_int(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	if (!handle->j.is_number_integer()) {
		notifier.throwException("JSON value is not an Integer");
		return nullptr;
	}
	return notifier.createInt(handle->j.get<int64_t>());
}

inline AObject *as_float(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	if (!handle->j.is_number()) {
		notifier.throwException("JSON value is not a Number");
		return nullptr;
	}

	return notifier.createFloat(handle->j.get<double>());
}

inline AObject *as_bool(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	if (!handle->j.is_boolean()) {
		notifier.throwException("JSON value is not a Boolean");
		return nullptr;
	}
	return notifier.createBool(handle->j.get<bool>());
}

inline AObject *to_int_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId arrayClassId = args[1]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto newArr = notifier.createArray(arrayClassId);
	try {
		for (auto &element : handle->j) {
			if (!element.is_number_integer()) {
				notifier.throwException(
				    "Element " + std::to_string(notifier.getArraySize(newArr)) +
				    " in JSON array is not an integer");
				return nullptr;
			}
			notifier.arrayAdd(newArr,
			                  notifier.createInt(element.get<int64_t>()));
		}
	} catch (const std::exception &e) {
		notifier.throwException(std::string("JSON Error: ") + e.what());
		return nullptr;
	}
	return newArr;
}

inline AObject *to_float_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId arrayClassId = args[1]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto newArr = notifier.createArray(arrayClassId);
	try {
		for (auto &element : handle->j) {
			if (!element.is_number()) {
				notifier.throwException("Element " +
				                        std::to_string(newArr->member->size) +
				                        " in JSON array is not a number");
				return nullptr;
			}
			notifier.arrayAdd(newArr,
			                  notifier.createFloat(element.get<double>()));
		}
	} catch (const std::exception &e) {
		notifier.throwException(std::string("JSON Error: ") + e.what());
		return nullptr;
	}
	return newArr;
}

inline AObject *to_bool_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId arrayClassId = args[1]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto newArr = notifier.createArray(arrayClassId);
	try {
		for (auto &element : handle->j) {
			if (!element.is_boolean()) {
				notifier.throwException(
				    "Element " + std::to_string(notifier.getArraySize(newArr)) +
				    " in JSON array is not a boolean");
				return nullptr;
			}
			notifier.arrayAdd(newArr, notifier.createBool(element.get<bool>()));
		}
	} catch (const std::exception &e) {
		notifier.throwException(std::string("JSON Error: ") + e.what());
		return nullptr;
	}
	return newArr;
}

inline AObject *to_string_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId arrayClassId = args[1]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto newArr = notifier.createArray(arrayClassId);
	try {
		int i = 0;
		for (auto &element : handle->j) {
			if (!element.is_string()) {
				notifier.throwException(
				    "Element " + std::to_string(notifier.getArraySize(newArr)) +
				    " in JSON array is not a string");
				return nullptr;
			}
			notifier.arrayAdd(
			    newArr, notifier.createString(element.get<std::string>()));
		}
	} catch (const std::exception &e) {
		notifier.throwException(std::string("JSON Error: ") + e.what());
		return nullptr;
	}
	return newArr;
}

inline AObject *to_json_array(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId arrayClassId = args[1]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto newArr = notifier.createArray(arrayClassId);
	try {
		for (auto &element : handle->j) {
			notifier.arrayAdd(
			    newArr, notifier.createNativeData(args[0]->type,
			                                      new AJsonHandle{element},
			                                      destroyJson));
		}
	} catch (const std::exception &e) {
		notifier.throwException(std::string("JSON Error: ") + e.what());
		return nullptr;
	}
	return newArr;
}

inline AObject *json_to_class(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId classId = args[1]->i;
	auto clazz = notifier.vm->data.classes[classId];

	if (!handle->j.is_object()) {
		notifier.throwException("JSON is not an object");
		return nullptr;
	}

	auto newObj = notifier.createMemberObject(classId, clazz->memberId.size());

	for (const auto &[memberName, memberPos] : clazz->memberMap) {
		int memberClassId = clazz->memberId[memberPos];

		if (!handle->j.contains(memberName)) {
			notifier.throwException("JSON missing field: " + memberName);
			return nullptr;
		}

		const auto &field = handle->j.at(memberName);

		try {
			switch (memberClassId) {
				case AutoLang::DefaultClass::intClassId:
					if (!field.is_number_integer()) {
						notifier.throwException("Field '" + memberName +
						                        "' is not an Int");
						return nullptr;
					}
					newObj->member->data[memberPos] =
					    notifier.createInt(field.get<int64_t>());
					break;

				case AutoLang::DefaultClass::floatClassId:
					if (!field.is_number()) {
						notifier.throwException("Field '" + memberName +
						                        "' is not a Float");
						return nullptr;
					}
					newObj->member->data[memberPos] =
					    notifier.createFloat(field.get<double>());
					break;

				case AutoLang::DefaultClass::stringClassId:
					if (!field.is_string()) {
						notifier.throwException("Field '" + memberName +
						                        "' is not a String");
						return nullptr;
					}
					newObj->member->data[memberPos] =
					    notifier.createString(field.get<std::string>());
					break;

				case AutoLang::DefaultClass::boolClassId:
					if (!field.is_boolean()) {
						notifier.throwException("Field '" + memberName +
						                        "' is not a Bool");
						return nullptr;
					}
					newObj->member->data[memberPos] =
					    notifier.createBool(field.get<bool>());
					break;

				default:
					notifier.throwException("Unsupported field type for: " +
					                        memberName);
					return nullptr;
			}
		} catch (const std::exception &e) {
			notifier.throwException(std::string("JSON Error: ") + e.what());
			return nullptr;
		}
	}

	return newObj;
}

inline AObject *to_array_class(NativeFuncInData) {
	auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
	ClassId classId    = args[1]->i;
	ClassId arrayClassId = args[2]->i;

	if (!handle->j.is_array()) {
		notifier.throwException("JSON is not an array");
		return nullptr;
	}

	auto clazz = notifier.vm->data.classes[classId];
	auto newArr = notifier.createArray(arrayClassId);

	for (size_t idx = 0; idx < handle->j.size(); ++idx) {
		const auto &elem = handle->j[idx];

		if (!elem.is_object()) {
			notifier.throwException("Element " + std::to_string(idx) +
			                        " in JSON array is not an object");
			return nullptr;
		}

		auto newObj = notifier.createMemberObject(classId, clazz->memberId.size());

		for (const auto &[memberName, memberPos] : clazz->memberMap) {
			int memberClassId = clazz->memberId[memberPos];

			if (!elem.contains(memberName)) {
				notifier.throwException("Element " + std::to_string(idx) +
				                        " missing field: " + memberName);
				return nullptr;
			}

			const auto &field = elem.at(memberName);

			try {
				switch (memberClassId) {
					case AutoLang::DefaultClass::intClassId:
						if (!field.is_number_integer()) {
							notifier.throwException("Element " + std::to_string(idx) +
							                        " field '" + memberName + "' is not an Int");
							return nullptr;
						}
						newObj->member->data[memberPos] =
						    notifier.createInt(field.get<int64_t>());
						break;

					case AutoLang::DefaultClass::floatClassId:
						if (!field.is_number()) {
							notifier.throwException("Element " + std::to_string(idx) +
							                        " field '" + memberName + "' is not a Float");
							return nullptr;
						}
						newObj->member->data[memberPos] =
						    notifier.createFloat(field.get<double>());
						break;

					case AutoLang::DefaultClass::stringClassId:
						if (!field.is_string()) {
							notifier.throwException("Element " + std::to_string(idx) +
							                        " field '" + memberName + "' is not a String");
							return nullptr;
						}
						newObj->member->data[memberPos] =
						    notifier.createString(field.get<std::string>());
						break;

					case AutoLang::DefaultClass::boolClassId:
						if (!field.is_boolean()) {
							notifier.throwException("Element " + std::to_string(idx) +
							                        " field '" + memberName + "' is not a Bool");
							return nullptr;
						}
						newObj->member->data[memberPos] =
						    notifier.createBool(field.get<bool>());
						break;

					default:
						notifier.throwException("Element " + std::to_string(idx) +
						                        " unsupported field type for: " + memberName);
						return nullptr;
				}
			} catch (const std::exception &e) {
				notifier.throwException(std::string("JSON Error: ") + e.what());
				return nullptr;
			}
		}

		notifier.arrayAdd(newArr, newObj);
	}

	return newArr;
}

inline AObject *to_string(NativeFuncInData) {
    auto handle = static_cast<AJsonHandle *>(args[0]->data->data);
    return notifier.createString(handle->j.dump(-1));
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/json", R"###(
@no_constructor
@no_extends
class Json {
    @native("json_parse")
    private static func _parse(classId: Int, text: String): Json
    static func parse(text: String): Json = _parse(getClassId(Json), text)

    @native("json_empty_object")
    private static func _emptyObject(classId: Int): Json
    static func emptyObject(): Json = _emptyObject(getClassId(Json))

    @native("json_empty_array")
    private static func _emptyArray(classId: Int): Json
    static func emptyArray(): Json = _emptyArray(getClassId(Json))

    @native("json_from_string")
    private static func _fromString(classId: Int, value: String): Json
    static func fromString(value: String): Json = _fromString(getClassId(Json), value)

    @native("json_from_int")
    private static func _fromInt(classId: Int, value: Int): Json
    static func fromInt(value: Int): Json = _fromInt(getClassId(Json), value)

    @native("json_from_float")
    private static func _fromFloat(classId: Int, value: Float): Json
    static func fromFloat(value: Float): Json = _fromFloat(getClassId(Json), value)

    @native("json_from_bool")
    private static func _fromBool(classId: Int, value: Bool): Json
    static func fromBool(value: Bool): Json = _fromBool(getClassId(Json), value)

    @native("json_stringify")
    func stringify(indent: Int = -1): String

    @native("json_is_object") func isObject(): Bool
    @native("json_is_array") func isArray(): Bool
    @native("json_is_string") func isString(): Bool
    @native("json_is_number") func isNumber(): Bool
    @native("json_is_bool") func isBool(): Bool
    @native("json_is_null") func isNull(): Bool

    @native("json_get_size") func getSize(): Int

    @native("json_has_key") func has(key: String): Bool
    
    @native("json_get_field")
    func get(key: String): Json

    @native("json_set_field") 
    func set(key: String, value: Json)

    @native("json_get_index") 
    func getAt(index: Int): Json

    @native("json_add_element") 
    func add(value: Json)

    @native("json_as_string") func asString(): String
    @native("json_as_int") func asInt(): Int
    @native("json_as_float") func asFloat(): Float
    @native("json_as_bool") func asBool(): Bool

	@native("json_to_int_array")
    private func _toIntArray(id: Int): Array<Int>
    func toIntArray(): Array<Int> = _toIntArray(getClassId(Array<Int>))

    @native("json_to_float_array")
    private func _toFloatArray(id: Int): Array<Float>
    func toFloatArray(): Array<Float> = _toFloatArray(getClassId(Array<Float>))

	@native("json_to_bool_array")
	private func _toBoolArray(id: Int): Array<Bool>
	func toBoolArray(): Array<Bool> = _toBoolArray(getClassId(Array<Bool>))

    @native("json_to_string_array")
    private func _toStringArray(id: Int): Array<String>
    func toStringArray(): Array<String> = _toStringArray(getClassId(Array<String>))

	@native("json_to_json_array")
	private func _toJsonArray(id: Int): Array<Json>
	func toJsonArray(): Array<Json> = _toJsonArray(getClassId(Array<Json>))

	@native("json_to_string") 
    func toString(): String

}

@native("json_to_class")
func _jsonToClass<T>(json: Json, classId: Int = getClassId(T)): T

func jsonToClass<T>(json: Json): T = _jsonToClass<T>(json)

@native("json_to_array_class")
func _jsonToArrayClass<T>(json: Json, classId: Int = getClassId(T), arrayClassId: Int = getClassId(Array<T>)): Array<T>

func jsonToArrayClass<T>(json: Json): Array<T> = _jsonToArrayClass<T>(json)

        )###",
	    LibraryConfig(),
	    ANativeMap({
	        {"json_parse", &json::parse},
	        {"json_empty_object", &json::empty_object},
	        {"json_empty_array", &json::empty_array},
	        {"json_from_string", &json::from_string},
	        {"json_from_int", &json::from_int},
	        {"json_from_float", &json::from_float},
	        {"json_from_bool", &json::from_bool},
	        {"json_stringify", &json::stringify},
	        {"json_is_object", &json::is_object},
	        {"json_is_array", &json::is_array},
	        {"json_is_string", &json::is_string},
	        {"json_is_number", &json::is_number},
	        {"json_is_bool", &json::is_bool},
	        {"json_is_null", &json::is_null},
	        {"json_get_size", &json::get_size},
	        {"json_has_key", &json::has_key},
	        {"json_get_field", &json::get_field},
	        {"json_set_field", &json::set_field},
	        {"json_get_index", &json::get_index},
	        {"json_add_element", &json::add_element},
	        {"json_as_string", &json::as_string},
	        {"json_as_int", &json::as_int},
	        {"json_as_float", &json::as_float},
	        {"json_as_bool", &json::as_bool},
	        {"json_to_int_array", &json::to_int_array},
	        {"json_to_float_array", &json::to_float_array},
	        {"json_to_bool_array", &json::to_bool_array},
	        {"json_to_string_array", &json::to_string_array},
	        {"json_to_json_array", &json::to_json_array},
	        {"json_to_class", &json::json_to_class},
			{"json_to_array_class", &json::to_array_class},
			{"json_to_string", &json::to_string},
	    }));
}

} // namespace json
} // namespace Libs
} // namespace AutoLang
#endif