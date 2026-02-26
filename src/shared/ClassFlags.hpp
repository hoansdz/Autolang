#ifndef CLASS_FLAGS_HPP
#define CLASS_FLAGS_HPP

#include <iostream>

namespace AutoLang {

enum ClassFlags : uint32_t {
	CLASS_NO_CONSTRUCTOR = 1u << 0,
	CLASS_NATIVE_DATA = 1u << 1,
	CLASS_HAS_PARENT = 1u << 2,
	CLASS_NO_EXTENDS = 1u << 3
};

}

#endif