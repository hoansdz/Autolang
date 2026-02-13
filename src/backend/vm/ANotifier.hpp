#ifndef ANOTIFIER_HPP
#define ANOTIFIER_HPP

#include "backend/vm/AVM.hpp"

namespace AutoLang {

class ANotifier {
  public:
	AVM* vm;
	CallFrame* callFrame;

	[[nodiscard]] inline AObject* createInt(int64_t value) {
		return vm->data.manager.createIntObject(value);
	}
	[[nodiscard]] inline AObject* createFloat(double value) {
		return vm->data.manager.createFloatObject(value);
	}
	[[nodiscard]] inline AObject* createBool(bool value) {
		return vm->data.manager.createBoolObject(value);
	}
	template <typename T>
	[[nodiscard]] inline AObject* createString(T value) {
		return vm->data.manager.createString(value);
	}
	[[nodiscard]] inline AObject* createString(AString* value) {
		return vm->data.manager.createStringObject(value);
	}
	template <typename T>
	[[nodiscard]] inline AObject* createException(T message) {
		auto obj = vm->data.manager.createEmptyObject();
		obj->type = DefaultClass::exceptionClassId;
		obj->member = new NormalArray<AutoLang::AObject*>(1);
		auto str = createString(message);
		str->retain();
		obj->member->data[0] = str;
		return obj;
	}
	template <typename T>
	inline void throwException(T message) {
		callFrame->exception = createException(message);
	}
	inline void input(AObject* obj) {
		vm->input(obj);
	}
	ANotifier(AVM *vm) : vm(vm) {}
};

} // namespace AutoLang

#endif