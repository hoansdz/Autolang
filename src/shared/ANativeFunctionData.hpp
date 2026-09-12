#ifndef ANATIVE_FUNCTION_DATA_HPP
#define ANATIVE_FUNCTION_DATA_HPP

// #include "backend/vm/ANotifier.hpp"
#include "shared/Type.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
using namespace emscripten;
#elif __PYBIND11__
#include <pybind11/embed.h>
using namespace pybind11;

#endif

namespace Autolang {

enum ANativeFunctionType : uint8_t {
	FUNC,
	LAMBDA,
#ifdef __EMSCRIPTEN__
	JS_FUNCTION,
#elif __PYBIND11__
	PY_FUNCTION,
#endif
};

struct ANativeFunctionData {
	ANativeFunctionType type;
	union {
		ANativeFunction native;
		ANativeLambdaFunction *nativeLambda;
#ifdef __EMSCRIPTEN__
		val *jsFunction;
#elif __PYBIND11__
		object *pyFunction;
#endif
	};
	ANativeFunctionData() : type(ANativeFunctionType::FUNC), native(nullptr) {}
	ANativeFunctionData(ANativeFunction native)
	    : type(ANativeFunctionType::FUNC), native(native) {}
	ANativeFunctionData(ANativeLambdaFunction nativeLambda)
	    : type(ANativeFunctionType::LAMBDA),
	      nativeLambda(new ANativeLambdaFunction(nativeLambda)) {}
#ifdef __EMSCRIPTEN__
	ANativeFunctionData(val *jsFunction)
	    : type(ANativeFunctionType::JS_FUNCTION), jsFunction(jsFunction) {}
#elif __PYBIND11__
	ANativeFunctionData(object *pyFunction)
	    : type(ANativeFunctionType::PY_FUNCTION), pyFunction(pyFunction) {}
#endif
	AObject *operator()(NativeFuncInData);
};

} // namespace Autolang

#endif