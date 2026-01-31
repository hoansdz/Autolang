#ifndef AFUNCTION_HPP
#define AFUNCTION_HPP

#include "backend/optimize/FixedArray.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"

#define NativeFuncInput ObjectManager &, AObject **, size_t
#define NativeFuncInData ObjectManager &manager, AObject **args, size_t size

struct Function {
	std::string name;
	AObject *(*native)(NativeFuncInput);
	bool isStatic;
	bool returnNullable;
	ClassId *args;
	FixedArray<bool> nullableArgs;
	ClassId returnId;
	std::vector<uint8_t> bytecodes;
	uint32_t maxDeclaration;
	Offset id;
	Function()
	    : native(nullptr), isStatic(false), returnNullable(false),
	      args(nullptr), returnId(0), maxDeclaration(0), id(0) {}
	Function(ClassId id, std::string name, AObject *(*native)(NativeFuncInput),
	         bool isStatic, ClassId *args, std::vector<bool> &nullableArgs,
	         uint32_t returnId, bool returnNullable)
	    : id(id), name(name), native(native), isStatic(isStatic),
	      returnNullable(returnNullable), args(args),
	      nullableArgs(nullableArgs), returnId(returnId),
	      maxDeclaration(native ? nullableArgs.size() : 0) {}
	uint32_t loadHash() {
		int64_t hash = 1469598103934665603ull; // FNV offset
		if (!isStatic) {
			hash ^= 488;
			hash *= 1099511628211ull;
		}
		for (size_t i = !isStatic; i < nullableArgs.size; ++i) {
			hash ^= args[i];
			hash *= 1099511628211ull;
		}
		return hash;
	}

	~Function() {
		if (!args) {
			return;
		}
		delete[] args;
	}

	// Support log
	std::string toString(CompiledProgram &data);
};

#endif