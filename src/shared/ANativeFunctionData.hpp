#ifndef ANATIVE_FUNCTION_DATA_HPP
#define ANATIVE_FUNCTION_DATA_HPP

#include "shared/Type.hpp"
#ifdef __EMSCRIPTEN__
#include "shared/JSFunction.hpp"
#include <emscripten/bind.h>
using namespace emscripten;
#endif

namespace AutoLang {

enum ANativeFunctionType : uint8_t {
	FUNC,
	LAMBDA,
#ifdef __EMSCRIPTEN__
	JS_FUNCTION,
#endif
};

struct ANativeFunctionData {
	ANativeFunctionType type;
	union {
		ANativeFunction native;
		ANativeLambdaFunction *nativeLambda;
#ifdef __EMSCRIPTEN__
		val *jsFunction;
#endif
	};
	ANativeFunctionData()
	    : type(ANativeFunctionType::FUNC), native(nullptr) {}
	ANativeFunctionData(ANativeFunction native)
	    : type(ANativeFunctionType::FUNC), native(native) {}
	ANativeFunctionData(ANativeLambdaFunction nativeLambda)
	    : type(ANativeFunctionType::LAMBDA),
	      nativeLambda(new ANativeLambdaFunction(nativeLambda)) {}
#ifdef __EMSCRIPTEN__
	ANativeFunctionData(val *jsFunction)
	    : type(ANativeFunctionType::JS_FUNCTION), jsFunction(jsFunction) {}
#endif
	inline AObject *operator()(NativeFuncInData) {
		switch (type) {
			case FUNC: {
				return native(notifier, args, argSize);
			}
			case LAMBDA: {
				return (*nativeLambda)(notifier, args, argSize);
			}
#ifdef __EMSCRIPTEN__
			default: {
				return callJSFunction(jsFunction, notifier, args, argSize);
			}
#endif
		}
	}
};

} // namespace AutoLang

#endif