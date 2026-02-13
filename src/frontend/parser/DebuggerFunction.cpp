#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "frontend/ACompiler.hpp"
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
		functionFlags |= FunctionFlags::FUNC_HAS_BODY;
		functionFlags |= FunctionFlags::FUNC_IS_NATIVE;
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		functionFlags |= FunctionFlags::FUNC_OVERRIDE;
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		functionFlags |= FunctionFlags::FUNC_NO_OVERRIDE;
	}
	if (context.annotationFlags & AnnotationFlags::AN_WAIT_INPUT) {
		functionFlags |= FunctionFlags::FUNC_HAS_BODY;
		functionFlags |= FunctionFlags::FUNC_WAIT_INPUT;
		if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
			throw ParserError(firstLine,
			                  "@wait_input must followed by @native");
		}
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
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	std::string name;
	switch (token->type) {
		case Lexer::TokenType::IDENTIFIER: {
			if (token->indexData == lexerIdSuper)
				throw ParserError(firstLine, "Super is a keyword");
			name = context.lexerString[token->indexData] + "()";
			break;
		}
		case Lexer::TokenType::LBRACKET: {
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::RBRACKET)) {
				--i;
				throw ParserError(firstLine, "Expected ] but not found");
			}
			name = "[]";
			break;
		}
		default: {
			--i;
			throw ParserError(firstLine, "Expected name but not found");
		}
	}
	// Arguments
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( but not found");
	}
	auto listDeclarationNode = loadListDeclaration(in_data, i);
	std::string returnClass;
	// Return class name
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE)
			goto createFunc;
		throw ParserError(firstLine, "Expected body but not found");
	}
	if (token->type == Lexer::TokenType::COLON) {
		ClassDeclaration classDeclaration =
		    loadClassDeclaration(in_data, i, token->line, true);
		returnClass = std::move(classDeclaration.className);
		if (classDeclaration.nullable) {
			functionFlags |= FunctionFlags::FUNC_RETURN_NULLABLE;
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE)
				goto createFunc;
			throw ParserError(firstLine, "Expected body but not found");
		}
	}

	switch (token->type) {
		case Lexer::TokenType::LBRACE: {
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
				--i;
				throw ParserError(firstLine,
				                  "@native function must not have a body");
			}
			break;
		}
		case Lexer::TokenType::EQUAL: {
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
				--i;
				throw ParserError(firstLine,
				                  "@native function must not have a body");
			}
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				throw ParserError(firstLine,
				                  "Expected value after = but not found");
			}
			if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
			    context.currentClassId) {
				listDeclarationNode.insert(
				    listDeclarationNode.begin(),
				    context.getCurrentClassInfo(in_data)->declarationThis);
			}
			CreateFuncNode *node = context.newFunctions.push(
			    firstLine, context.currentClassId, name,
			    std::move(returnClass), std::move(listDeclarationNode),
			    functionFlags);
			node->pushFunction(in_data);
			auto func = compile.functions[node->id];
			func->returnId = DefaultClass::nullClassId;
			context.gotoFunction(node->id);
			auto &scope =
			    context.getCurrentFunctionInfo(in_data)->scopes.back();

			for (size_t i = 0; i < node->arguments.size(); ++i) {
				auto *argument = node->arguments[i];
				argument->id = i;
				scope[argument->name] = argument;
			}

			auto returnNode =
			    context.returnPool.push(firstLine, context.currentFunctionId,
			                            loadExpression(in_data, 0, i));
			node->body.nodes.push_back(returnNode);
			context.gotoFunction(context.mainFunctionId);
			return node;
		}
		default: {
			--i;
			if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
				throw ParserError(firstLine, "Expected body but not found");
			}
			break;
		}
	}

createFunc:;

	// Add this
	if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
	    context.currentClassId) {
		listDeclarationNode.insert(
		    listDeclarationNode.begin(),
		    context.getCurrentClassInfo(in_data)->declarationThis);
	}

	CreateFuncNode *node = context.newFunctions.push(
	    firstLine, context.currentClassId, name, std::move(returnClass),
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
		for (size_t i = 0; i < node->arguments.size(); ++i) {
			auto *argument = node->arguments[i];
			argument->id = i;
		}
		context.gotoFunction(context.mainFunctionId);
		return node;
	} else {
		node->pushFunction(in_data);
	}
	// compile.funcMap[compile.functions[node->id].name]->push_back(node->id);
	auto func = compile.functions[node->id];
	// std::cerr<<"Created "<<name+"()"<<" ->
	// "<<compile.classes[func->returnId]->name<<"\n";
	context.gotoFunction(node->id);
	auto &scope = context.getCurrentFunctionInfo(in_data)->scopes.back();

	for (size_t i = 0; i < node->arguments.size(); ++i) {
		auto *argument = node->arguments[i];
		argument->id = i;
		scope[argument->name] = argument;
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