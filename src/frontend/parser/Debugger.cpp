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
			throw ParserError(
			    token->line,
			    "Semicolon ';' is not supported in Autolang\nHint: Remove ';' from your code");
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
bool loadBody(in_func, std::vector<ExprNode *> &nodes, size_t &i,
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

HasClassIdNode *loadExpression(in_func, int minPrecedence, size_t &i) {
	HasClassIdNode *left = parsePrimary(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	uint32_t tokenIndex = i;
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			case Lexer::TokenType::COMMA:
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE: {
				--i;
				return left;
			}
			default:
				break;
		};
		int precedence = getPrecedence(token->type);
		if (precedence == -1 || precedence < minPrecedence)
			break;
		// if (firstLine != token->line) {
		// 	--i;
		// 	return left;
		// }
		Lexer::TokenType op = token->type;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected expression after operator but not found\nHint: "
			    "Provide a valid right operand expression after operator");
		}
		HasClassIdNode *right = loadExpression(in_data, precedence + 1, i);
		switch (op) {
			case Lexer::TokenType::DOT_DOT_LT: {
				left = context.rangeNode.push(firstLine, left, right, true);
				continue;
			}
			case Lexer::TokenType::DOT_DOT: {
				left = context.rangeNode.push(firstLine, left, right, false);
				continue;
			}
			case Lexer::TokenType::QMARK_QMARK: {
				left = context.nullCoalescingPool.push(firstLine, left, right);
				continue;
			}
			default:
				break;
		}
		// auto binaryNode = context.binaryNodePool.push(op, left, right);
		left = context.binaryNodePool.push(
		    firstLine, tokenIndex, context.currentClassId, op, left, right);
		// auto binaryNode =
		//     std::make_unique<BinaryNode>(firstLine, op, left.release(),
		//     right);
		// if (minPrecedence == 0) {
		// 	left.reset(binaryNode);
		// 	auto node = binaryNode->calculate(in_data);
		// 	if (node != nullptr) {
		// 		// binaryNode->left = nullptr;
		// 		// binaryNode->right = nullptr;
		// 		left.reset(node);
		// 		continue;
		// 	}
		// }
		// left.reset(binaryNode.release());
		// std::cerr<<"op "<<binaryNode<<":"<<Lexer::Token(0,
		// binaryNode->op,
		// "").toString()<<'\n';
	}
	--i;
	return left;
}

template <bool trailingComma>
std::vector<HasClassIdNode *> loadListArgument(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	char openBracket = getOpenBracket(token->type);
	if (openBracket == '\0')
		throw ParserError(
		    token->line,
		    "Unexpected token " + token->toString(context) +
		        "\nHint: Ensure arguments list opens with a valid bracket '('");
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(0, "Bug: Lexer did not ensure a closing bracket");
	}
	std::vector<HasClassIdNode *> nodes;
	switch (token->type) {
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET:
		case Lexer::TokenType::RBRACE: {
			if (!isCloseBracket(openBracket, token->type)) {
				for (auto *node : nodes)
					ExprNode::deleteNode(node);
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			return nodes;
		}
		default: {
			nodes.push_back(loadExpression(in_data, 0, i));
			break;
		}
	}
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::LPAREN:
			case Lexer::TokenType::LBRACKET:
			case Lexer::TokenType::LBRACE: {
				nodes.push_back(loadExpression(in_data, 0, i));
				break;
			}
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE: {
				if (!isCloseBracket(openBracket, token->type)) {
					// for (auto *node : nodes)
					// 	ExprNode::deleteNode(node);
					throw ParserError(
					    token->line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				return nodes;
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i))
					goto expectedCloseBracket;
				if constexpr (trailingComma) {
					switch (token->type) {
						case Lexer::TokenType::RPAREN:
						case Lexer::TokenType::RBRACKET:
						case Lexer::TokenType::RBRACE: {
							if (!isCloseBracket(openBracket, token->type)) {
								for (auto *node : nodes)
									ExprNode::deleteNode(node);
								throw ParserError(token->line,
								                  "Bug: Lexer did not ensure a "
								                  "closing bracket");
							}
							return nodes;
						}
						default:
							break;
					}
				}
				nodes.push_back(loadExpression(in_data, 0, i));
				break;
			}
			default: {
				--i;
				throw ParserError(token->line,
				                  "Unknown token " + token->toString(context) +
				                      "\nHint: Provide a valid argument "
				                      "expression or closing "
				                      "bracket");
			}
		}
	}
