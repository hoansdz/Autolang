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
	LAMBDA = 0,
	FUNC = 0,
#ifdef __EMSCRIPTEN__
	JS_FUNCTION,
#elif __PYBIND11__
	PY_FUNCTION,
#endif
};

struct ANativeFunctionData {
	ANativeFunctionType type;
	ANativeLambdaFunction nativeLambda;
#ifdef __EMSCRIPTEN__
	val *jsFunction = nullptr;
#elif __PYBIND11__
	object *pyFunction = nullptr;
#endif
	ANativeFunctionData() : type(ANativeFunctionType::LAMBDA), nativeLambda(nullptr) {}
	ANativeFunctionData(ANativeFunction native)
	    : type(ANativeFunctionType::LAMBDA), nativeLambda(native) {}
	ANativeFunctionData(ANativeLambdaFunction nativeLambda)
	    : type(ANativeFunctionType::LAMBDA),
	      nativeLambda(std::move(nativeLambda)) {}
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