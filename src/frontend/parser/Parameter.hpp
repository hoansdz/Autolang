#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include "frontend/parser/node/OptimizeNode.hpp"

namespace AutoLang {

struct DeclarationNode;
struct HasClassIdNode;

struct Parameter {
	std::vector<DeclarationNode *> parameters;
	std::vector<HasClassIdNode *> parameterDefaultValues;
	uint32_t defaultValuePos; // Parameters size if not
	Parameter *copy(in_func);
};

} // namespace AutoLang

#endif