expectedCloseBracket:
	for (auto *node : nodes)
		ExprNode::deleteNode(node);
	throw ParserError(token->line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

HasClassIdNode *inferenceNodeFromLBrace(in_func, size_t &i,
                                        NodeType canBeNodeType) {
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(0, "Bug: Lexer did not ensure a closing bracket");
	}
	switch (token->type) {
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET: {
			throw ParserError(token->line,
			                  "Bug: Lexer did not ensure a closing bracket");
		}
		case Lexer::TokenType::RBRACE: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					return context.createSetPool.push(
					    token->line, nullptr, std::vector<HasClassIdNode *>());
				}
				case NodeType::CREATE_MAP: {
					return context.createMapPool.push(
					    token->line, nullptr,
					    std::vector<
					        std::pair<HasClassIdNode *, HasClassIdNode *>>());
				}
				default: {
					return context.createSetPool.push(
					    token->line, nullptr, std::vector<HasClassIdNode *>());
				}
			}
			break;
		}
		case Lexer::TokenType::OR_OR: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					throw ParserError(
					    token->line,
					    "Expected Set<> but closure found\nHint: Do not pass "
					    "closure parameters inside Set literal");
				}
				case NodeType::CREATE_MAP: {
					throw ParserError(
					    token->line,
					    "Expected Map<> but closure found\nHint: Do not pass "
					    "closure parameters inside Map literal");
				}
				default:
					break;
			}
			--i;
			return loadClosure<false>(in_data, i);
		}
		case Lexer::TokenType::OR: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					throw ParserError(
					    token->line,
					    "Expected Set<> but closure found\nHint: Do not pass "
					    "closure parameters inside Set literal");
				}
				case NodeType::CREATE_MAP: {
					throw ParserError(
					    token->line,
					    "Expected Map<> but closure found\nHint: Do not pass "
					    "closure parameters inside Map literal");
				}
			}
			--i;
			return loadClosure(in_data, i);
		}
		default: {
			auto firstExpression = loadExpression(in_data, 0, i);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    0, "Bug: Lexer did not ensure a closing bracket");
			}
			switch (token->type) {
				case Lexer::TokenType::COMMA: {
					if (canBeNodeType == NodeType::CREATE_MAP) {
						throw ParserError(
						    token->line,
						    "Expected Map<> but Set<> found\nHint: Map "
						    "elements must use 'key: value' pairs");
					}
					return loadSet(in_data, i, firstExpression);
				}
				case Lexer::TokenType::COLON: {
					if (canBeNodeType == NodeType::CREATE_SET) {
						throw ParserError(
						    token->line,
						    "Expected Set<> but Map<> found\nHint: Set "
						    "elements must be single values without ':'");
					}
					return loadMap(in_data, i, firstExpression);
				}
				case Lexer::TokenType::RPAREN:
				case Lexer::TokenType::RBRACKET: {
					throw ParserError(
					    token->line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				case Lexer::TokenType::RBRACE: {
					return context.createSetPool.push(
					    token->line, nullptr,
					    std::vector<HasClassIdNode *>{firstExpression});
				}
			}
			throw ParserError(
			    token->line,
			    "Unexpected token " + token->toString(context) +
			        "\nHint: Expected ',' or ':' inside literal structure");
		}
	}
}

