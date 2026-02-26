#ifndef CREATE_NODE_HPP
#define CREATE_NODE_HPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/node/Node.hpp"
#include <cmath>
#include <iostream>
#include <vector>

namespace AutoLang {

inline bool isFunctionExist(in_func, std::string &name);
inline bool isClassExist(in_func, std::string &name);
inline bool isDeclarationExist(in_func, std::string &name);

// var val name: className = value
struct DeclarationNode : HasClassIdNode {
	std::optional<ClassId> contextCallClassId;
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	std::string name;
	FunctionId id;
	ClassDeclaration *classDeclaration;
	bool isGlobal;
	bool isVal;
	bool nullable;
	bool mustInferenceNullable = false;
	DeclarationNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	                std::string name, ClassDeclaration *classDeclaration,
	                bool isVal, bool isGlobal, bool nullable)
	    : HasClassIdNode(NodeType::DECLARATION, 0, line),
	      contextCallClassId(contextCallClassId), name(std::move(name)),
	      classDeclaration(classDeclaration), isGlobal(isGlobal), isVal(isVal),
	      nullable(nullable) {}
	void optimize(in_func) override;
	ExprNode *copy(in_func) override;
	~DeclarationNode() {}
};

struct GenericDeclarationNode : NullableNode {
	LexerStringId nameId;
	std::vector<ClassDeclaration *> allClassDeclarations;
	std::vector<CallNode *> allCallNodes;
	GenericDeclarationNode(uint32_t line, LexerStringId nameId)
	    : NullableNode(NodeType::GENERIC_DECLARATION, 0, false, line),
	      nameId(nameId) {}
};

struct CreateConstructorNode : HasClassIdNode {
	ClassId classId;
	std::string name;
	FunctionId funcId;
	BlockNode body;
	const std::vector<DeclarationNode *> arguments;
	uint32_t functionFlags;
	bool isPrimary;
	CreateConstructorNode(uint32_t line, ClassId classId, std::string name,
	                      std::vector<DeclarationNode *> arguments,
	                      bool isPrimary, uint32_t functionFlags)
	    : HasClassIdNode(NodeType::CREATE_CONSTRUCTOR, 0, line),
	      classId(classId), functionFlags(functionFlags), name(std::move(name)),
	      arguments(std::move(arguments)), isPrimary(isPrimary), body(line) {}
	void pushFunction(in_func);
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
	CreateClassNode(uint32_t line, LexerStringId nameId,
	                uint32_t classFlags)
	    : HasClassIdNode(NodeType::CREATE_CLASS, 0, line), body(line),
	      nameId(nameId), superDeclaration(nullptr), classFlags(classFlags) {}
	void pushClass(in_func);
	void optimize(in_func) override;
	void loadSuper(in_func);
	~CreateClassNode() {}
};

} // namespace AutoLang

#endif