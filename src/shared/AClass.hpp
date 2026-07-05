#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include "third_party/ankerl/unordered_dense.h"
#include "shared/Type.hpp"
#include "shared/InheritanceBitset.hpp"
#include "shared/Bytecodes.hpp"

namespace AutoLang {

struct CompiledProgram;

using GenericTypes = Bytecodes;

struct AClass
{
	StringArenaOffset nameStringOffset;
	ClassId id;
	uint32_t classFlags;
	std::optional<ClassId> parentId;
	GenericTypes genericType;
	std::vector<ClassId> memberId;
	std::vector<FunctionId> vtable; // Override function
	HashMap<std::string, MemberOffset> memberMap;
	HashMap<std::string, std::vector<FunctionId>> funcMap;
	InheritanceBitset inheritance;
	AClass(){}
	// AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
	void log(CompiledProgram& data);
	std::string getName(CompiledProgram &data);
};

}

#endif