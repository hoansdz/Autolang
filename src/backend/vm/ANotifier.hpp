#ifndef ANOTIFIER_HPP
#define ANOTIFIER_HPP

#include "backend/libs/array.hpp"
#include "backend/vm/AVM.hpp"
#include <sstream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#elif __PYBIND11__
#include <pybind11/embed.h>
#endif

namespace Autolang {

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
	[[nodiscard]] inline AObject *createNull() {
		return DefaultClass::nullObject;
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
	[[nodiscard]] inline AObject *createObject(ClassId classId) {
		auto obj = vm->data.manager.getEmptyObject();
		obj->type = classId;
		return obj;
	}
	[[nodiscard]] inline AObject *createBytes(uint32_t size) {
		return vm->data.manager.getBytes(size);
	}
#ifdef __EMSCRIPTEN__
	[[nodiscard]] inline AObject *getJsObject(ClassId classId,
	                                          emscripten::val *jsObject) {
		return vm->data.manager.getJsObject(classId, jsObject);
	}
#elif __PYBIND11__
	[[nodiscard]] inline AObject *getPyObject(ClassId classId,
	                                          pybind11::object *pyObject) {
		return vm->data.manager.getPyObject(classId, pyObject);
	}
#endif
	[[nodiscard]] inline AObject *createMemberObject(uint32_t type,
	                                                 size_t memberCount) {
		auto obj = vm->data.manager.get(type, memberCount);
		return obj;
	}
	template <typename T>
	[[nodiscard]] inline AObject *createException(T message) {
		auto obj = vm->data.manager.get(DefaultClass::exceptionClassId, 1);
		auto str = createString(message);
		str->retain();
		obj->member->data[0] = str;
		return obj;
	}
	[[nodiscard]] inline AObject *createArray(ClassId classId) {
		auto obj = vm->data.manager.createEmptyObject();
		obj->type = classId;
		obj->member = new NormalArray<Autolang::AObject *>(0);
		obj->flags |= AObject::Flags::OBJ_IS_ARRAY;
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
	inline size_t getArraySize(AObject *arr) { return arr->member->size; }
	inline bool hasException() {
		return vm->callFrames.index && vm->callFrames.top()->exception;
	}
	template <typename T> inline void throwException(T message) {
		callFrame->exception = createException(message);
		callFrame->exception->retain();
	}
	template <typename... Args>
	[[nodiscard]] inline AObject *callFunctionObject(AObject *funcObject,
	                                                 Args &&...args) {
		((std::forward<Args>(args)->retain(),
		  vm->stack.push(std::forward<Args>(args))),
		 ...);
		vm->callFunctionObject(funcObject);
		if (hasException()) {
			return nullptr;
		}
		if (funcObject->function->function->returnId ==
		    DefaultClass::voidClassId) {
			return nullptr;
		}
		return vm->stack.pop();
	}
	template <typename... Args>
	[[nodiscard]] inline AObject *callFunction(Function *func, Args &&...args) {
		((std::forward<Args>(args)->retain(),
		  vm->stack.push(std::forward<Args>(args))),
		 ...);
		vm->callFunction(func);
		if (hasException()) {
			return nullptr;
		}
		if (func->returnId == DefaultClass::voidClassId) {
			return nullptr;
		}
		return vm->stack.pop();
	}
	inline std::string getClassName(ClassId classId) {
		return vm->data.classes[classId]->getName(vm->data);
	}
	inline bool instanceof(AObject *obj, ClassId classId) {
		return obj->type == classId ||
		       vm->data.classes[obj->type]->inheritance.get(classId);
	}
	std::string toString(AObject *obj);
	inline void input(AObject *obj) { vm->input(obj); }
	inline void release(AObject *obj) { vm->data.manager.release(obj); }
	ANotifier(AVM *vm) : vm(vm) {}
};

} // namespace Autolang

#endif