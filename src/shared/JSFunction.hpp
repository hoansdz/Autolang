#ifndef JS_FUNCTION_HPP
#define JS_FUNCTION_HPP

#include "shared/Type.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/AObject.hpp"
#include <emscripten/bind.h>

using namespace emscripten;

namespace AutoLang {

inline val autolangToJS(AObject *obj) {
	switch (obj->type) {
		case DefaultClass::intClassId: {
			return val(obj->i);
		}
		case DefaultClass::floatClassId: {
			return val(obj->f);
		}
		case DefaultClass::stringClassId: {
			return val(std::string(obj->str->data));
		}
		case DefaultClass::boolClassId: {
			return val(obj->b);
		}
		case DefaultClass::nullClassId: {
			return val::null();
		}
		default: {
			return val(reinterpret_cast<uintptr_t>(obj));
		}
	}
}

inline AObject *jsObjectToAObject(ANotifier &notifier, val value) {
	if (value.isNull()) {
		return DefaultClass::nullObject;
	}
	ClassId classId = notifier.callFrame->func->returnId;
	switch (classId) {
		case DefaultClass::intClassId: {
			return notifier.createInt(value.as<int64_t>());
		}
		case DefaultClass::floatClassId: {
			return notifier.createFloat(value.as<double>());
		}
		case DefaultClass::stringClassId: {
			return notifier.createString(value.as<std::string>());
		}
		case DefaultClass::boolClassId: {
			return notifier.createBool(value.as<bool>());
		}
		case DefaultClass::voidClassId: {
			return nullptr;
		}
		default: {
			return nullptr;
		}
	}
}

inline AObject *callJSFunction(val *jsFunction, NativeFuncInData) {
	val jsArgsArray = val::array();
	for (size_t i = 0; i < argSize; ++i) {
		jsArgsArray.call<void>("push", autolangToJS(args[i]));
	}
	notifier.callFrame->func->returnId;
	return jsObjectToAObject(notifier, (*jsFunction)(jsArgsArray));
}

} // namespace AutoLang

#endif