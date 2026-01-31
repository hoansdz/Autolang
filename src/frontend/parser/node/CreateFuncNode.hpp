#ifndef CREATE_FUNC_NODE_HPP
#define CREATE_FUNC_NODE_HPP

#include <cmath>
#include <iostream>
#include <vector>
#include "frontend/parser/node/Node.hpp"

namespace AutoLang {

// func name(arguments): returnClass { body }
struct CreateFuncNode : HasClassIdNode {
	std::optional<ClassId> contextCallClassId;
	Lexer::TokenType accessModifier;
	std::string name;
	std::string returnClass;
	Offset id;
	BlockNode body;
	const std::vector<DeclarationNode *> arguments;
	bool isStatic;
	bool returnNullable;
	CreateFuncNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	               std::string name, std::string returnClass,
	               bool returnNullable,
	               std::vector<DeclarationNode *> arguments, bool isStatic,
	               Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC)
	    : HasClassIdNode(NodeType::CREATE_FUNC, 0, line),
	      contextCallClassId(contextCallClassId),
	      accessModifier(accessModifier), name(std::move(name)),
	      returnClass(std::move(returnClass)), arguments(std::move(arguments)),
	      isStatic(isStatic), returnNullable(returnNullable), body(line) {}
	void pushFunction(in_func);
	void optimize(in_func) override;
	~CreateFuncNode() {}
};

} // namespace AutoLang

#endif