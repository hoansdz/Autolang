#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "DebuggerFunction.hpp"

namespace AutoLang
{
	
CreateFuncNode* loadFunc(in_func, size_t& i) {
	bool isStatic = false;
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	if (!context.keywords.empty()) {
		bool hasAccessModifier = false;
		for (auto& keyword : context.keywords) {
			switch (keyword) {
				case Lexer::TokenType::STATIC:
					if (isStatic) break;
					isStatic = true;
					continue;
				case Lexer::TokenType::PRIVATE:
				case Lexer::TokenType::PROTECTED:
				case Lexer::TokenType::PUBLIC: {
					if (hasAccessModifier) break;
					hasAccessModifier = true;
					accessModifier = keyword;
					continue;
				}
				default:
					throw std::runtime_error("Invalid keyword '"+Lexer::Token(0, keyword).toString(context)+"'");
			}
			throw std::runtime_error("Keyword '"+Lexer::Token(0, keyword).toString(context)+"' has declared");
		}
		if (hasAccessModifier && !context.currentClassId) {
			throw std::runtime_error("Cannot declare function with keyword '"+Lexer::Token(0, accessModifier).toString(context)+" outside class");
		}
	}
	Lexer::Token *token;
	//Name
	if (!nextToken(&token, context.tokens, i) ||
		!expect(token, Lexer::TokenType::IDENTIFIER)) {
		throw std::runtime_error("Expected name but not found");
	}
	std::string& name = context.lexerString[token->indexData];
	//Arguments
	if (!nextToken(&token, context.tokens, i) ||
		 !expect(token, Lexer::TokenType::LPAREN)) {
		throw std::runtime_error("Expected ( but not found");
	}
	auto listDeclarationNode = loadListDeclaration(in_data, i);
	//Return class
	if (!nextToken(&token, context.tokens, i)) {
		throw std::runtime_error("Expected body but not found");
	}
	std::string returnClass;
	bool returnNullable = false;
	if (token->type == Lexer::TokenType::COLON) {
		ClassDeclaration classDeclaration = loadClassDeclaration(in_data, i , token->line);
		returnClass = std::move(classDeclaration.className);
		returnNullable = classDeclaration.nullable;
		if (returnClass == "Null")
			throw std::runtime_error("Cannot return Null class");
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected body but not found");
		}
	}

	//Add this
	if (!isStatic && context.currentClassId) {
		// scope["this"] = context.getCurrentClassInfo(in_data)->declarationThis;
		// node->arguments.insert(node->arguments.begin(), context.getCurrentClassInfo(in_data)->declarationThis);
		listDeclarationNode.insert(listDeclarationNode.begin(), context.getCurrentClassInfo(in_data)->declarationThis);
	}

	CreateFuncNode* node = context.newFunctions.push(
		context.currentClassId, name, std::move(returnClass), returnNullable,
		std::move(listDeclarationNode), isStatic, accessModifier
	);
	node->pushFunction(in_data);
	auto func = &compile.functions[node->id];
	context.gotoFunction(node->id);
	auto& scope = context.getCurrentFunctionInfo(in_data)->scopes.back();

	//Add this
	if (!isStatic && context.currentClassId) {
		scope["this"] = context.getCurrentClassInfo(in_data)->declarationThis;
	}

	func->maxDeclaration += node->arguments.size();
	for (auto& argument:node->arguments) {
		scope[argument->name] = argument;
		argument->id = context.getCurrentFunctionInfo(in_data)->declaration++;
	}
	loadBody(in_data, node->body.nodes, i);
	if (!node->returnClass.empty()) {
		bool hasReturn = false;
		for (size_t i = node->body.nodes.size(); i-- > 0;) {
			if (node->body.nodes[i]->kind == NodeType::RET) {
				hasReturn = true;
				break;
			}
		}
		if (!hasReturn)
			throw std::runtime_error("Didn't declare return");
	}
	context.gotoFunction(context.mainFunctionId);
	return node;
}

ReturnNode* loadReturn(in_func, size_t& i) {
	Lexer::Token *token = &context.tokens[i];
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		--i;
		auto value = context.getCurrentFunctionInfo(in_data)->isConstructor ? new VarNode(context.getCurrentClassInfo(in_data)->declarationThis, false, true) : nullptr;
		return context.returnPool.push(context.currentFunctionId, value);
	}
	if (context.getCurrentFunctionInfo(in_data)->isConstructor)
		throw std::runtime_error("Cannot return value in constructor");
	return context.returnPool.push(context.currentFunctionId, loadExpression(in_data, 0, i));
}

}

#endif