HasClassIdNode *loadSet(in_func, size_t &i, HasClassIdNode *firstExpression) {
	Lexer::Token *token;
	std::vector<HasClassIdNode *> values = {firstExpression};
	--i;
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET: {
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			case Lexer::TokenType::RBRACE: {
				return context.createSetPool.push(token->line, nullptr,
				                                  std::move(values));
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				if (expect(token, Lexer::TokenType::RBRACE)) {
					return context.createSetPool.push(token->line, nullptr,
					                                  std::move(values));
				}
				values.push_back(loadExpression(in_data, 0, i));
				break;
			}
			default: {
				--i;
				throw ParserError(
				    token->line,
				    "Unknown token " + token->toString(context) +
				        "\nHint: Expected element expression or ',' inside set "
				        "literal");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

HasClassIdNode *loadMap(in_func, size_t &i, HasClassIdNode *firstExpression) {
	std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values;
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Bug: Lexer did not ensure a closing bracket");
	}
	values.push_back(
	    std::make_pair(firstExpression, loadExpression(in_data, 0, i)));
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET: {
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			case Lexer::TokenType::RBRACE: {
				return context.createMapPool.push(token->line, nullptr,
				                                  std::move(values));
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				if (expect(token, Lexer::TokenType::RBRACE)) {
					return context.createMapPool.push(token->line, nullptr,
					                                  std::move(values));
				}
				auto key = loadExpression(in_data, 0, i);
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::COLON)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected :\nHint: Use ':' to separate "
					                  "key and value in "
					                  "map entry");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				values.push_back(
				    std::make_pair(key, loadExpression(in_data, 0, i)));
				break;
			}
			default: {
				--i;
				throw ParserError(
				    token->line,
				    "Unknown token " + token->toString(context) +
				        "\nHint: Expected map entry 'key: value' or closing "
				        "bracket '}'");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

template <Lexer::TokenType closeBracket, bool mustHaveColon,
          bool allowDefaultValue>
Parameter *loadListDeclaration(in_func, size_t &i, bool allowVar) {
	Lexer::Token *token = &context.tokens[i];
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Bug: Lexer did not ensure a closing bracket");
	}
	auto parameter = context.parameterPool.push();
	switch (token->type) {
		case closeBracket: {
			parameter->defaultValuePos = 0;
			return parameter;
		}
		case Lexer::TokenType::VAR:
		case Lexer::TokenType::VAL: {
			if (!allowVar)
				throw ParserError(
				    token->line,
				    token->toString(context) +
				        " can't be allowed here\nHint: Do not use 'var' or "
				        "'val' keywords in function parameters unless in "
				        "constructor");
		}
		case Lexer::TokenType::IDENTIFIER: {
			--i;
			break;
		}
		default:
			throw ParserError(
			    token->line,
			    "Expected parameter name but not found\nHint: Provide a "
			    "parameter name identifier");
	}
	bool addedDefaultValue = false;
	while (nextToken(&token, context.tokens, i)) {
		bool isVal = true;
		if (allowVar) {
			if (!expect(token, Lexer::TokenType::VAR) &&
			    !expect(token, Lexer::TokenType::VAL)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected var or val but not found\nHint: Primary "
				    "constructor parameters must specify 'var' or 'val'");
			}
			isVal = token->type == Lexer::TokenType::VAL;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected parameter name but not found\nHint: Provide an "
				    "identifier for parameter name after 'var'/'val'");
			}
		}
		if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
			throw ParserError(
			    token->line,
			    "Expected identifier but not found\nHint: Parameter name "
			    "must be a valid identifier");
		}
		LexerStringId baseName = token->indexData;
		const std::string &name = context.lexerString[token->indexData];
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected ':' after parameter name but not found\nHint: Add "
			    "':' after parameter name to declare parameter type");
		}
		Autolang::ClassDeclaration *classDeclaration = nullptr;
		if (!expect(token, Lexer::TokenType::COLON)) {
			if constexpr (mustHaveColon) {
				throw ParserError(
				    token->line,
				    "Expected ':' after parameter name but not found\nHint: "
				    "Parameter type annotation requires ':'");
			}
		} else {
			classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, false);
			if (!classDeclaration->isGenerics(in_data)) {
				context.allClassDeclarations.push_back(classDeclaration);
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				break;
			}
		}
		// if (classDeclaration) {
		// 	std::cerr << name << ": " <<
		// classDeclaration->getName<true>(in_data) << "\n";
		// }
		auto node = context.makeDeclarationNode(
		    in_data, token->line, baseName, name, classDeclaration, isVal,
		    false, classDeclaration ? classDeclaration->nullable : true, false,
		    false);
		parameter->parameters.push_back(node);
		if (expect(token, Lexer::TokenType::EQUAL)) {
			if constexpr (!allowDefaultValue) {
				throw ParserError(
				    token->line,
				    "Unexpected default value\nHint: Default parameter values "
				    "are not allowed in this declaration");
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected value after '=' but not found\nHint: Provide a "
				    "default value expression after '='");
			}
			if (!addedDefaultValue) {
				addedDefaultValue = true;
				parameter->defaultValuePos = parameter->parameters.size() - 1;
			}
			auto value = loadExpression(in_data, 0, i);
			parameter->parameterDefaultValues.push_back(value);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				break;
			}
		} else if (addedDefaultValue) {
			throw ParserError(
			    token->line,
			    "Parameter with default value cannot precede parameter "
			    "without default value\nHint: Place all parameters with "
			    "default values at the end of parameter list");
		}
		switch (token->type) {
			using namespace Lexer;
			case closeBracket: {
				if (!addedDefaultValue) {
					parameter->defaultValuePos = parameter->parameters.size();
				}
				return parameter;
			}
			case TokenType::COMMA: {
				break;
			}
			default: {
				throw ParserError(
				    token->line,
				    "Unexpected token '" + token->toString(context) +
				        "'\nHint: Expected ',' or closing bracket in parameter "
				        "list");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

HasClassIdNode *parsePrimary(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	HasClassIdNode *node;
	switch (token->type) {
		case Lexer::TokenType::IDENTIFIER: {
			node = loadIdentifier(in_data, i);
			if (node->kind != NodeType::CALL ||
			    (context.currentClassId &&
			     context.currentFunctionId == context.mainFunctionId)) {
				break;
			}
			auto n = static_cast<CallNode *>(node);
			if (!n->caller) {
				auto declarationNode = context.findDeclaration(
				    in_data, token->line, n->nameId, true);
				n->funcObject = declarationNode;
			}
			break;
		}
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::EXMARK:
		case Lexer::TokenType::MINUS: {
			auto op = token->type;
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    firstLine, "Expected value after '" +
				                   Lexer::Token(0, op).toString(context) + "'");
			}
			return context.unaryNodePool.push(
			    token->line,
			    op == Lexer::TokenType::EXMARK ? Lexer::TokenType::NOT : op,
			    parsePrimary(in_data, i));
		}
		case Lexer::TokenType::NUMBER: {
			node = loadNumber(in_data, i);
			break;
		}
		case Lexer::TokenType::STRING: {
			node = context.constValuePool.push(
			    firstLine, context.lexerString[token->indexData]);
			break;
		}
		case Lexer::TokenType::LT: {
			bool isGeneric = false;
			std::vector<ClassDeclaration *> inputVecs;
			loadListGenericDeclarationType(in_data, i, firstLine, false,
			                               inputVecs, isGeneric);
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(firstLine, "Expected array after <Type>");
			}
			switch (token->type) {
				case Lexer::TokenType::LBRACKET: {
					if (inputVecs.size() != 1) {
						throw ParserError(
						    token->line,
						    "'Array' expects 1 type argument but " +
						        std::to_string(inputVecs.size()) +
						        " were given");
					}
					auto classDeclaration =
					    context.classDeclarationAllocator.push();
					classDeclaration->baseClassLexerStringId = lexerIdArray;
					classDeclaration->inputClassId = std::move(inputVecs);
					// if (classDeclaration->inputClassId.size() == 0) {
					// std::cerr
					//     <<
					//     context.lexerString[classDeclaration->inputClassId[0]
					//                                ->baseClassLexerStringId]
					//     << " " << classDeclaration->inputClassId[0] << "\n";
					// }
					classDeclaration->line = firstLine;
					classDeclaration->isGeneric = isGeneric;
					if (!isGeneric) {
						context.allClassDeclarations.push_back(
						    classDeclaration);
					}
					auto list = loadListArgument<true>(in_data, i);
					node = context.createArrayPool.push(
					    firstLine, classDeclaration, std::move(list));
					break;
				}
				case Lexer::TokenType::LBRACE: {
					if (inputVecs.size() == 1) {
						auto classDeclaration =
						    context.classDeclarationAllocator.push();
						classDeclaration->baseClassLexerStringId = lexerIdSet;
						classDeclaration->inputClassId = std::move(inputVecs);
						classDeclaration->line = firstLine;
						classDeclaration->isGeneric = isGeneric;
						if (!isGeneric) {
							context.allClassDeclarations.push_back(
							    classDeclaration);
						}
						node = inferenceNodeFromLBrace(in_data, i,
						                               NodeType::CREATE_SET);
						static_cast<CreateSetNode *>(node)->classDeclaration =
						    classDeclaration;
					} else {
						if (inputVecs.size() != 2) {
							throw ParserError(
							    token->line,
							    "'Map' expects 2 type argument but " +
							        std::to_string(inputVecs.size()) +
							        " were given");
						}
						auto classDeclaration =
						    context.classDeclarationAllocator.push();
						classDeclaration->baseClassLexerStringId = lexerIdMap;
						classDeclaration->inputClassId = std::move(inputVecs);
						classDeclaration->line = firstLine;
						classDeclaration->isGeneric = isGeneric;
						if (!isGeneric) {
							context.allClassDeclarations.push_back(
							    classDeclaration);
						}
						node = inferenceNodeFromLBrace(in_data, i,
						                               NodeType::CREATE_MAP);
						static_cast<CreateMapNode *>(node)->classDeclaration =
						    classDeclaration;
					}
					break;
				}
				default: {
					throw ParserError(firstLine, "Expected array after <Type>");
				}
			}
			break;
		}
		case Lexer::TokenType::LBRACKET: {
			auto list = loadListArgument<true>(in_data, i);
			node = context.createArrayPool.push(firstLine, nullptr,
			                                    std::move(list));
			break;
		}
		case Lexer::TokenType::LBRACE: {
			node = inferenceNodeFromLBrace(in_data, i, NodeType::UNKNOW);
			break;
		}
		case Lexer::TokenType::LPAREN: {
			auto list = loadListArgument(in_data, i);
			if (list.size() != 1) {
				if (list.empty()) {
					throw ParserError(firstLine,
					                  "Expected value but empty bracket found");
				}
				throw ParserError(firstLine,
				                  "Expected value but arguments found");
			}
			node = list[0];
			break;
		}
		case Lexer::TokenType::IF: {
			node = loadIf(in_data, i, true);
			break;
		}
		case Lexer::TokenType::WHEN: {
			node = loadWhen(in_data, i, true);
			break;
		}
		default:
			throw ParserError(firstLine, "Expected value but token '" +
			                                 token->toString(context) +
			                                 "' found");
	}
	bool addOptionalNode = false;
	while (true) {
		uint32_t endLine = context.tokens[i].line;
		if (!nextToken(&token, context.tokens, i))
			goto ret;
		switch (token->type) {
			case Lexer::TokenType::LBRACKET: {
				if (token->line != endLine)
					goto ret;
				uint32_t firstLine = token->line;
				size_t tokenIndex = i;
				auto arguments = loadListArgument(in_data, i);
				bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
				node = context.callNodePool.push(
				    firstLine, tokenIndex, context.currentClassId, node,
				    lexerIdLRBRACKET, std::move(arguments),
				    context.justFindStatic, !isForceNonNull, false);
				if (isForceNonNull) {
					static_cast<CallNode *>(node)->isForceNonNull = true;
				}
				break;
			}
			case Lexer::TokenType::LPAREN: {
				if (token->line != endLine)
					goto ret;
				size_t tokenIndex = i;
				auto arguments = loadListArgument(in_data, i);
				bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
				auto callNode = context.callNodePool.push(
				    firstLine, tokenIndex, context.currentClassId, nullptr, 0,
				    std::move(arguments), context.justFindStatic,
				    !isForceNonNull, false);
				if (isForceNonNull) {
					callNode->isForceNonNull = true;
				}
				callNode->funcObject = node;
				node = callNode;
				break;
			}
			case Lexer::TokenType::EXMARK: {
				node->setNullable(false);
				if (node->isNullableNode()) {
					static_cast<NullableNode *>(node)->isForceNonNull = true;
				}
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::DOT)) {
					goto ret;
				}
			}
			case Lexer::TokenType::QMARK_DOT:
			case Lexer::TokenType::DOT: {
				bool accessNullable =
				    token->type == Lexer::TokenType::QMARK_DOT;
				if (!addOptionalNode && accessNullable)
					addOptionalNode = true;
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::IDENTIFIER)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected identifier after '.' but not found");
				}
				auto temp = loadIdentifier(in_data, i, false);
				switch (temp->kind) {
					case NodeType::VAR: {
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    token->indexData, false, temp->isNullable(),
						    accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::UNKNOW: {
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    token->indexData, false,
						    static_cast<UnknowNode *>(temp)->nullable,
						    accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::CONST_VAL: {
						throw ParserError(firstLine,
						                  "Cannot call a constant value");
					}
					default: {
						assert(temp->kind == NodeType::CALL);
						static_cast<CallNode *>(temp)->caller = node;
						static_cast<CallNode *>(temp)->accessNullable =
						    accessNullable;
						node = temp;
						break;
					}
				}
				break;
			}
			case Lexer::TokenType::PLUS_EQUAL:
			case Lexer::TokenType::MINUS_EQUAL:
			case Lexer::TokenType::STAR_EQUAL:
			case Lexer::TokenType::SLASH_EQUAL:
			case Lexer::TokenType::EQUAL: {
				Lexer::TokenType op = token->type;
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected expression after '=' but not found");
				}
				if (addOptionalNode) {
					--i;
					throw ParserError(
					    firstLine,
					    "Invalid assignment target, you must use non "
					    "null variables to assignment");
				}
				auto value = loadExpression(in_data, 0, i);
				switch (node->kind) {
					case NodeType::GET_PROP:
					case NodeType::VAR: {
						auto varNode = static_cast<AccessNode *>(node);
						if (varNode->declaration) {
							if (varNode->declaration->isVal) {
								ExprNode::deleteNode(value);
								throw ParserError(
								    token->line,
								    varNode->declaration->name +
								        " cannot be changed because it's val");
							}
						}
						return context.setValuePool.push(
						    token->line, varNode, value, context.justFindStatic,
						    op);
					}
					case NodeType::CALL:
					case NodeType::UNKNOW: {
						return context.setValuePool.push(
						    token->line, node, value, context.justFindStatic,
						    op);
					}
					default:
						break;
				}
				ExprNode::deleteNode(value);
				throw ParserError(firstLine, "Invalid assignment target");
			}
			default:
				goto ret;
		}
	}
