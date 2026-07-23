#ifndef CLASS_FLAGS_HPP
#define CLASS_FLAGS_HPP

#include <iostream>

namespace Autolang {

enum ClassFlags : uint32_t {
	CLASS_NO_CONSTRUCTOR = 1u << 0,
	CLASS_NATIVE_DATA = 1u << 1,
	CLASS_HAS_PARENT = 1u << 2,
	CLASS_NO_EXTENDS = 1u << 3,
	CLASS_IS_ENUM = 1u << 4,
#ifdef __EMSCRIPTEN__
	CLASS_JS_OBJECT = 1u << 5,
#elif __PYBIND11__
	CLASS_PY_OBJECT = 1u << 5,
#endif
};

}

#endif