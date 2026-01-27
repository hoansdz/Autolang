#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include "ankerl/unordered_dense.h"
#include "shared/Type.hpp"
#include "shared/InheritanceBitset.hpp"

struct AClass
{
	std::string name;
	uint32_t id;
	std::vector<AClass *> parent;
	std::vector<ClassId> memberId;
	ankerl::unordered_dense::map<std::string, MemberOffset> memberMap;
	ankerl::unordered_dense::map<std::string, std::vector<Offset>> funcMap;
	InheritanceBitset inheritance;
	AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
};

#endif