ret:
	--i;
	if (addOptionalNode) {
		return context.optionalAccessNodePool.push(firstLine, node);
	}
	return node;
}

bool nextTokenIfMarkNonNull(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	if (nextTokenSameLine(&token, context.tokens, i, token->line) &&
	    expect(token, Lexer::TokenType::EXMARK)) {
		if (!(context.mode->flags & LibraryFlags::ALLOW_NON_NULL_ASSERTION)) {
			throw ParserError(
			    token->line,
			    "Non-null assertion operator '!' is disabled (enable "
			    "'allowNonNullAssertion' option to use it)");
		}
		return true;
	}
	--i;
	return false;
}

HasClassIdNode *loadIdentifier(in_func, size_t &i, bool allowAddThis) {
	Lexer::Token *identifier = &context.tokens[i];
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		if (!allowAddThis) {
			// std::cerr << "B " << context.lexerString[identifier->indexData]
			// << "\n";
			// Never happen because closure end with '}'
			// addThisToClosure(in_data, i);
			return context.unknowNodePool.push(
			    context.tokens[i].line, context.currentClassId,
			    context.currentFunctionId, identifier->indexData, true,
			    context.justFindStaticMember);
		}
		return findIdentifierNode(in_data, i, identifier->indexData, true);
	}
	bool nullable = true;
	switch (token->type) {
		case Lexer::TokenType::LT: {
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::IDENTIFIER)) {
				--i;
				break;
			}
			if (!nextToken(&token, context.tokens, i)) {
				i -= 2;
				break;
			}
			switch (token->type) {
				case Lexer::TokenType::LT:
				case Lexer::TokenType::GT:
				case Lexer::TokenType::COMMA:
				case Lexer::TokenType::QMARK:
				case Lexer::TokenType::LPAREN:
				case Lexer::TokenType::MINUS_GT:
				case Lexer::TokenType::AT_SIGN: {
					break;
				}
				default: {
					i -= 2;
					goto doneLT;
				}
			}
			auto firstLine = token->line;
			i -= 4;
			// std::cerr << "Creating " << context.tokens[i].toString(context)
			//           << "\n";
			auto classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, true);
			auto funcInfo = context.getCurrentFunctionInfo(in_data);
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				bool isGeneric = classDeclaration->isGenerics(in_data);
				if (!isGeneric) {
					context.allClassDeclarations.push_back(classDeclaration);
					// std::cerr << "Created unknownode: "
					//           << classDeclaration->getName(in_data) << " "
					//           << classDeclaration->isGenerics(in_data) <<
					//           "\n";
					// std::cerr << ParserContext::mode->path << ":" <<
					// token->line
					//           << "\n";
				}
				--i;
				auto name = classDeclaration->getName(in_data);

				LexerStringId newNameId =
				    context.createLexerStringIfNotExists(name);

				auto node = context.unknowNodePool.push(
				    context.tokens[i].line, context.currentClassId,
				    context.currentFunctionId, newNameId, true,
				    context.justFindStaticMember);
				if (isGeneric) {
					// std::cerr << "Created unknownode: "
					//           << classDeclaration->getName(in_data) << "\n";
					// std::cerr << ParserContext::mode->path << ":" <<
					// token->line
					//           << "\n";
					if (context.currentClassId) {
						auto classInfo = context.getCurrentClassInfo(in_data);
						classInfo->genericData
						    ->mustRenameNodes[classDeclaration] = node;
					} else if (context.preloadGenericData) {
						context.preloadGenericData
						    ->mustRenameNodes[classDeclaration] = node;
					}
				}
				return node;
			}
			// std::cerr << "Created callnode "
			//           << classDeclaration->getName(in_data) << "\n";
			size_t tokenIndex = i;
			auto arguments = loadListArgument(in_data, i);
			bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    context.createLexerStringIfNotExists(
			        classDeclaration->getName(in_data)),
			    std::move(arguments), context.justFindStatic, !isForceNonNull,
			    false);
			if (isForceNonNull) {
				callNode->isForceNonNull = true;
			}
			// if (classDeclaration->isGenerics(in_data)) {

			// Must rename in both if T in class, R in function
			bool isGeneric = classDeclaration->isGenerics(in_data);
			if (!isGeneric) {
				context.genericCallers.push_back(classDeclaration);
			} else {
				if (context.currentClassId) {
					auto classInfo = context.getCurrentClassInfo(in_data);
					if (classInfo->genericData) {
						classInfo->genericData
						    ->mustRenameNodes[classDeclaration] = callNode;
					}
				} else if (context.preloadGenericData) {
					context.preloadGenericData
					    ->mustRenameNodes[classDeclaration] = callNode;
				}
			}
			// }
			return callNode;
		}
		case Lexer::TokenType::LPAREN: {
			uint32_t firstLine = token->line;
			auto arguments = loadListArgument(in_data, i);
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LBRACE)) {
				--i;
				token = &context.tokens[i];
			} else {
				auto closureNode = loadClosure(in_data, i);
				arguments.push_back(closureNode);
			}
			switch (identifier->indexData) {
				case lexerIdInt: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Int expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::intClassId);
				}
				case lexerIdFloat: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Float expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::floatClassId);
				}
				case lexerIdBool: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Bool expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::boolClassId);
				}
				case lexerIdgetClassId: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Bool expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					break;
				}
			}

			{
				size_t tokenIndex = i;
				auto funcObject = context.findDeclaration(
				    in_data, token->line, token->indexData, false);
				if (funcObject) {
					auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
					auto callNode = context.callNodePool.push(
					    firstLine, tokenIndex, context.currentClassId, nullptr,
					    identifier->indexData, context.justFindStatic,
					    std::move(arguments), !isForceNonNull, false);
					if (isForceNonNull) {
						callNode->isForceNonNull = true;
					}
					return callNode;
				}
			}

			addThisToClosure(in_data, i);
			size_t tokenIndex = i;
			auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    identifier->indexData, std::move(arguments),
			    context.justFindStatic, !isForceNonNull, false);
			if (isForceNonNull) {
				callNode->isForceNonNull = isForceNonNull;
			}
			auto funcInfo = context.getCurrentFunctionInfo(in_data);
			if (context.currentClassId) {
				if (context.currentFunctionId != context.mainFunctionId &&
				    funcInfo->genericData) {
					auto genericDeclaration =
					    funcInfo->findGenericDeclaration(identifier->indexData);
					if (genericDeclaration) {
						genericDeclaration->allCallNodes.push_back(callNode);
					}
				} else {
					auto classInfo = context.getCurrentClassInfo(in_data);
					auto genericDeclaration = classInfo->findGenericDeclaration(
					    identifier->indexData);
					if (genericDeclaration) {
						genericDeclaration->allCallNodes.push_back(callNode);
					}
				}
			} else if (context.currentFunctionId == context.mainFunctionId &&
			           funcInfo->genericData) {
				auto genericDeclaration =
				    funcInfo->findGenericDeclaration(identifier->indexData);
				if (genericDeclaration) {
					genericDeclaration->allCallNodes.push_back(callNode);
				}
			}
			return callNode;
		}
		case Lexer::TokenType::LBRACKET: {
			auto varNode =
			    findIdentifierNode(in_data, i, identifier->indexData, true);
			uint32_t firstLine = token->line;
			size_t tokenIndex = i;
			auto arguments = loadListArgument(in_data, i);
			auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, varNode,
			    lexerIdLRBRACKET, std::move(arguments), context.justFindStatic,
			    !isForceNonNull, false);
			if (isForceNonNull) {
				callNode->isForceNonNull = isForceNonNull;
			}
			return callNode;
		}
		case Lexer::TokenType::LBRACE: {
			addThisToClosure(in_data, i);
			uint32_t firstLine = token->line;
			size_t tokenIndex = i;
			auto closureNode = loadClosure(in_data, i);
			return context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    identifier->indexData,
			    std::vector<HasClassIdNode *>{closureNode},
			    context.justFindStatic, false, false);
		}
		case Lexer::TokenType::EXMARK: {
			if (!(context.mode->flags &
			      LibraryFlags::ALLOW_NON_NULL_ASSERTION)) {
				throw ParserError(token->line,
				                  "Non-null assertion operator '!' is "
				                  "disabled (enable 'allowNonNullAssertion' "
				                  "option to use it)");
			}
			++i;
			nullable = false;
			break;
		}
		default:
			break;
	}
