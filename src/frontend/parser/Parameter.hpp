#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/SmallVector.hpp"
#include <cstdint>

namespace Autolang {

struct DeclarationNode;
struct HasClassIdNode;

struct Parameter {
	SmallVector<DeclarationNode *, 4> parameters;
	SmallVector<HasClassIdNode *, 2> parameterDefaultValues;
	uint32_t defaultValuePos; // Parameters size if not
	Parameter *copy(in_func);
};

} // namespace Autolang

#endif