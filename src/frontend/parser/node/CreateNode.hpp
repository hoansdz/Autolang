#ifndef CREATE_NODE_HPP
#define CREATE_NODE_HPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/node/Node.hpp"
#include <cmath>
#include <iostream>
#include <vector>

namespace AutoLang {

// var val name: className = value
struct DeclarationNode : HasClassIdNode {
	std::optional<ClassId> contextCallClassId;
	LexerStringId baseName;
	std::string name;
	DeclarationOffset id;
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	bool isGlobal;
	bool isVal;
	bool nullable;
	bool mustInferenceNullable = false;
	bool loaded = false;
	DeclarationNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	                LexerStringId baseName, std::string name,
	                ClassDeclaration *classDeclaration, bool isVal,
	                bool isGlobal, bool nullable)
	    : HasClassIdNode(NodeType::DECLARATION, 0, line, classDeclaration),
	      contextCallClassId(contextCallClassId), baseName(baseName),
	      name(std::move(name)), isGlobal(isGlobal), isVal(isVal),
	      nullable(nullable) {}
	void optimize(in_func) override;
	ExprNode *copy(in_func) override;
	~DeclarationNode() {}
};

struct GenericDeclarationCondition {
	// enum Condition : uint32_t { MUST_EXTENDS, MUST_IS };
	// Condition condition;
	ClassDeclaration *classDeclaration;
};

struct GenericDeclarationNode : NullableNode {
	LexerStringId nameId;
	std::optional<GenericDeclarationCondition> condition;
	std::vector<ClassDeclaration *> allClassDeclarations;
	std::vector<CallNode *> allCallNodes;
	GenericDeclarationNode(uint32_t line, LexerStringId nameId)
	    : NullableNode(NodeType::GENERIC_DECLARATION, 0, false, line),
	      nameId(nameId) {}
};

struct CreateConstructorNode : HasClassIdNode {
	ClassId classId;
	LexerStringId nameId;
	FunctionId funcId;
	BlockNode body;
	Parameter *parameter;
	uint32_t functionFlags;
	bool isPrimary;
	CreateConstructorNode(uint32_t line, ClassId classId, LexerStringId nameId,
	                      Parameter *parameter, bool isPrimary,
	                      uint32_t functionFlags)
	    : HasClassIdNode(NodeType::CREATE_CONSTRUCTOR, 0, line),
	      classId(classId), nameId(nameId), body(line), parameter(parameter),
	      functionFlags(functionFlags), isPrimary(isPrimary) {}
	void pushFunction(in_func);
	ExprNode *copy(in_func) override;
	void optimize(in_func) override;
	//~CreateConstructorNode(){}
};

// class name(arguments) { body }
struct CreateClassNode : HasClassIdNode {
	LexerStringId nameId;
	ClassDeclaration *superDeclaration;
	uint32_t classFlags;
	BlockNode body;
	bool loadedSuper = false;
	CreateClassNode(uint32_t line, LexerStringId nameId, uint32_t classFlags)
	    : HasClassIdNode(NodeType::CREATE_CLASS, 0, line), body(line),
	      nameId(nameId), superDeclaration(nullptr), classFlags(classFlags) {}
	void pushClass(in_func);
	void optimize(in_func) override;
	void loadSuper(in_func);
	~CreateClassNode() {}
};

} // namespace AutoLang

#endif