doneLT:;
	--i;
	token = &context.tokens[i];
	if (!allowAddThis) {
		addThisToClosure(in_data, i);
		auto node = context.unknowNodePool.push(
		    token->line, context.currentClassId, context.currentFunctionId,
		    identifier->indexData, nullable, context.justFindStaticMember);
		if (!nullable) {
			node->isForceNonNull = true;
		}
		return node;
	}
	auto node = findIdentifierNode(in_data, i, identifier->indexData, nullable);
	if (!nullable && node->isNullableNode()) {
		static_cast<NullableNode *>(node)->isForceNonNull = true;
	}
	return node;
}

bool addThisToClosure(in_func, size_t &i) {
	if (context.currentClosureNode && context.currentClassId &&
	    !context.currentClosureNode->declarationThis) {
		auto declarationThis =
		    context.classInfo[*context.currentClassId]->declarationThis;
		for (int i = context.closureScopes.size(); i-- > 0;) {
			auto closure = context.closureScopes[i];
			if (closure->declarationThis) {
				break;
			}
			closure->declarationThis = declarationThis;
			closure->scopes[0][lexerIdthis] = declarationThis;
		}
		return true;
	}
	return false;
}

HasClassIdNode *findIdentifierNode(in_func, size_t &i, LexerStringId nameId,
                                   bool nullable) {
	if (nameId == lexerIdthis) {
		if (!context.currentClassId) {
			return context.unknowNodePool.push(
			    context.tokens[i].line, std::nullopt, context.currentFunctionId,
			    nameId, nullable, context.justFindStaticMember);
		}
		addThisToClosure(in_data, i);
		if (context.currentFunctionId == context.mainFunctionId ||
		    !(context.getCurrentFunction(in_data)->functionFlags &
		      FunctionFlags::FUNC_IS_STATIC)) {
			auto declarationThis =
			    context.classInfo[*context.currentClassId]->declarationThis;
			return context.varPool.push(declarationThis->line, declarationThis,
			                            false, false);
		}
		return context.unknowNodePool.push(
		    context.tokens[i].line, context.currentClassId,
		    context.currentFunctionId, nameId, nullable,
		    context.justFindStaticMember);
	}
	auto varNode = findVarNode(in_data, i, nameId, nullable);
	if (varNode) {
		return varNode;
	}
	// std::cerr << "C " << context.lexerString[nameId] << "\n";
	addThisToClosure(in_data, i);
	auto unknowNode = context.unknowNodePool.push(
	    context.tokens[i].line, context.currentClassId,
	    context.currentFunctionId, nameId, nullable,
	    context.justFindStaticMember);
	if (context.preloadGenericData) {
		auto declaration = context.preloadGenericData->findDeclaration(nameId);
		if (declaration) {
			auto classDeclaration = context.classDeclarationAllocator.push();
			classDeclaration->line = context.tokens[i].line;
			classDeclaration->baseClassLexerStringId = nameId;
			classDeclaration->isGeneric = true;
			declaration->allClassDeclarations.push_back(classDeclaration);
			context.preloadGenericData->mustRenameNodes[classDeclaration] =
			    unknowNode;
		}
	}
	return unknowNode;
}

