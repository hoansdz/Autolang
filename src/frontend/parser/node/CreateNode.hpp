#ifndef CREATE_NODE_HPP
#define CREATE_NODE_HPP

#include <iostream>
#include <vector>
#include <cmath>
#include "frontend/parser/node/Node.hpp"

namespace AutoLang
{

	inline bool isFunctionExist(in_func, std::string &name);
	inline bool isClassExist(in_func, std::string &name);
	inline bool isDeclarationExist(in_func, std::string &name);

	// var val name: className = value
	struct DeclarationNode : HasClassIdNode
	{
		Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
		std::string name;
		std::string className;
		Offset id;
		bool isGlobal;
		bool isVal;
		bool nullable;
		bool mustInferenceNullable = false;
		DeclarationNode(uint32_t line, std::string name, std::string className, bool isVal, bool isGlobal, bool nullable) : HasClassIdNode(NodeType::DECLARATION, 0, line), name(std::move(name)),
																							  className(std::move(className)), isGlobal(isGlobal), isVal(isVal), nullable(nullable) {}
		void optimize(in_func) override;
		~DeclarationNode() {}
	};

	struct CreateConstructorNode : HasClassIdNode
	{
		ClassId classId;
		Lexer::TokenType accessModifier;
		std::string name;
		Offset funcId;
		BlockNode body;
		const std::vector<DeclarationNode *> arguments;
		bool isPrimary;
		CreateConstructorNode(uint32_t line, ClassId classId, std::string name, std::vector<DeclarationNode *> arguments, bool isPrimary,
							  Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC) : HasClassIdNode(NodeType::CREATE_CONSTRUCTOR, 0, line), classId(classId), accessModifier(accessModifier),
																							name(std::move(name)), arguments(std::move(arguments)), isPrimary(isPrimary), body(line) {}
		void pushFunction(in_func);
		void optimize(in_func) override;
		//~CreateConstructorNode(){}
	};

	// class name(arguments) { body }
	struct CreateClassNode : HasClassIdNode
	{
		LexerStringId nameId;
		std::optional<LexerStringId> superId;
		BlockNode body;
		bool loadedSuper = false;
		CreateClassNode(uint32_t line, LexerStringId nameId, std::optional<LexerStringId> superId) : HasClassIdNode(NodeType::CREATE_CLASS, 0, line), body(line), nameId(nameId), superId(superId) {}
		void pushClass(in_func);
		void optimize(in_func) override;
		void loadSuper(in_func);
		~CreateClassNode() {}
	};

}

#endif