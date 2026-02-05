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
	[[nodiscard]] inline AObject* createString(AString* value) {
		return vm->data.manager.createStringObject(value);
	}
	template <typename T>
	[[nodiscard]] inline AObject* createString(T value) {
		return vm->data.manager.createString(value);
	}
	template <typename T>
	[[nodiscard]] inline AObject* createException(T message) {
		auto obj = vm->data.manager.createEmptyObject();
		obj->type = DefaultClass::exceptionClassId;
		obj->member = new NormalArray[1];
		auto str = createString(message);
		str->retain();
		obj->member[0] = str;
		return obj;
	}
	ANotifier(AVM *vm) : vm(vm) {}
};

} // namespace AutoLang

#endif