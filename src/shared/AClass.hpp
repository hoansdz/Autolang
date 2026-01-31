#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include "ankerl/unordered_dense.h"
#include "shared/Type.hpp"
#include "shared/InheritanceBitset.hpp"

struct CompiledProgram;

struct AClass
{
	std::string name;
	ClassId id;
	std::optional<ClassId> parentId;
	std::vector<ClassId> memberId;
	std::vector<Offset> vtable; // Override function
	HashMap<std::string, MemberOffset> memberMap;
	HashMap<std::string, std::vector<Offset>> funcMap;
	InheritanceBitset inheritance;
	AClass(){}
	AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
	void log(CompiledProgram& data);
};

#endif