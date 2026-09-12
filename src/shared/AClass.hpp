#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include <string_view>
#include "third_party/ankerl/unordered_dense.h"
#include "shared/SmallVector.hpp"
#include "shared/Type.hpp"
#include "shared/InheritanceBitset.hpp"
#include "shared/Bytecodes.hpp"

namespace Autolang {

struct CompiledProgram;

using GenericTypes = Bytecodes;

struct AClass
{
	StringArenaOffset nameStringOffset;
	ClassId id;
	uint32_t classFlags;
	ClassId parentId;
	ClassId genericBaseClassId;
	GenericTypes genericType;
	Offset memberIdOffset;
	SmallVector<FunctionId, 4> vtable; // Override function
	HashMap<std::string_view, MemberOffset> memberMap;
	HashMap<std::string_view, std::vector<FunctionId>> funcMap;
	InheritanceBitset inheritance;
	AClass(){}
	// AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
	void log(CompiledProgram& data);
	std::string getName(CompiledProgram &data);
};

}

#endif