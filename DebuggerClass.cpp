#ifndef DEBUGGER_CLASS_CPP
#define DEBUGGER_CLASS_CPP

#include <memory>
#include "DebuggerClass.hpp"

namespace AutoLang
{

CreateClassNode* loadClass(in_func, size_t& i) {
	bool isDataClass = false;
	if (!context.keywords.empty()) {
		if (context.keywords[0] != Lexer::TokenType::DATA) {
			throw std::runtime_error("Invalid keyword");
		}
		isDataClass = true;
	}
	Lexer::Token *token;
	//Name
	if (!nextToken(&token, context.tokens, i) ||
		!expect(token, Lexer::TokenType::IDENTIFIER)) {
		throw std::runtime_error("Expected name but not found");
	}
	std::string& name = context.lexerString[token->indexData];
	//Body class
	if (!nextToken(&token, context.tokens, i))
		throw std::runtime_error("Expected body but not found");
	auto node = std::make_unique<CreateClassNode>(isDataClass, name);
	node->pushClass(in_data);
	auto lastClass = context.currentClass;
	auto clazz = &compile.classes[node->classId];
	context.gotoClass(clazz);
	auto declarationNode = new DeclarationNode(
		"this", "", true, false
	);
	//Has PrimaryConstructor
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (!isDataClass)
			throw std::runtime_error("Expected {}, did you want data class ?");
		context.currentClassInfo->primaryConstructor = new CreateConstructorNode(clazz, name+"()", loadListDeclaration(in_data, i, true), true, 
			Lexer::TokenType::PUBLIC);
		context.currentClassInfo->primaryConstructor->pushFunction(in_data);
		context.currentClassInfo->primaryConstructor->func->maxDeclaration += context.currentClassInfo->primaryConstructor->arguments.size();
	} else {
		--i;
	}
	//Body
	if (!nextToken(&token, context.tokens, i)) {
		context.gotoClass(lastClass);
		return node.release();
	}
	if (expect(token, Lexer::TokenType::LBRACE)) {
		//'this' declaration
		declarationNode->id = 0;
		declarationNode->classId = clazz->id;
		
		context.currentClassInfo->declarationThis = declarationNode;

		loadBody(in_data, node->body.nodes, i, false);

		//Create constructor if it hasn't constructor
		if (!context.currentClassInfo->primaryConstructor && 
			context.currentClassInfo->secondaryConstructor.empty()) {
			CreateConstructorNode* constructor = new CreateConstructorNode(
				clazz, name+"()", {context.currentClassInfo->declarationThis}, false, 
				Lexer::TokenType::PUBLIC
			);
			constructor->pushFunction(in_data);
			context.currentClassInfo->secondaryConstructor.push_back(constructor);
		}
		context.gotoClass(lastClass);
	} else {
		context.gotoClass(lastClass);
		--i;
	}
	return node.release();
}

void loadConstructor(in_func, size_t& i) {
	if (!context.keywords.empty())
		throw std::runtime_error("Invalid keyword");
	if (context.currentClassInfo->primaryConstructor)
		throw std::runtime_error("Cannot declare constructor in data class");
	Lexer::Token *token;
	//Arguments
	if (!nextToken(&token, context.tokens, i)) {
		throw std::runtime_error("Expected ( but not found");
	}
	std::vector<DeclarationNode*> listDeclarationNode;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		listDeclarationNode = loadListDeclaration(in_data, i);
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected body but not found");
		}
	}
	listDeclarationNode.insert(listDeclarationNode.begin(), context.currentClassInfo->declarationThis);
	//Body
	if (!expect(token, Lexer::TokenType::LBRACE)) {
		throw std::runtime_error("Expected body but not found");
	}
	//Create constructor
	auto constructor = new CreateConstructorNode(context.currentClass, context.currentClass->name + "()", 
		std::move(listDeclarationNode), false, Lexer::TokenType::PUBLIC);
	context.currentClassInfo->secondaryConstructor.push_back(constructor);
	constructor->pushFunction(in_data);
	constructor->func->maxDeclaration = constructor->arguments.size();
	context.gotoFunction(constructor->func);
	
	//Add to scope
	auto& scope = context.currentFuncInfo->scopes.back();
	scope["this"] = context.currentClassInfo->declarationThis;

	for (size_t j = 1; j < constructor->arguments.size(); ++j) {
		auto* argument = constructor->arguments[j];
		scope[argument->name] = argument;
		argument->id = context.currentFuncInfo->declaration++;
	}
	loadBody(in_data, constructor->body.nodes, i, false);
	context.gotoFunction(context.mainFunction);
}

// void loadClassInit(in_func, size_t& i) {
// 	if (!context.keywords.empty())
// 		throw std::runtime_error("Invalid keyword");
// 	context.gotoFunction(&compile.functions[context.currentClassInfo->initFunction->id]);
// 	Lexer::Token *token;
// 	//Body
// 	if (!nextToken(&token, context.tokens, i) ||
// 		!expect(token, Lexer::TokenType::LBRACE)) {
// 		throw std::runtime_error("Expected body but not found");
// 	}
// 	loadBody(in_data, context.currentClassInfo->initFunction->body.nodes, i, false);
// 	context.gotoFunction(context.mainFunction);
// }

}

#endif