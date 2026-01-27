#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

CreateFuncNode *loadFunc(in_func, size_t &i) {
	bool isStatic = !context.currentClassId ||
	                (context.modifierflags & ModifierFlags::STATIC);
	if (isStatic)
		context.modifierflags &= ~ModifierFlags::STATIC;
	Lexer::TokenType accessModifier = getAndEnsureOneAccessModifier(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	std::string &name = context.lexerString[token->indexData];
	if (name == "super")
		throw ParserError(firstLine, "Super is a keyword");
	// Arguments
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( but not found");
	}
	auto listDeclarationNode = loadListDeclaration(in_data, i);
	// Return class name
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine, "Expected body but not found");
	}
	std::string returnClass;
	bool returnNullable = false;
	if (token->type == Lexer::TokenType::COLON) {
		ClassDeclaration classDeclaration =
		    loadClassDeclaration(in_data, i, token->line);
		returnClass = std::move(classDeclaration.className);
		returnNullable = classDeclaration.nullable;
		if (returnClass == "Null")
			throw ParserError(firstLine, "Cannot return Null class");
		if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
			--i;
			throw ParserError(firstLine, "Expected body but not found");
		}
	}

	// Add this
	if (!isStatic && context.currentClassId) {
		// scope["this"] =
		// context.getCurrentClassInfo(in_data)->declarationThis;
		// node->arguments.insert(node->arguments.begin(),
		// context.getCurrentClassInfo(in_data)->declarationThis);
		listDeclarationNode.insert(
		    listDeclarationNode.begin(),
		    context.getCurrentClassInfo(in_data)->declarationThis);
	}

	CreateFuncNode *node = context.newFunctions.push(
	    firstLine, context.currentClassId, name, std::move(returnClass),
	    returnNullable, std::move(listDeclarationNode), isStatic,
	    accessModifier);
	node->pushFunction(in_data);
	// compile.funcMap[compile.functions[node->id].name].push_back(node->id);
	auto func = &compile.functions[node->id];
	context.gotoFunction(node->id);
	auto &scope = context.getCurrentFunctionInfo(in_data)->scopes.back();

	if (!isStatic && context.currentClassId) {
		// Add this
		if (!isStatic) {
			scope["this"] =
			    context.getCurrentClassInfo(in_data)->declarationThis;
		}
	}

	func->maxDeclaration += node->arguments.size();
	for (auto &argument : node->arguments) {
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
			throw ParserError(firstLine, "Function " + func->name +
			                                 " didn't declare return");
	}
	context.gotoFunction(context.mainFunctionId);
	return node;
}

ReturnNode *loadReturn(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		--i;
		auto value =
		    context.getCurrentFunctionInfo(in_data)->isConstructor
		        ? new VarNode(
		              firstLine,
		              context.getCurrentClassInfo(in_data)->declarationThis,
		              false, false)
		        : nullptr;
		return context.returnPool.push(firstLine, context.currentFunctionId,
		                               value);
	}
	if (context.getCurrentFunctionInfo(in_data)->isConstructor)
		throw ParserError(token->line, "Cannot return value in constructor");
	return context.returnPool.push(firstLine, context.currentFunctionId,
	                               loadExpression(in_data, 0, i));
}

} // namespace AutoLang

#endif