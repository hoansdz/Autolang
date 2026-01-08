#ifndef CREATE_NODE_HPP
#define CREATE_NODE_HPP

#include <iostream>
#include <vector>
#include "Node.hpp"

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
		uint32_t id;
		bool isGlobal;
		bool isVal;
		bool nullable;
		bool mustInferenceNullable = false;
		DeclarationNode(std::string name, std::string className, bool isVal, bool isGlobal, bool nullable) : HasClassIdNode(NodeType::DECLARATION), name(std::move(name)),
																							  className(std::move(className)), isGlobal(isGlobal), isVal(isVal), nullable(nullable) {}
		void optimize(in_func) override;
		~DeclarationNode() {}
	};

	// func name(arguments): returnClass { body }
	struct CreateFuncNode : HasClassIdNode
	{
		std::optional<uint32_t> contextCallClassId;
		Lexer::TokenType accessModifier;
		std::string name;
		std::string returnClass;
		uint32_t id;
		BlockNode body;
		const std::vector<DeclarationNode *> arguments;
		bool isStatic;
		bool returnNullable;
		CreateFuncNode(std::optional<uint32_t> contextCallClassId, std::string name, std::string returnClass, bool returnNullable, std::vector<DeclarationNode *> arguments,
					   bool isStatic, Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC) : HasClassIdNode(NodeType::CREATE_FUNC), contextCallClassId(contextCallClassId), accessModifier(accessModifier), name(std::move(name)), returnClass(std::move(returnClass)),
																									arguments(std::move(arguments)), isStatic(isStatic), returnNullable(returnNullable) {}
		void pushFunction(in_func);
		void optimize(in_func) override;
		~CreateFuncNode() {}
	};

	struct CreateConstructorNode : HasClassIdNode
	{
		std::optional<uint32_t> contextCallClassId;
		Lexer::TokenType accessModifier;
		std::string name;
		uint32_t funcId;
		BlockNode body;
		const std::vector<DeclarationNode *> arguments;
		bool isPrimary;
		CreateConstructorNode(std::optional<uint32_t> contextCallClassId, std::string name, std::vector<DeclarationNode *> arguments, bool isPrimary,
							  Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC) : HasClassIdNode(NodeType::CREATE_CONSTRUCTOR), contextCallClassId(contextCallClassId), accessModifier(accessModifier),
																							name(std::move(name)), arguments(std::move(arguments)), isPrimary(isPrimary) {}
		void pushFunction(in_func);
		void optimize(in_func) override;
		//~CreateConstructorNode(){}
	};

	// class name(arguments) { body }
	struct CreateClassNode : HasClassIdNode
	{
		std::string name;
		BlockNode body;
		CreateClassNode() : HasClassIdNode(NodeType::CREATE_CLASS) {}
		CreateClassNode(std::string name) : HasClassIdNode(NodeType::CREATE_CLASS), name(std::move(name)) {}
		void pushClass(in_func);
		void optimize(in_func) override;
		~CreateClassNode() {}
	};

}

#endif