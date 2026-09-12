#include "shared/ANativeFunctionData.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"

#ifdef __EMSCRIPTEN__
#include "shared/JSFunction.hpp"
#elif __PYBIND11__
#include "shared/PYFunction.hpp"
#endif

namespace Autolang {

AObject *ANativeFunctionData::operator()(NativeFuncInData) {
	switch (type) {
		case FUNC: {
			return native(notifier, args, argSize);
		}
		case LAMBDA: {
			return (*nativeLambda)(notifier, args, argSize);
		}
#ifdef __EMSCRIPTEN__
		case JS_FUNCTION: {
			return callJSFunction(jsFunction, notifier, args, argSize);
		}
#endif
#ifdef __PYBIND11__
		case PY_FUNCTION: {
			return callPyFunction(pyFunction, notifier, args, argSize);
		}
#endif
		default: {
			return nullptr;
		}
	}
}

} // namespace Autolang
