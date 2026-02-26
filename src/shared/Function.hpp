#ifndef AFUNCTION_HPP
#define AFUNCTION_HPP

#include "shared/CompiledProgram.hpp"
#include "shared/FunctionFlags.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"

namespace AutoLang {

struct Function {
	std::string name;
	uint32_t argSize;
	ClassId *args;
	ClassId returnId;
	uint32_t functionFlags;
	union {
		ANativeFunction native;
		std::vector<uint8_t> bytecodes;
	};
	uint32_t maxDeclaration;
	FunctionId id;
	Function()
	    : functionFlags(FunctionFlags::FUNC_IS_NATIVE), args(nullptr),
	      returnId(0), maxDeclaration(0), id(0) {}
	// Function(ClassId id, std::string name, AObject
	// *(*native)(NativeFuncInput),
	//          bool isStatic, ClassId *args, bool* nullableArgs, argsSize,
	//          uint32_t returnId, bool returnNullable)
	//     : id(id), name(name), native(native), isStatic(isStatic),
	//       returnNullable(returnNullable), args(args),
	//       nullableArgs(nullableArgs), returnId(returnId),
	//       maxDeclaration(native ? nullableArgs.size() : 0) {}
	uint32_t loadHash() {
		int64_t hash = 1469598103934665603ull; // FNV offset
		bool isStatic = functionFlags & FunctionFlags::FUNC_IS_STATIC;
		if (isStatic) {
			hash ^= 488;
			hash *= 1099511628211ull;
		}
		for (size_t i = !isStatic; i < argSize; ++i) {
			hash ^= args[i];
			hash *= 1099511628211ull;
		}
		return hash;
	}

	~Function() {
		if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
			bytecodes.~vector<uint8_t>();
		}
		if (args)
			delete[] args;
	}

	// Support log
	std::string toString(CompiledProgram &data);
};

} // namespace AutoLang

#endif