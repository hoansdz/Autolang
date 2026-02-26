#ifndef CREATE_FUNC_NODE_HPP
#define CREATE_FUNC_NODE_HPP

#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include <cmath>
#include <iostream>
#include <vector>

namespace AutoLang {

// func name(arguments): returnClass { body }
struct CreateFuncNode : ExprNode {
	std::optional<ClassId> contextCallClassId;
	std::string name;
	ClassDeclaration *classDeclaration;
	FunctionId id;
	BlockNode body;
	const std::vector<DeclarationNode *> arguments;
	uint32_t functionFlags;
	CreateFuncNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	               std::string name, ClassDeclaration *classDeclaration,
	               std::vector<DeclarationNode *> arguments,
	               uint32_t functionFlags)
	    : ExprNode(NodeType::CREATE_FUNC, line),
	      contextCallClassId(contextCallClassId), name(std::move(name)),
	      classDeclaration(classDeclaration), arguments(std::move(arguments)),
	      body(line), functionFlags(functionFlags) {}
	void pushFunction(in_func);
	void pushNativeFunction(in_func, ANativeFunction native);
	void optimize(in_func) override;
	~CreateFuncNode() {}
};

} // namespace AutoLang

#endif