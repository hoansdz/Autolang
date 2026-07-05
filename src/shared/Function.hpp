#ifndef AFUNCTION_HPP
#define AFUNCTION_HPP

#include "shared/ANativeFunctionData.hpp"
#include "shared/Bytecodes.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/FunctionFlags.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"
#include "shared/Type.hpp"


namespace AutoLang {

struct Function {
	StringArenaOffset nameStringOffset;
	uint32_t argSize;
	ClassId returnId;
	uint32_t functionFlags;
	uint32_t maxDeclaration;
	FunctionId id;
	ClassId *args;
	union {
		ANativeFunctionData *native;
		Bytecodes bytecodes;
	};
	Function()
	    : args(nullptr), returnId(0),
	      functionFlags(FunctionFlags::FUNC_IS_NATIVE), maxDeclaration(0),
	      id(0) {}
	// Function(ClassId id, std::string name, AObject
	// *(*native)(NativeFuncInput),
	//          bool isStatic, ClassId *args, bool* nullableArgs, argsSize,
	//          uint32_t returnId, bool returnNullable)
	//     : id(id), name(name), native(native), isStatic(isStatic),
	//       returnNullable(returnNullable), args(args),
	//       nullableArgs(nullableArgs), returnId(returnId),
	//       maxDeclaration(native ? nullableArgs.size() : 0) {}

	~Function() {
		// if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
		// 	bytecodes.~vector<uint8_t>();
		// }
		if (args)
			delete[] args;
	}

	std::string getName(CompiledProgram &data);

	// Support log
	std::string toString(CompiledProgram &data);
};

} // namespace AutoLang

#endif