#ifndef ANOTIFIER_HPP
#define ANOTIFIER_HPP

#include "backend/libs/array.hpp"
#include "backend/vm/AVM.hpp"
#include <sstream>

namespace AutoLang {

class ANotifier {
  public:
	AVM *vm;
	CallFrame *callFrame;

	[[nodiscard]] inline AObject *createInt(int64_t value) {
		return vm->data.manager.createIntObject(value);
	}
	[[nodiscard]] inline AObject *createFloat(double value) {
		return vm->data.manager.createFloatObject(value);
	}
	[[nodiscard]] inline AObject *createBool(bool value) {
		return vm->data.manager.createBoolObject(value);
	}
	template <typename T> [[nodiscard]] inline AObject *createString(T value) {
		return vm->data.manager.createString(value);
	}
	[[nodiscard]] inline AObject *createString(AString *value) {
		return vm->data.manager.createStringObject(value);
	}
	[[nodiscard]] inline AObject *getTrueObject() {
		return DefaultClass::trueObject;
	}
	[[nodiscard]] inline AObject *getFalseObject() {
		return DefaultClass::falseObject;
	}
	[[nodiscard]] inline AObject *getNullObject() {
		return DefaultClass::nullObject;
	}
	[[nodiscard]] inline AObject *
	createNativeData(ClassId classId, void *data,
	                 DestructorParameters destructor = nullptr) {
		auto obj =
		    vm->data.manager.get(classId, new ANativeData{data, destructor});
		return obj;
	}
	[[nodiscard]] inline AObject *createBytes(uint32_t size) {
		return vm->data.manager.getBytes(size);
	}
	template <typename T>
	[[nodiscard]] inline AObject *createException(T message) {
		auto obj = vm->data.manager.createEmptyObject();
		obj->type = DefaultClass::exceptionClassId;
		obj->member = new NormalArray<AutoLang::AObject *>(1);
		auto str = createString(message);
		str->retain();
		obj->member->data[0] = str;
		return obj;
	}
	[[nodiscard]] inline AObject *createArray(ClassId classId) {
		auto obj = vm->data.manager.createEmptyObject();
		obj->type = classId;
		obj->member = new NormalArray<AutoLang::AObject *>(0);
		return obj;
	}
	inline void arrayAdd(AObject *arr, AObject *obj) {
		obj->retain();

		if (arr->member->size == 0) {
			arr->member->reallocate(1);
			arr->member->data[0] = obj;
			arr->member->size = 1;
			arr->member->maxSize = 1;
			return;
		}

		if (arr->member->size == arr->member->maxSize) {
			size_t newMax =
			    (arr->member->maxSize == 0) ? 1 : arr->member->maxSize * 2;
			arr->member->reallocate(newMax);
			arr->member->maxSize = static_cast<int64_t>(newMax);
		}

		arr->member->data[arr->member->size++] = obj;
	}
	inline bool hasException() { return vm->callFrames.top()->exception; }
	template <typename T> inline void throwException(T message) {
		callFrame->exception = createException(message);
	}
	template <typename... Args>
	[[nodiscard]] inline AObject *callFunctionObject(AObject *funcObject,
	                                                 Args &&...args) {
		((std::forward<Args>(args)->retain(),
		  vm->stack.push(std::forward<Args>(args))),
		 ...);
		vm->callFunctionObject(funcObject);
		if (funcObject->function->function->returnId ==
		    DefaultClass::voidClassId) {
			return nullptr;
		}
		return vm->stack.pop();
	}
	inline std::string toString(AObject *obj) {
		if (!obj) {
			return "c_nullptr";
		}
		uint32_t type = obj->type;
		switch (type) {
			case AutoLang::DefaultClass::intClassId:
				return std::to_string(obj->i);
			case AutoLang::DefaultClass::floatClassId:
				return std::to_string(obj->f);
			case AutoLang::DefaultClass::stringClassId:
				return std::string(obj->str->data);
			case AutoLang::DefaultClass::nullClassId:
				return "null";
			case DefaultClass::boolClassId:
				return (obj == DefaultClass::trueObject ? "true" : "false");
			default:
				auto clazz = vm->data.classes[obj->type];
				// auto it = clazz->funcMap.find("toString");
				// if (it == clazz->funcMap.end()) {
				std::stringstream ss;
				ss << clazz->name << "@" << obj;
				return ss.str();
				// }
				// return vm->callFunction<false, true, false>(currentCallFrame,
				// currentFunction, bytecodes, i);
		}
	}
	inline void input(AObject *obj) { vm->input(obj); }
	inline void release(AObject *obj) { vm->data.manager.release(obj); }
	ANotifier(AVM *vm) : vm(vm) {}
};

} // namespace AutoLang

#endif