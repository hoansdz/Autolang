#ifndef FUNCTION_OBJECT_HPP
#define FUNCTION_OBJECT_HPP

// #include "shared/Function.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>

namespace Autolang {

struct AObject;
struct Function;

struct FunctionObject {
	uint32_t size;
	AObject **args;
	Function *function;

	explicit FunctionObject(uint32_t size, AObject **args, Function *function)
	    : size(size), args(args), function(function) {}

	~FunctionObject() {
		delete []args;
	}
};
} // namespace Autolang

#endif