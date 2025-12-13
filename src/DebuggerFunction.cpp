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
		if (hasAccessModifier && !context.currentClass) {
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
	if (token->type == Lexer::TokenType::COLON) {
		if (!nextToken(&token, context.tokens, i) ||
			!expect(token, Lexer::TokenType::IDENTIFIER)) {
			throw std::runtime_error("Expected class name but not found");
		}
		returnClass = context.lexerString[token->indexData];
		if (returnClass == "Null")
			throw std::runtime_error("Cannot return Null class");
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected body but not found");
		}
	}
	auto node = std::make_unique<CreateFuncNode>(
		context.currentClass->id, name, std::move(returnClass), 
		std::move(listDeclarationNode), isStatic, accessModifier
	);
	node->pushFunction(in_data);
	auto func = &compile.functions[node->id];
	context.gotoFunction(func);
	auto& scope = context.currentFuncInfo->scopes.back();
	if (!isStatic && context.currentClass) {
		//Add "this"
		scope["this"] = context.currentClassInfo->declarationThis;
		node->arguments.insert(node->arguments.begin(), context.currentClassInfo->declarationThis);
	}
	func->maxDeclaration += node->arguments.size();
	for (auto& argument:node->arguments) {
		scope[argument->name] = argument;
		argument->id = context.currentFuncInfo->declaration++;
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
	context.gotoFunction(compile.main);
	return node.release();
}

ReturnNode* loadReturn(in_func, size_t& i) {
	Lexer::Token *token = &context.tokens[i];
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		--i;
		auto value = context.currentFuncInfo->isConstructor ? new VarNode(context.currentClassInfo->declarationThis, false) : nullptr;
		return new ReturnNode(context.currentFunction, value);
	}
	if (context.currentFuncInfo->isConstructor)
		throw std::runtime_error("Cannot return value in constructor");
	return new ReturnNode(context.currentFunction, loadExpression(in_data, 0, i));
}

}

#endif