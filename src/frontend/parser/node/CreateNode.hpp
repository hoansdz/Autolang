#ifndef CREATE_NODE_HPP
#define CREATE_NODE_HPP

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
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	std::string name;
	std::string className;
	Offset id;
	bool isGlobal;
	bool isVal;
	bool nullable;
	bool mustInferenceNullable = false;
	DeclarationNode(uint32_t line, std::string name, std::string className,
	                bool isVal, bool isGlobal, bool nullable)
	    : HasClassIdNode(NodeType::DECLARATION, 0, line), name(std::move(name)),
	      className(std::move(className)), isGlobal(isGlobal), isVal(isVal),
	      nullable(nullable) {}
	void optimize(in_func) override;
	~DeclarationNode() {}
};

struct CreateConstructorNode : HasClassIdNode {
	ClassId classId;
	std::string name;
	Offset funcId;
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
	LexerStringId superId;
	uint32_t classFlags;
	BlockNode body;
	bool loadedSuper = false;
	CreateClassNode(uint32_t line, LexerStringId nameId, LexerStringId superId,
	                uint32_t classFlags)
	    : HasClassIdNode(NodeType::CREATE_CLASS, 0, line), body(line),
	      nameId(nameId), superId(superId), classFlags(classFlags) {}
	void pushClass(in_func);
	void optimize(in_func) override;
	void loadSuper(in_func);
	~CreateClassNode() {}
};

} // namespace AutoLang

#endif