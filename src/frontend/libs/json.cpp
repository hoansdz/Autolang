#ifndef LIB_JSON_CPP
#define LIB_JSON_CPP

#include "json.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <string>
#include <third_party/nlohmann/json.hpp>

namespace Autolang {
class ACompiler;

namespace Libs {
namespace json {

inline AObject *parse(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    const std::string &text = args[0]->str->data;

    try {
        auto parsed = new nlohmann::json(nlohmann::json::parse(text));
        notifier.addManagedMemory(128);
        auto newObj = notifier.createObject(classId);
        newObj->json = parsed;
        return newObj;
    } catch (const nlohmann::json::exception &e) {
        notifier.throwException(std::string("JSON Parse Error: ") + e.what());
        return nullptr;
    }
}

inline AObject *empty_object(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    auto parsed = new nlohmann::json(nlohmann::json::object());
    notifier.addManagedMemory(128);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *empty_array(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    auto parsed = new nlohmann::json(nlohmann::json::array());
    notifier.addManagedMemory(128);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *from_string(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    const std::string &val = args[0]->str->data;
    auto parsed = new nlohmann::json(val);
    notifier.addManagedMemory(128);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *from_int(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    int64_t val = args[0]->i;
    auto parsed = new nlohmann::json(val);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *from_float(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    double val = args[0]->f;
    auto parsed = new nlohmann::json(val);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *from_bool(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    bool val = args[0]->b;
    auto parsed = new nlohmann::json(val);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *stringify(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    int64_t indent = static_cast<int>(args[1]->i);

    std::string result = j_ptr->dump(indent);
    return notifier.createString(result);
}

inline AObject *is_object(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_object());
}

inline AObject *is_array(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_array());
}

inline AObject *is_string(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_string());
}

inline AObject *is_number(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_number());
}

inline AObject *is_bool(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_boolean());
}

inline AObject *is_null(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_null());
}

inline AObject *get_size(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createInt(static_cast<int64_t>(j_ptr->size()));
}

inline AObject *has_key(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    const std::string &key = args[1]->str->data;

    if (!j_ptr->is_object()) {
        return notifier.createBool(false);
    }
    return notifier.createBool(j_ptr->contains(key));
}

inline AObject *get_field(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    const std::string &key = args[1]->str->data;

    if (!j_ptr->is_object()) {
        notifier.throwException("JSON is not an object");
        return nullptr;
    }
    if (!j_ptr->contains(key)) {
        notifier.throwException("JSON Object does not contain key: " + key);
        return nullptr;
    }

    auto j_copy = new nlohmann::json(j_ptr->at(key));
    auto newObj = notifier.createObject(args[0]->type);
    newObj->json = j_copy;
    return newObj;
}

inline AObject *set_field(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    const std::string &key = args[1]->str->data;
    auto val_ptr = args[2]->json;

    if (!j_ptr->is_object()) {
        notifier.throwException("JSON is not an object");
        return nullptr;
    }

    (*j_ptr)[key] = *val_ptr;
    return nullptr;
}

inline AObject *get_index(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    size_t index = static_cast<size_t>(args[1]->i);

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }
    if (index >= j_ptr->size()) {
        notifier.throwException("JSON Array index out of bounds");
        return nullptr;
    }

