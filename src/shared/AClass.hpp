#ifndef ACLASS_HPP
#define ACLASS_HPP

#include <iostream>
#include <vector>
#include "ankerl/unordered_dense.h"

struct AClass
{
	std::string name;
	uint32_t id;
	std::vector<AClass *> parent;
	std::vector<uint32_t> memberId;
	ankerl::unordered_dense::map<std::string, uint32_t> memberMap;
	ankerl::unordered_dense::map<std::string, std::vector<uint32_t>> funcMap;
	AClass(std::string name, uint32_t id) : name(std::move(name)), id(id) {}
};

#endif