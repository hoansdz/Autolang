#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include "ankerl/unordered_dense.h"
#include "shared/Type.hpp"
#include "shared/InheritanceBitset.hpp"

namespace AutoLang {

struct CompiledProgram;

struct AClass
{
	std::string name;
	ClassId id;
	uint32_t classFlags;
	std::optional<ClassId> parentId;
	std::vector<ClassId> memberId;
	std::vector<FunctionId> vtable; // Override function
	HashMap<std::string, MemberOffset> memberMap;
	HashMap<std::string, std::vector<FunctionId>> funcMap;
	InheritanceBitset inheritance;
	AClass(){}
	// AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
	void log(CompiledProgram& data);
};

}

#endif