#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "frontend/parser/Debugger.hpp"
#include "shared/FunctionFlags.hpp"

namespace AutoLang {

CreateFuncNode *loadFunc(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	uint32_t functionFlags = 0;
	if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
		throw ParserError(firstLine,
		                  "@no_constructor is only supported classes");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
		functionFlags |= FunctionFlags::FUNC_IS_NATIVE;
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		functionFlags |= FunctionFlags::FUNC_OVERRIDE;
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		functionFlags |= FunctionFlags::FUNC_NO_OVERRIDE;
	}
	if (!context.currentClassId ||
	    (context.modifierflags & ModifierFlags::MF_STATIC)) {
		functionFlags |= FunctionFlags::FUNC_IS_STATIC;
		context.modifierflags &= ~ModifierFlags::MF_STATIC;
	}
	Lexer::TokenType accessModifier = getAndEnsureOneAccessModifier(in_data, i);
	switch (accessModifier) {
		case Lexer::TokenType::PUBLIC: {
			functionFlags |= FunctionFlags::FUNC_PUBLIC;
			break;
		}
		case Lexer::TokenType::PRIVATE: {
			functionFlags |= FunctionFlags::FUNC_PRIVATE;
			break;
		}
		case Lexer::TokenType::PROTECTED: {
			functionFlags |= FunctionFlags::FUNC_PROTECTED;
			break;
		}
		default:
			break;
	}
	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	std::string &name = context.lexerString[token->indexData];
	if (token->indexData == lexerIdSuper)
		throw ParserError(firstLine, "Super is a keyword");
	// Arguments
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( but not found");
	}
	auto listDeclarationNode = loadListDeclaration(in_data, i);
	std::string returnClass;
	bool returnNullable = false;
	// Return class name
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE)
			goto createFunc;
		throw ParserError(firstLine, "Expected body but not found");
	}
	if (token->type == Lexer::TokenType::COLON) {
		ClassDeclaration classDeclaration =
		    loadClassDeclaration(in_data, i, token->line);
		returnClass = std::move(classDeclaration.className);
		returnNullable = classDeclaration.nullable;
		if (returnClass == "Null")
			throw ParserError(firstLine, "Cannot return Null class");
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE)
				goto createFunc;
			throw ParserError(firstLine, "Expected body but not found");
		}
	}

	if (!expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
			throw ParserError(firstLine, "Expected body but not found");
		}
	} else {
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			--i;
			throw ParserError(firstLine,
			                  "@native function must not have a body");
		}
	}

createFunc:;

	if (returnNullable) {
		functionFlags |= FunctionFlags::FUNC_RETURN_NULLABLE;
	}

	// Add this
	if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
	    context.currentClassId) {
		// scope["this"] =
		// context.getCurrentClassInfo(in_data)->declarationThis;
		// node->arguments.insert(node->arguments.begin(),
		// context.getCurrentClassInfo(in_data)->declarationThis);
		listDeclarationNode.insert(
		    listDeclarationNode.begin(),
		    context.getCurrentClassInfo(in_data)->declarationThis);
	}

	CreateFuncNode *node = context.newFunctions.push(
	    firstLine, context.currentClassId, name + "()", std::move(returnClass),
	    std::move(listDeclarationNode), functionFlags);
	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto &token = context.annotationMetadata[AnnotationFlags::AN_NATIVE];
		auto &name = context.lexerString[token.indexData];
		auto it = context.mode->nativeFuncMap.find(name);
		if (it == context.mode->nativeFuncMap.end()) {
			throw ParserError(firstLine, "Native function name '" + name +
			                                 "' could not be found");
		}
		node->pushNativeFunction(in_data, it->second);
		auto func = compile.functions[node->id];
		context.gotoFunction(node->id);
		for (size_t i = 1; i < node->arguments.size(); ++i) {
			node->arguments[i]->id = i;
		}
		context.gotoFunction(context.mainFunctionId);
		return node;
	} else {
		node->pushFunction(in_data);
	}
	// compile.funcMap[compile.functions[node->id].name]->push_back(node->id);
	auto func = compile.functions[node->id];
	context.gotoFunction(node->id);
	auto &scope = context.getCurrentFunctionInfo(in_data)->scopes.back();

	if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
	    context.currentClassId) {
		// Add this
		scope["this"] = context.getCurrentClassInfo(in_data)->declarationThis;
	}

	for (size_t i = 1; i < node->arguments.size(); ++i) {
		node->arguments[i]->id = i;
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
	auto func = context.getCurrentFunction(in_data);
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		--i;
		auto declarartionThis =
		    context.getCurrentClassInfo(in_data)->declarationThis;
		auto value =
		    (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR)
		        ? new VarNode(firstLine, declarartionThis, false, false)
		        : nullptr;
		return context.returnPool.push(firstLine, context.currentFunctionId,
		                               value);
	}
	if (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR)
		throw ParserError(token->line, "Cannot return value in constructor");
	return context.returnPool.push(firstLine, context.currentFunctionId,
	                               loadExpression(in_data, 0, i));
}

} // namespace AutoLang

#endif