HasClassIdNode *findVarNode(in_func, size_t &i, LexerStringId nameId,
                            bool nullable) {
	auto constValueNode = findConstValueNode(in_data, i, nameId);
	if (constValueNode)
		return constValueNode;
	auto node =
	    context.findDeclaration(in_data, context.tokens[i].line, nameId, true);
	if (!node)
		return nullptr;
	if (static_cast<AccessNode *>(node)->nullable) // #
		static_cast<AccessNode *>(node)->nullable = nullable;
	return node;
}

ConstValueNode *findConstValueNode(in_func, size_t &i, LexerStringId nameId) {
	switch (nameId) {
		case lexerId__FILE__: {
			return context.constValuePool.push(0, context.mode->path);
		}
		case lexerId__LINE__: {
			return context.constValuePool.push(
			    0, static_cast<int64_t>(context.tokens[i].line));
		}
		case lexerId__FUNC__: {
			return context.constValuePool.push(
			    0, context.getCurrentFunction(in_data)->getName(compile));
		}
		case lexerId__CLASS__: {
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				if (classInfo->genericData) {
					return nullptr;
				}
			}
			return context.constValuePool.push(
			    0, context.currentClassId
			           ? context.getCurrentClass(in_data)->getName(compile)
			           : "");
		}
		case lexerIdtrue:
		case lexerIdfalse:
		case lexerIdnull: {
			return context.constValue[nameId];
		}
		default:
			break;
	}
	return nullptr;
}

