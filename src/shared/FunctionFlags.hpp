#ifndef FUNCTION_FLAGS_HPP
#define FUNCTION_FLAGS_HPP

#include <iostream>

namespace AutoLang {

enum FunctionFlags : uint32_t {
	FUNC_IS_CONSTRUCTOR = 1u << 0,
	FUNC_IS_VIRTUAL = 1u << 1,
	FUNC_RETURN_NULLABLE = 1u << 2,
	FUNC_PUBLIC = 1u << 3,
	FUNC_PRIVATE = 1u << 4,
	FUNC_PROTECTED = 1u << 5,
	FUNC_IS_NATIVE = 1u << 6,
	FUNC_IS_STATIC = 1u << 7,
	FUNC_OVERRIDE = 1u << 8,
	FUNC_NO_OVERRIDE = 1u << 9,
	FUNC_IS_DATA_CONSTRUCTOR = 1u << 10,
};

}

#endif