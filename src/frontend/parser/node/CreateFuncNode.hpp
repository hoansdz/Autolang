#ifndef CREATE_FUNC_NODE_HPP
#define CREATE_FUNC_NODE_HPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/node/Node.hpp"
#include <cmath>
#include <iostream>
#include <vector>

namespace Autolang {

// fun name(arguments): returnClass { body }
struct CreateFuncNode : ExprNode {
	std::optional<ClassId> contextCallClassId;
	LexerStringId nameId;
	uint32_t tokenIndex;
	FunctionId id;
	uint32_t functionFlags;
	ClassDeclaration *classDeclaration;
	Parameter *parameter;
	bool optimized;
	CreateFuncNode(uint32_t line, uint32_t tokenIndex,
	               std::optional<ClassId> contextCallClassId,
	               LexerStringId nameId, ClassDeclaration *classDeclaration,
	               Parameter *parameter, uint32_t functionFlags)
	    : ExprNode(NodeType::CREATE_FUNC, line),
	      contextCallClassId(contextCallClassId), nameId(nameId),
	      tokenIndex(tokenIndex), classDeclaration(classDeclaration),
	      parameter(parameter), functionFlags(functionFlags), optimized(false) {
	}
	template <bool addToGlobalScope = true> void pushFunction(in_func);
	template <bool addToGlobalScope = true>
	void pushNativeFunction(in_func, ANativeFunctionData *native);
	ExprNode *copy(in_func) override;
	ExprNode *optimize(in_func) override;
	~CreateFuncNode() {}
};

} // namespace Autolang

#endif