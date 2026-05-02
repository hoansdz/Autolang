#ifndef FUNCTION_EVENT_HPP
#define FUNCTION_EVENT_HPP

#ifdef __EMSCRIPTEN__
#include "shared/JSFunction.hpp"
#include <emscripten/bind.h>
using namespace emscripten;
#endif

namespace AutoLang {

enum FunctionEventType : uint8_t {
	FE_LAMBDA,
#ifdef __EMSCRIPTEN__
	FE_JS_FUNCTION,
#endif
};

struct FunctionEvent {
	FunctionEventType type;
	union {
		std::function<void(std::string_view message)> nativeLambda;
#ifdef __EMSCRIPTEN__
		val jsFunction;
#endif
	};
	FunctionEvent(std::function<void(std::string_view)> func) {
		type = FE_LAMBDA;
		new (&nativeLambda)
		    std::function<void(std::string_view)>(std::move(func));
	}
#ifdef __EMSCRIPTEN__
	FunctionEvent(val jsFunc) {
		type = FE_JS_FUNCTION;
		new (&jsFunction) val(std::move(jsFunc));
	}
#endif
	FunctionEvent(const FunctionEvent &) = delete;
	FunctionEvent &operator=(const FunctionEvent &) = delete;
	inline void operator()(std::string_view message) {
		switch (type) {
			case FE_LAMBDA: {
				nativeLambda(message);
				break;
			}
#ifdef __EMSCRIPTEN__
			default: {
				jsFunction(val(std::string(message)));
				break;
			}
#endif
		}
	}

	~FunctionEvent() {
		switch (type) {
			case FE_LAMBDA:
				nativeLambda.~function();
				break;
#ifdef __EMSCRIPTEN__
			case FE_JS_FUNCTION:
				jsFunction.~val();
				break;
#endif
			default:
				break;
		}
	}
};

} // namespace AutoLang

#endif