#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Import.hpp"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>

namespace Autolang {

void freeData(in_func) {
	for (auto *funcInfo : context.functionInfo) {
		funcInfo->body.refresh();
	}
	size_t sizeNewClasses = context.newClasses.getSize();
	for (size_t i = 0; i < sizeNewClasses; ++i) {
		context.newClasses[i]->body.refresh();
		context.newClasses[i]->body.nodes.clear();
	}
	// for (size_t i = 0 ; i < context.createConstructorPool.size; ++i) {
	// 	context.createConstructorPool[i]->body.refresh();
	// }
	// for (auto *node : context.staticNode) {
	// 	ExprNode::deleteNode(node);
	// }
	context.staticNode.clear();
	context.createConstructorPool.destroy();
	context.newClasses.refresh();
	context.newFunctions.refresh();
	context.declarationNodePool.refresh();
	// context.binaryNodePool.refresh();
}

void estimate(in_func, Lexer::Context &lexerContext) {
	uint32_t estimateNewClasses = lexerContext.estimate.classes;
	uint32_t estimateNewConstructorNode =
	    lexerContext.estimate.classes + lexerContext.estimate.constructorNode;
	uint32_t estimateNewFunctions =
	    lexerContext.estimate.functions + lexerContext.estimate.constructorNode;
	uint32_t estimateAllClasses = compile.classes.size() + estimateNewClasses;
	uint32_t estimateAllFunctions =
	    compile.functions.size() + estimateNewFunctions + estimateNewClasses;
	uint32_t estimateDeclaration =
	    lexerContext.estimate.declaration + estimateNewClasses;
	// uint32_t estimateBinaryNode = lexerContext.estimate.binaryNode;

	context.modifierflags = 0;

	// context.createConstructorPool.allocate(estimateNewConstructorNode);
	context.declarationNodePool.allocate(estimateDeclaration);
	// context.ifPool.allocate(lexerContext.estimate.ifNode);
	// context.whilePool.allocate(lexerContext.estimate.whileNode);
	// context.returnPool.allocate(lexerContext.estimate.returnNode +
	//                             lexerContext.estimate.constructorNode +
	//                             estimateNewClasses);
	// context.setValuePool.allocate(lexerContext.estimate.setNode);
	// context.tryCatchPool.allocate(lexerContext.estimate.tryCatchNode);
	// context.throwPool.allocate(lexerContext.estimate.throwNode);
	// context.binaryNodePool.allocate(estimateBinaryNode);

	context.constValue.reserve(3); // Const

	context.newClasses.allocate(estimateNewClasses);
	context.newFunctions.allocate(estimateNewFunctions);

	context.classInfo.reserve(estimateAllClasses);
	context.functionInfo.reserve(estimateAllFunctions);

	compile.classes.reserve(estimateAllClasses);
	compile.classMap.reserve(estimateAllClasses);
	compile.functions.reserve(estimateAllFunctions);
	compile.funcMap.reserve(estimateAllFunctions);

	printDebug("Estimate Declarations: " + std::to_string(estimateDeclaration));
	printDebug("Estimate New classes: " + std::to_string(estimateNewClasses));
	printDebug("Estimate New functions: " +
	           std::to_string(estimateNewFunctions));
	printDebug("Estimate All classes: " + std::to_string(estimateAllClasses));
	printDebug("Estimate All functions: " +
	           std::to_string(estimateAllFunctions));
}

ExprNode *loadLine(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	context.modifierflags = 0;
	context.annotationFlags = 0;
initial:;
	bool isInFunction = !context.currentClassId ||
	                    context.currentFunctionId != context.mainFunctionId ||
	                    context.currentClosureNode;
	switch (token->type) {
		case Lexer::TokenType::PLUS_PLUS:
		case Lexer::TokenType::MINUS_MINUS:
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::EXMARK:
		case Lexer::TokenType::MINUS: {
			auto op = token->type;
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Expected value after '" +
				                      Lexer::Token(0, op).toString(context) +
				                      "'\nHint: Provide an operand or "
				                      "expression after unary operator");
			}
			return context.unaryNodePool.push(
			    token->line,
			    op == Lexer::TokenType::EXMARK ? Lexer::TokenType::NOT : op,
			    parsePrimary(in_data, i));
		}
		case Lexer::TokenType::END_IMPORT: {
			context.loadingLibs.pop_back();
			ParserContext::mode = context.loadingLibs.back();
			if (!nextToken(&token, context.tokens, i)) {
				return nullptr;
			}
			goto initial;
		}
		case Lexer::TokenType::LBRACE:
		case Lexer::TokenType::LPAREN:
		case Lexer::TokenType::LBRACKET:
		case Lexer::TokenType::NUMBER:
		case Lexer::TokenType::STRING:
		case Lexer::TokenType::IDENTIFIER: {
			if (!isInFunction) {
				goto err_call_func;
			}
			return loadExpression(in_data, 0, i);
		}
		case Lexer::TokenType::VAR:
		case Lexer::TokenType::VAL: {
			auto node = loadDeclaration(in_data, i);
			return node;
		}
		case Lexer::TokenType::BREAK:
		case Lexer::TokenType::CONTINUE: {
			if (!isInFunction)
				goto err_call_func;
			if (!context.canBreakContinue)
				throw ParserError(
				    token->line,
				    "'" + Lexer::Token(0, token->type).toString(context) +
				        "' only allowed inside a loop\nHint: Move 'break' or "
				        "'continue' inside a 'for' or 'while' loop");
			return context.skipNodePool.push(token->type, token->line);
		}
		case Lexer::TokenType::TRY: {
			if (!isInFunction)
				goto err_call_func;
			return loadTryCatch(in_data, i);
		}
		case Lexer::TokenType::THROW: {
			if (!isInFunction)
				goto err_call_func;
			return loadThrow(in_data, i);
		}
		case Lexer::TokenType::IF: {
			if (!isInFunction)
				goto err_call_func;
			return loadIf(in_data, i, false);
		}
		case Lexer::TokenType::WHEN: {
			if (!isInFunction)
				goto err_call_func;
			return loadWhen(in_data, i, false);
		}
		case Lexer::TokenType::FOR: {
			if (!isInFunction)
				goto err_call_func;
			bool lastCanBreakContinue = context.canBreakContinue;
			context.canBreakContinue = true;
			auto node = loadFor(in_data, i);
			context.canBreakContinue = lastCanBreakContinue;
			return node;
		}
		case Lexer::TokenType::WHILE: {
			if (!isInFunction)
				goto err_call_func;
			bool lastCanBreakContinue = context.canBreakContinue;
			context.canBreakContinue = true;
			auto node = loadWhile(in_data, i);
			context.canBreakContinue = lastCanBreakContinue;
			return node;
		}
		case Lexer::TokenType::FUNC: {
			if (context.currentFunctionId != context.mainFunctionId) {
				throw ParserError(token->line,
				                  "Error: Function declarations are not "
				                  "allowed inside another function\nHint: Move "
				                  "function declaration out to top-level or "
				                  "class scope");
			}
			if (context.currentClosureNode) {
				throw ParserError(token->line,
				                  "Error: Function declarations are not "
				                  "allowed inside closure\nHint: Move function "
				                  "declaration out of closure");
			}
			auto node = loadFunc(in_data, i);
			if (!node)
				throw ParserError(
				    0, "Internal error: loadFunc returned null unexpectedly");
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				classInfo->createFunctionNodes.push_back(
				    static_cast<CreateFuncNode *>(node));
			}
			return nullptr;
		}
		case Lexer::TokenType::CONSTRUCTOR: {
			if (context.currentFunctionId != context.mainFunctionId) {
				throw ParserError(
				    token->line,
				    "Error: Constructor declarations are not "
				    "allowed inside another function\nHint: "
				    "Declare constructors directly inside a class");
			}
			if (!context.currentClassId) {
				throw ParserError(
				    token->line,
				    "Error: Constructor declarations are not "
				    "allowed outside class\nHint: Move constructor "
				    "inside a class definition");
			}
			if (context.currentClosureNode) {
				throw ParserError(token->line,
				                  "Error: Constructor declarations are not "
				                  "allowed inside closure\nHint: Declare "
				                  "constructors directly inside a class body");
			}
			loadConstructor(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::ENUM: {
			if (context.currentClassId) {
				throw ParserError(token->line,
				                  "Cannot declare enum in class\nHint: Declare "
				                  "enums at file/top-level scope, not inside a "
				                  "class");
			}
			loadEnum(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::CLASS: {
			if (context.currentClassId) {
				throw ParserError(token->line,
				                  "Error: Class declarations are not "
				                  "allowed inside other class\nHint: Move "
				                  "class declaration to top-level scope");
			}
			if (context.currentFunctionId != context.mainFunctionId) {
				throw ParserError(token->line,
				                  "Error: Class declarations are not "
				                  "allowed inside function\nHint: Move class "
				                  "declaration to top-level scope");
			}
			if (context.currentClosureNode) {
				throw ParserError(token->line,
				                  "Error: Class declarations are not "
				                  "allowed inside closure\nHint: Move class "
				                  "declaration to top-level scope");
			}
			auto node = loadClass(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::RETURN: {
			return loadReturn(in_data, i);
		}
		case Lexer::TokenType::AT_SIGN: {
			if (context.currentFunctionId != context.mainFunctionId) {
				throw ParserError(token->line,
				                  "Error: Annotation declaration are not "
				                  "allowed inside function\nHint: Place "
				                  "annotations on top-level or class members");
			}
			loadAnnotations(in_data, i);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				return nullptr;
			}
			goto initial;
		}
		case Lexer::TokenType::PUBLIC: {
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(
				    token->line, "Duplicate modifier 'public'\nHint: Use "
				                 "'public' modifier only once per declaration");
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(token->line,
				                  "Error: Invalid modifier combination: "
				                  "'public' and 'private'\nHint: Choose either "
				                  "'public' or 'private', not both");
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(
				    token->line,
				    "Error: Invalid modifier combination: "
				    "'public' and 'protected'\nHint: Choose either "
				    "'public' or 'protected', not both");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Error: 'public' must be followed by a declaration\nHint: "
				    "Follow 'public' with a class, function, or variable "
				    "declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PUBLIC;
			goto initial;
		}
		case Lexer::TokenType::PRIVATE: {
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(token->line,
				                  "Error: Duplicate modifier 'private'\nHint: "
				                  "Use 'private' modifier only once per "
				                  "declaration");
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(token->line,
				                  "Error: Invalid modifier combination: "
				                  "'private' and 'public'\nHint: Choose either "
				                  "'private' or 'public', not both");
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(token->line,
				                  "Error: Invalid modifier combination: "
				                  "'private' and 'protected'\nHint: Choose "
				                  "either 'private' or 'protected', not both");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Error: 'private' must be followed by a declaration\nHint: "
				    "Follow 'private' with a class, function, or variable "
				    "declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PRIVATE;
			goto initial;
		}
		case Lexer::TokenType::PROTECTED: {
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(token->line,
				                  "Duplicate modifier 'protected'\nHint: Use "
				                  "'protected' modifier only once per "
				                  "declaration");
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(
				    token->line,
				    "Error: Invalid modifier combination: "
				    "'protected' and 'public'\nHint: Choose either "
				    "'protected' or 'public', not both");
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(
				    token->line,
				    "Error: Invalid modifier combination: "
				    "'protected' and 'private'\nHint: Choose either "
				    "'protected' or 'private', not both");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Error: 'protected' must be followed by a "
				    "declaration\nHint: "
				    "Follow 'protected' with a class, function, or variable "
				    "declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PROTECTED;
			goto initial;
		}
		case Lexer::TokenType::STATIC: {
			if (context.modifierflags & ModifierFlags::MF_STATIC)
				throw ParserError(
				    token->line,
				    "Error: Duplicate modifier 'static'\nHint: Use "
				    "'static' modifier only once per declaration");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Error: 'static' must be followed by a declaration\nHint: "
				    "Follow 'static' with a function or variable declaration");
			}
			context.modifierflags |= ModifierFlags::MF_STATIC;
			goto initial;
		}
		case Lexer::TokenType::TYPEALIAS: {
			loadTypealias(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::LATEINIT: {
			if (!(context.mode->flags & LibraryFlags::ALLOW_LATEINIT_KEYWORD)) {
				throw ParserError(
				    token->line,
				    "Error: 'lateinit' keyword is disabled. "
				    "\nNote: Enable 'allowLateinitKeyword' to use it.");
			}
			if (context.modifierflags & ModifierFlags::MF_LATEINIT)
				throw ParserError(token->line,
				                  "Error: Duplicate modifier 'lateinit'\nHint: "
				                  "Use 'lateinit' modifier only once per "
				                  "declaration");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Error: 'lateinit' must be followed by a "
				                  "declaration\nHint: "
				                  "Follow 'lateinit' with a var declaration");
			}
			context.modifierflags |= ModifierFlags::MF_LATEINIT;
			goto initial;
		}
		case Lexer::TokenType::SEMI_COLON: {
			throw ParserError(token->line,
			                  "Semicolon ';' is not supported in "
			                  "Autolang\nHint: Remove ';' from your code");
		}
		default:
			throw ParserError(
			    token->line,
			    "Unexpected token " + token->toString(context) +
			        "\nHint: Ensure correct syntax or remove unexpected token");
	}
	++i;
	return nullptr;
err_call_func:;
	throw ParserError(
	    token->line,
	    "Command are not allowed outside a function \nHint: Move statements or "
	    "expressions inside a function body");
	// err_call_class:;
	// 	throw ParserError(token->line, "Command are not allowed outside class
	// ");
}

template <bool loadedLBrace>
bool loadBody(in_func, SmallVector<ExprNode *, 8> &nodes, size_t &i,
              bool createScope) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (createScope)
		context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
	if constexpr (!loadedLBrace) {
		if (token->type != Lexer::TokenType::LBRACE) {
			nodes.push_back(loadLine(in_data, i));
			if (createScope)
				context.getCurrentFunctionInfo(in_data)->popBackScope();
			return true;
		}
	}
	while (nextToken(&token, context.tokens, i)) {
		if (token->type == Lexer::TokenType::RBRACE) {
			if (createScope)
				context.getCurrentFunctionInfo(in_data)->popBackScope();
			return true;
		}
		try {
			auto node = loadLine(in_data, i);
			ensureEndline(in_data, i);
			if (node == nullptr)
				continue;
			if (context.annotationFlags || context.modifierflags) {
				throw ParserError(
				    node->line,
				    "Bug: Annotations and modifiers hasn't reset yet");
			}
			nodes.push_back(node);
		} catch (const ParserError &err) {
			context.hasError = true;
			context.logError(err.line, err.message);
			Lexer::Token *token;
			uint32_t countScope = 1;
			while (nextToken(&token, context.tokens, i)) {
				switch (token->type) {
					case Lexer::TokenType::LBRACE: {
						++countScope;
						break;
					}
					case Lexer::TokenType::RBRACE: {
						--countScope;
						if (countScope == 0) {
							if (createScope) {
								context.getCurrentFunctionInfo(in_data)
								    ->popBackScope();
							}
							return false;
						}
						break;
					}
					default:
						break;
				}
			}
		}
	}
	throw ParserError(
	    firstLine,
	    "Expected } but not found\nHint: Close the block with a matching '}' "
	    "bracket");
}

void ensureEndline(in_func, size_t &i) {
	if (i >= context.tokens.size())
		return;
	Lexer::Token *token = &context.tokens[i];
	if (nextTokenSameLine(&token, context.tokens, i, token->line)) {
		if (token->type == Lexer::TokenType::RBRACE) {
			--i;
			return;
		}
		if (token->type == Lexer::TokenType::SEMI_COLON) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Semicolon ';' is not supported in "
			                  "Autolang\nHint: Remove ';' from your code");
		}
		std::string line = token->toString(context);
		while (nextTokenSameLine(&token, context.tokens, i, token->line)) {
			line += " " + token->toString(context);
			break;
		}
		--i;
		throw ParserError(
		    context.tokens[i].line,
		    "Multiple commands are not allowed on a single line: " + line +
		        "\nHint: Separate commands onto distinct lines");
	}
	--i;
}

template bool loadBody<false>(in_func, SmallVector<ExprNode *, 8> &nodes,
                              size_t &i, bool createScope);
template bool loadBody<true>(in_func, SmallVector<ExprNode *, 8> &nodes,
                             size_t &i, bool createScope);

} // namespace Autolang

#endif