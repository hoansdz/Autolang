#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/SmallVector.hpp"
#include <cstdint>
#include <vector>

namespace Autolang {

struct DeclarationNode;
struct HasClassIdNode;

struct DestructurePatternItem {
	DeclarationNode *source;
	SmallVector<DeclarationNode *, 8> targets;
};

struct Parameter {
	SmallVector<DeclarationNode *, 4> parameters;
	SmallVector<HasClassIdNode *, 2> parameterDefaultValues;
	uint32_t defaultValuePos; // Parameters size if not
	std::vector<DestructurePatternItem> destructurePatterns;
	Parameter *copy(in_func);
};

} // namespace Autolang

#endif