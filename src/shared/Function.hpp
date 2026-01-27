#ifndef AFUNCTION_HPP
#define AFUNCTION_HPP

#include "shared/CompiledProgram.hpp"
#include "backend/optimize/FixedArray.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"

#define NativeFuncInput ObjectManager &, StackAllocator &, size_t
#define NativeFuncInData ObjectManager &manager, StackAllocator &stackAllocator, size_t size

struct Function
{
	std::string name;
	AObject *(*native)(NativeFuncInput);
	bool isStatic;
	bool returnNullable;
	FixedArray<ClassId> args;
	FixedArray<bool> nullableArgs;
	ClassId returnId;
	std::vector<uint8_t> bytecodes;
	uint32_t maxDeclaration;
	Offset id;
	//Support log
	std::string toString(CompiledProgram& data);
	Function(ClassId id, std::string name, AObject *(*native)(NativeFuncInput), bool isStatic, std::vector<ClassId> &args, std::vector<bool> &nullableArgs, uint32_t returnId, bool returnNullable) : 
		id(id), name(name), native(native), isStatic(isStatic), returnNullable(returnNullable), args(args), nullableArgs(nullableArgs), returnId(returnId), maxDeclaration(native ? this->args.size : 0) {}
};

#endif