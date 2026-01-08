#ifndef DEBUGGER_CLASS_CPP
#define DEBUGGER_CLASS_CPP

#include <memory>
#include "DebuggerClass.hpp"

namespace AutoLang
{

CreateClassNode* loadClass(in_func, size_t& i) {
	if (!context.keywords.empty()) {
		throw std::runtime_error("Invalid keyword");
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
	CreateClassNode* node = context.newClasses.push(name); //NewClasses managed
	node->pushClass(in_data);
	auto lastClass = context.getCurrentClass(in_data);
	auto clazz = &compile.classes[node->classId];
	context.gotoClass(clazz);

	auto declarationNode = context.declarationNodePool.push(
		"this", "", true, false, false
	);
	declarationNode->classId = node->classId;
	//Add declaration this
	context.getCurrentClassInfo(in_data)->declarationThis = declarationNode;
	//Has PrimaryConstructor
	// bool hasPrimaryConstructor = false;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		context.getCurrentClassInfo(in_data)->primaryConstructor = context.createConstructorPool.push(context.currentClassId, name+"()", loadListDeclaration(in_data, i, true), true, 
			Lexer::TokenType::PUBLIC);
		context.getCurrentClassInfo(in_data)->primaryConstructor->pushFunction(in_data);
		compile.functions[context.getCurrentClassInfo(in_data)->primaryConstructor->funcId].maxDeclaration += context.getCurrentClassInfo(in_data)->primaryConstructor->arguments.size();
	}

	//Body
	if (!nextToken(&token, context.tokens, i)) {
		context.gotoClass(lastClass);
		return node;
	}

	if (expect(token, Lexer::TokenType::LBRACE)) {
		//'this' declaration
		declarationNode->id = 0;

		loadBody(in_data, node->body.nodes, i, false);

		//Create constructor if it hasn't constructor
		if (!context.getCurrentClassInfo(in_data)->primaryConstructor && 
			context.getCurrentClassInfo(in_data)->secondaryConstructor.empty()) {
			auto* constructor = context.createConstructorPool.push(
				context.currentClassId, name+"()", std::vector<DeclarationNode*>{}, false,
				Lexer::TokenType::PUBLIC
			);
			constructor->pushFunction(in_data);
			context.getCurrentClassInfo(in_data)->secondaryConstructor.push_back(constructor);
		}
		context.gotoClass(lastClass);
	} else {
		context.gotoClass(lastClass);
		--i;
	}
	return node;
}

void loadConstructor(in_func, size_t& i) {
	if (!context.keywords.empty())
		throw std::runtime_error("Invalid keyword");
	if (context.getCurrentClassInfo(in_data)->primaryConstructor)
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
	listDeclarationNode.insert(listDeclarationNode.begin(), context.getCurrentClassInfo(in_data)->declarationThis);
	//Body
	if (!expect(token, Lexer::TokenType::LBRACE)) {
		throw std::runtime_error("Expected body but not found");
	}
	//Create constructor
	auto constructor = context.createConstructorPool.push(context.currentClassId, context.getCurrentClass(in_data)->name + "()", 
		std::move(listDeclarationNode), false, Lexer::TokenType::PUBLIC);
	context.getCurrentClassInfo(in_data)->secondaryConstructor.push_back(constructor);
	constructor->pushFunction(in_data);
	compile.functions[constructor->funcId].maxDeclaration = constructor->arguments.size();
	context.gotoFunction(constructor->funcId);
	
	//Add to scope
	auto& scope = context.getCurrentFunctionInfo(in_data)->scopes.back();
	scope["this"] = context.getCurrentClassInfo(in_data)->declarationThis;

	for (size_t j = 1; j < constructor->arguments.size(); ++j) {
		auto* argument = constructor->arguments[j];
		scope[argument->name] = argument;
		argument->id = context.getCurrentFunctionInfo(in_data)->declaration++;
	}
	loadBody(in_data, constructor->body.nodes, i, false);
	context.gotoFunction(context.mainFunctionId);
}

// void loadClassInit(in_func, size_t& i) {
// 	if (!context.keywords.empty())
// 		throw std::runtime_error("Invalid keyword");
// 	context.gotoFunction(&compile.functions[context.getCurrentClassInfo(in_data)->initFunction->id]);
// 	Lexer::Token *token;
// 	//Body
// 	if (!nextToken(&token, context.tokens, i) ||
// 		!expect(token, Lexer::TokenType::LBRACE)) {
// 		throw std::runtime_error("Expected body but not found");
// 	}
// 	loadBody(in_data, context.getCurrentClassInfo(in_data)->initFunction->body.nodes, i, false);
// 	context.gotoFunction(context.mainFunction);
// }

}

#endif