    auto j_copy = new nlohmann::json(j_ptr->at(index));
    auto newObj = notifier.createObject(args[0]->type);
    newObj->json = j_copy;
    return newObj;
}

inline AObject *add_element(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    auto val_ptr = args[1]->json;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    j_ptr->push_back(*val_ptr);
    return nullptr;
}

inline AObject *as_string(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    if (!j_ptr->is_string()) {
        notifier.throwException("JSON value is not a String");
        return nullptr;
    }
    return notifier.createString(j_ptr->get<std::string>());
}

inline AObject *as_int(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    if (!j_ptr->is_number_integer()) {
        notifier.throwException("JSON value is not an Integer");
        return nullptr;
    }
    return notifier.createInt(j_ptr->get<int64_t>());
}

inline AObject *as_float(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    if (!j_ptr->is_number()) {
        notifier.throwException("JSON value is not a Number");
        return nullptr;
    }
    return notifier.createFloat(j_ptr->get<double>());
}

inline AObject *as_bool(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    if (!j_ptr->is_boolean()) {
        notifier.throwException("JSON value is not a Boolean");
        return nullptr;
    }
    return notifier.createBool(j_ptr->get<bool>());
}

inline AObject *to_int_array(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    try {
        for (auto &element : *j_ptr) {
            if (!element.is_number_integer()) {
                notifier.throwException("Element " + std::to_string(notifier.getArraySize(newArr)) + " in JSON array is not an integer");
                return nullptr;
            }
            notifier.arrayAdd(newArr, notifier.createInt(element.get<int64_t>()));
        }
    } catch (const std::exception &e) {
        notifier.throwException(std::string("JSON Error: ") + e.what());
        return nullptr;
    }
    return newArr;
}

inline AObject *to_float_array(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    try {
        for (auto &element : *j_ptr) {
            if (!element.is_number()) {
                notifier.throwException("Element " + std::to_string(newArr->member->size) + " in JSON array is not a number");
                return nullptr;
            }
            notifier.arrayAdd(newArr, notifier.createFloat(element.get<double>()));
        }
    } catch (const std::exception &e) {
        notifier.throwException(std::string("JSON Error: ") + e.what());
        return nullptr;
    }
    return newArr;
}

inline AObject *to_bool_array(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    try {
        for (auto &element : *j_ptr) {
            if (!element.is_boolean()) {
                notifier.throwException("Element " + std::to_string(notifier.getArraySize(newArr)) + " in JSON array is not a boolean");
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
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    try {
        for (auto &element : *j_ptr) {
            if (!element.is_string()) {
                notifier.throwException("Element " + std::to_string(notifier.getArraySize(newArr)) + " in JSON array is not a string");
                return nullptr;
            }
            notifier.arrayAdd(newArr, notifier.createString(element.get<std::string>()));
        }
    } catch (const std::exception &e) {
        notifier.throwException(std::string("JSON Error: ") + e.what());
        return nullptr;
    }
    return newArr;
}

inline AObject *to_json_array(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    try {
        for (auto &element : *j_ptr) {
            auto elem_ptr = new nlohmann::json(element);
            auto newElemObj = notifier.createObject(args[0]->type);
            newElemObj->json = elem_ptr;
            notifier.arrayAdd(newArr, newElemObj);
        }
    } catch (const std::exception &e) {
        notifier.throwException(std::string("JSON Error: ") + e.what());
        return nullptr;
    }
    return newArr;
}

AObject* jsonValueToAObject(ANotifier& notifier, const nlohmann::json& field, ClassId targetClassId, bool isNullable);

AObject* jsonObjectToAObject(ANotifier& notifier, const nlohmann::json& j_obj, ClassId classId);

AObject* jsonArrayToAObject(ANotifier& notifier, const nlohmann::json& j_arr, ClassId arrayClassId, ClassId elemClassId, bool elemNullable);

AObject* jsonValueToAObject(ANotifier& notifier, const nlohmann::json& field, ClassId targetClassId, bool isNullable) {
    if (field.is_null()) {
        if (isNullable) return notifier.createNull();
        notifier.throwException("JSON field is null but target is not nullable");
        return nullptr;
    }

    switch (targetClassId) {
        case Autolang::DefaultClass::intClassId:
            if (!field.is_number_integer()) {
                notifier.throwException("JSON value is not an Int");
                return nullptr;
            }
            return notifier.createInt(field.get<int64_t>());
            
        case Autolang::DefaultClass::floatClassId:
            if (!field.is_number()) {
                notifier.throwException("JSON value is not a Float");
                return nullptr;
            }
            return notifier.createFloat(field.get<double>());
            
        case Autolang::DefaultClass::stringClassId:
            if (!field.is_string()) {
                notifier.throwException("JSON value is not a String");
                return nullptr;
            }
            return notifier.createString(field.get<std::string>());
            
        case Autolang::DefaultClass::boolClassId:
            if (!field.is_boolean()) {
                notifier.throwException("JSON value is not a Bool");
                return nullptr;
            }
            return notifier.createBool(field.get<bool>());
            
        case Autolang::DefaultClass::jsonClassId: {
            auto j_copy = new nlohmann::json(field);
            auto newObj = notifier.createObject(Autolang::DefaultClass::jsonClassId);
            newObj->json = j_copy;
            return newObj;
        }

        default: {
            auto clazz = notifier.vm->data.classes[targetClassId];
            
            switch (clazz->genericBaseClassId) {
                case Autolang::DefaultClass::arrayClassId: {
                    ClassId genericElemClassId = notifier.vm->data.allGenericType[clazz->genericType.offset];
                    bool genericElemNullable = notifier.vm->data.allGenericTypeNullable[clazz->genericType.offset];
                    return jsonArrayToAObject(notifier, field, targetClassId, genericElemClassId, genericElemNullable);
                }
                
                default: {
                    return jsonObjectToAObject(notifier, field, targetClassId);
                }
            }
        }
    }
}

AObject* jsonObjectToAObject(ANotifier& notifier, const nlohmann::json& j_obj, ClassId classId) {
    if (!j_obj.is_object()) {
        notifier.throwException("JSON is not an object");
        return nullptr;
    }

    auto clazz = notifier.vm->data.classes[classId];
    auto newObj = notifier.createMemberObject(classId, clazz->memberMap.size());
    ClassId* memberId = notifier.vm->data.getMemberRef(clazz->memberIdOffset);
    size_t nullableOffset = clazz->memberIdOffset;

    for (const auto &[memberName, memberPos] : clazz->memberMap) {
        ClassId memberClassId = memberId[memberPos];
        bool isNullable = notifier.vm->data.allMemberNullable[nullableOffset + memberPos];
        
        bool hasField = j_obj.contains(memberName);
        
        if (!hasField || j_obj.at(memberName).is_null()) {
            if (isNullable) {
                newObj->member->data[memberPos] = notifier.createNull();
            } else {
                notifier.throwException("JSON missing required non-nullable field: " + memberName);
                return nullptr;
            }
        } else {
            AObject* val = jsonValueToAObject(notifier, j_obj.at(memberName), memberClassId, isNullable);
            if (!val) return nullptr;
            newObj->member->data[memberPos] = val;
        }
    }
    return newObj;
}

AObject* jsonArrayToAObject(ANotifier& notifier, const nlohmann::json& j_arr, ClassId arrayClassId, ClassId elemClassId, bool elemNullable) {
    if (!j_arr.is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    for (size_t idx = 0; idx < j_arr.size(); ++idx) {
        AObject* val = jsonValueToAObject(notifier, j_arr[idx], elemClassId, elemNullable);
        
        if (!val) return nullptr;

        notifier.arrayAdd(newArr, val);
    }
    return newArr;
}

inline AObject *json_to_class(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId classId = notifier.callFrame->func->returnId;
    return jsonObjectToAObject(notifier, *j_ptr, classId);
}

inline AObject *to_array_class(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;
    auto arrayClass = notifier.vm->data.classes[arrayClassId];
    
    ClassId elemClassId = notifier.vm->data.allGenericType[arrayClass->genericType.offset];
    bool elemNullable = notifier.vm->data.allGenericTypeNullable[arrayClass->genericType.offset];
    
    return jsonArrayToAObject(notifier, *j_ptr, arrayClassId, elemClassId, elemNullable);
}

inline AObject *to_string(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createString(j_ptr->dump(-1));
}

inline AObject *remove_field(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    const std::string &key = args[1]->str->data;

    if (!j_ptr->is_object()) {
        notifier.throwException("JSON is not an object");
        return nullptr;
    }
    
    bool isRemoved = j_ptr->erase(key) > 0;
    return notifier.createBool(isRemoved);
}

inline AObject *remove_at(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    size_t index = static_cast<size_t>(args[1]->i);

    if (!j_ptr->is_array()) {
        notifier.throwException("JSON is not an array");
        return nullptr;
    }
    
    if (index >= j_ptr->size()) {
        notifier.throwException("JSON Array index out of bounds");
        return nullptr;
    }

    j_ptr->erase(index);
    return notifier.createBool(true);
}

inline AObject *clear(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    j_ptr->clear();
    return nullptr;
}

inline AObject *keys(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    ClassId arrayClassId = notifier.callFrame->func->returnId;

    if (!j_ptr->is_object()) {
        notifier.throwException("JSON is not an object");
        return nullptr;
    }

    auto newArr = notifier.createArray(arrayClassId);
    for (auto& el : j_ptr->items()) {
        notifier.arrayAdd(newArr, notifier.createString(el.key()));
    }
    return newArr;
}

inline AObject *null_value(NativeFuncInData) {
    constexpr ClassId classId = DefaultClass::jsonClassId;
    auto parsed = new nlohmann::json(nullptr);
    auto newObj = notifier.createObject(classId);
    newObj->json = parsed;
    return newObj;
}

inline AObject *is_int(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_number_integer());
}

inline AObject *is_float(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    return notifier.createBool(j_ptr->is_number_float());
}

inline AObject *clone(NativeFuncInData) {
    auto j_ptr = args[0]->json;
    auto j_copy = new nlohmann::json(*j_ptr); 
    auto newObj = notifier.createObject(args[0]->type);
    newObj->json = j_copy;
    return newObj;
}

void init(ACompiler &compiler) {
    compiler.registerBuiltInLibrary(
        "std/json", R"###(
    @native("json_parse")
    static fun Json.parse(text: String): Json

    @native("json_empty_object")
    static fun Json.emptyObject(): Json

    @native("json_empty_array")
    static fun Json.emptyArray(): Json

    @native("json_from_string")
    static fun Json.fromString(value: String): Json

    @native("json_from_int")
    static fun Json.fromInt(value: Int): Json

    @native("json_from_float")
    static fun Json.fromFloat(value: Float): Json

    @native("json_from_bool")
    static fun Json.fromBool(value: Bool): Json

    @native("json_stringify")
    fun Json.stringify(indent: Int = -1): String

    @native("json_is_object") fun Json.isObject(): Bool
    @native("json_is_array") fun Json.isArray(): Bool
    @native("json_is_string") fun Json.isString(): Bool
    @native("json_is_number") fun Json.isNumber(): Bool
    @native("json_is_bool") fun Json.isBool(): Bool
    @native("json_is_null") fun Json.isNull(): Bool

    @native("json_get_size") fun Json.getSize(): Int

    @native("json_has_key") fun Json.has(key: String): Bool
    
    @native("json_get_field")
    fun Json.get(key: String): Json

    @native("json_set_field") 
    fun Json.set(key: String, value: Json)

    @native("json_get_index") 
    fun Json.getAt(index: Int): Json

    @native("json_add_element") 
    fun Json.add(value: Json)

    @native("json_as_string") fun Json.asString(): String
    @native("json_as_int") fun Json.asInt(): Int
    @native("json_as_float") fun Json.asFloat(): Float
    @native("json_as_bool") fun Json.asBool(): Bool

    @native("json_to_int_array")
    fun Json.toIntArray(): Array<Int>

    @native("json_to_float_array")
    fun Json.toFloatArray(): Array<Float>

    @native("json_to_bool_array")
    fun Json.toBoolArray(): Array<Bool>

    @native("json_to_string_array")
    fun Json.toStringArray(): Array<String>

    @native("json_to_json_array")
    fun Json.toJsonArray(): Array<Json>

    @native("json_to_string") 
    fun Json.toString(): String

    @native("json_to_class")
    fun jsonToClass<T>(json: Json): T

    @native("json_to_array_class")
    fun jsonToArrayClass<T>(json: Json): Array<T>

    @native("json_null_value")
    static fun Json.nullValue(): Json

    @native("json_remove_field") 
    fun Json.remove(key: String): Bool
    
    @native("json_remove_at") 
    fun Json.removeAt(index: Int): Bool

    @native("json_clear") 
    fun Json.clear()

    @native("json_keys")
    fun Json.keys(): Array<String>

    @native("json_is_int") fun Json.isInt(): Bool
    @native("json_is_float") fun Json.isFloat(): Bool

    @native("json_clone")
    fun Json.clone(): Json
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
            {"json_null_value", &json::null_value},
            {"json_remove_field", &json::remove_field},
            {"json_remove_at", &json::remove_at},
            {"json_clear", &json::clear},
            {"json_keys", &json::keys},
            {"json_is_int", &json::is_int},
            {"json_is_float", &json::is_float},
            {"json_clone", &json::clone},
        }));
}

} // namespace json
} // namespace Libs
} // namespace Autolang
#endif