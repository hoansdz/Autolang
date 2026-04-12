#ifndef CREATE_FUNC_NODE_HPP
#define CREATE_FUNC_NODE_HPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/node/Node.hpp"
#include <cmath>
#include <iostream>
#include <vector>

namespace AutoLang {

// func name(arguments): returnClass { body }
struct CreateFuncNode : ExprNode {
	std::optional<ClassId> contextCallClassId;
	LexerStringId nameId;
	ClassDeclaration *classDeclaration;
	FunctionId id;
	Parameter *parameter;
	uint32_t functionFlags;
	CreateFuncNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	               LexerStringId nameId, ClassDeclaration *classDeclaration,
	               Parameter *parameter, uint32_t functionFlags)
	    : ExprNode(NodeType::CREATE_FUNC, line),
	      contextCallClassId(contextCallClassId), nameId(nameId),
	      classDeclaration(classDeclaration), parameter(parameter),
	      functionFlags(functionFlags) {}
	void pushFunction(in_func);
	void pushNativeFunction(in_func, ANativeFunctionData *native);
	ExprNode *copy(in_func) override;
	void optimize(in_func) override;
	~CreateFuncNode() {}
};

} // namespace AutoLang

#endif