void ensureNoKeyword(in_func, size_t &i) {
	if (!context.modifierflags)
		return;
	throw ParserError(
	    context.tokens[i].line,
	    "Command doesn't support any keyword\nHint: Remove modifiers (public, "
	    "private, static, etc.) from this command");
}

void ensureNoAnnotations(in_func, size_t &i) {
	if (!context.annotationFlags)
		return;
	throw ParserError(context.tokens[i].line,
	                  "Command doesn't support any annotations\nHint: Remove "
	                  "'@' annotations from this command");
}

Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t &i) {
	// No keywords
	if (!context.modifierflags)
		return Lexer::TokenType::PUBLIC;
	if (context.modifierflags & ModifierFlags::MF_STATIC)
		throw ParserError(
		    context.tokens[i].line,
		    "Command doesn't support 'static' keyword\nHint: Remove 'static' "
		    "keyword");
	if (context.modifierflags & ModifierFlags::MF_LATEINIT)
		throw ParserError(
		    context.tokens[i].line,
		    "Command doesn't support 'lateinit' keyword\nHint: Remove "
		    "'lateinit' keyword");
	switch (context.modifierflags) {
		case ModifierFlags::MF_PUBLIC:
			return Lexer::TokenType::PUBLIC;
		case ModifierFlags::MF_PRIVATE:
			return Lexer::TokenType::PRIVATE;
		case ModifierFlags::MF_PROTECTED:
			return Lexer::TokenType::PROTECTED;
		default:
			throw ParserError(
			    0, "Bug: Parser did not ensure exactly one modifier\nHint: Use "
			       "only "
			       "one access modifier (public, private, or protected)");
	}
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
			throw ParserError(
			    context.tokens[i].line,
			    "Semicolon ';' is not supported in Autolang\nHint: Remove ';' from your code");
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

char getOpenBracket(Lexer::TokenType type) {
	switch (type) {
		case Lexer::TokenType::LPAREN:
			return '(';
		case Lexer::TokenType::LBRACKET:
			return '[';
		case Lexer::TokenType::LBRACE:
			return '{';
		default:
			return '\0';
	}
}

bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket) {
	switch (openBracket) {
		case '(':
			return closeBracket == Lexer::TokenType::RPAREN;
		case '[':
			return closeBracket == Lexer::TokenType::RBRACKET;
		case '{':
			return closeBracket == Lexer::TokenType::RBRACE;
		default:
			return false;
	}
}

int getPrecedence(Lexer::TokenType type) {
	switch (type) {
		case Lexer::TokenType::DOT_DOT_LT:
		case Lexer::TokenType::DOT_DOT: {
			return 15;
		}
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS: {
			return 10;
		}
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::PERCENT:
		case Lexer::TokenType::SLASH:
		case Lexer::TokenType::AND:
		case Lexer::TokenType::OR: {
			return 20;
		}
		case Lexer::TokenType::QMARK_QMARK: {
			return 10;
		}
		case Lexer::TokenType::SAFE_CAST:
		case Lexer::TokenType::UNSAFE_CAST:
		case Lexer::TokenType::IS:
		case Lexer::TokenType::EQEQ:
		case Lexer::TokenType::NOTEQ:
		case Lexer::TokenType::EQEQEQ:
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::LTE:
		case Lexer::TokenType::GTE:
		case Lexer::TokenType::LT:
		case Lexer::TokenType::GT: {
			return 7;
		}
		case Lexer::TokenType::IN_: {
			return 5;
		}
		case Lexer::TokenType::OR_OR:
		case Lexer::TokenType::AND_AND: {
			return 3;
		}
		default:
			return -1;
	}
}

ConstValueNode *loadNumber(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t type = Autolang::DefaultClass::intClassId;
	const std::string &data = context.lexerString[token->indexData];
	const char *s = data.c_str();
	while (*s) {
		switch (*s) {
			case '.':
			case 'e':
			case 'E': {
				type = Autolang::DefaultClass::floatClassId;
				goto foundFlag;
			}
		}
		++s;
	}
foundFlag:
	return type == Autolang::DefaultClass::intClassId
	           ? context.constValuePool.push(
	                 token->line, static_cast<int64_t>(std::stoll(data)))
	           : context.constValuePool.push(
	                 token->line, static_cast<double>(std::stod(data)));
}

} // namespace Autolang

#endif