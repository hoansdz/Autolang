#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include "frontend/parser/Debugger.hpp"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>

namespace AutoLang {

bool build(CompiledProgram &compile, AVMReadFileMode &mode) {
	auto startCompiler = std::chrono::high_resolution_clock::now();
	ParserContext context;
	context.init(compile, mode);
	context.line = 0;
	size_t i = 0;
	Lexer::Context lexerContext;
	lexerData(in_data, mode, lexerContext);

	// for (auto& token : context.tokens) {
	// 	std::cout<<token.toString(context)<<" ";
	// }

	auto startParserTime = std::chrono::high_resolution_clock::now();
	if (!lexerContext.hasError) {
		i = 0;
		while (i < context.tokens.size()) {
			try {
				auto node = loadLine(in_data, i);
				ensureEndline(in_data, i);
				++i;
				if (node == nullptr) {
					continue;
				}
				context.getCurrentFunctionInfo(in_data)->block.nodes.push_back(
				    node);
			} catch (const std::runtime_error &err) {
				context.hasError = true;
				std::cout << "Unexpected exception: " << err.what() << '\n';
			} catch (const ParserError &err) {
				context.hasError = true;
				context.logMessage(err.line, err.message);
				if (i >= context.tokens.size())
					break;
				uint32_t line = context.tokens[i].line;
				Lexer::Token *_;
				while (nextTokenSameLine(&_, context.tokens, i, line))
					;
			}
		}
	}

	try {
		if (!context.hasError) {
			auto parserTime = std::chrono::high_resolution_clock::now();
			std::cerr << "Parser time : "
			          << std::chrono::duration_cast<std::chrono::milliseconds>(
			                 parserTime - startParserTime)
			                 .count()
			          << " ms" << '\n';

			printDebug("-----------------AST Node-----------------\n");
			resolve(in_data);
			auto resolveTime = std::chrono::high_resolution_clock::now();
			std::cerr << "Optimize and Putbytecode time : "
			          << std::chrono::duration_cast<std::chrono::milliseconds>(
			                 resolveTime - parserTime)
			                 .count()
			          << " ms" << '\n';
		}
	} catch (const std::exception &err) {
		context.hasError = true;
		std::cout << "Unexpected exception: " << err.what() << '\n';
	} catch (const ParserError &err) {
		context.hasError = true;
		context.logMessage(err.line, err.message);
	}
	freeData(in_data);
	return !context.hasError;
}

void lexerData(in_func, AVMReadFileMode &mode, Lexer::Context &lexerContext) {
	auto startLexer = std::chrono::high_resolution_clock::now();
	context.tokens = Lexer::load(&context, mode, lexerContext);
	auto lexerTime = std::chrono::high_resolution_clock::now();
	std::cerr << "Lexer file " << mode.path << " in  "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 lexerTime - startLexer)
	                 .count()
	          << " ms" << '\n';
	estimate(in_data, lexerContext);
}

void freeData(in_func) {
	for (auto &[_, funcInfo] : context.functionInfo) {
		funcInfo.block.refresh();
	}
	// size_t sizeNewClasses = context.newClasses.getSize();
	// for (size_t i = 0 ; i < sizeNewClasses; ++i) {
	// 	context.newClasses[i]->body.refresh();
	// 	context.newClasses[i]->body.nodes.clear();
	// }
	// for (size_t i = 0 ; i < context.createConstructorPool.size; ++i) {
	// 	context.createConstructorPool[i]->body.refresh();
	// }
	for (auto *node : context.staticNode) {
		ExprNode::deleteNode(node);
	}
	context.createConstructorPool.refresh();
	context.newClasses.refresh();
	context.newFunctions.refresh();
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

	context.createConstructorPool.allocate(estimateNewConstructorNode);
	context.declarationNodePool.allocate(estimateDeclaration);
	context.ifPool.allocate(lexerContext.estimate.ifNode);
	context.whilePool.allocate(lexerContext.estimate.whileNode);
	context.returnPool.allocate(lexerContext.estimate.returnNode +
	                            lexerContext.estimate.constructorNode +
	                            estimateNewClasses);
	context.setValuePool.allocate(lexerContext.estimate.setNode);
	// context.binaryNodePool.allocate(estimateBinaryNode);

	context.constValue.reserve(3 // Const
	);

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

void resolve(in_func) {
	printDebug(context.getMainFunction(in_data)->bytecodes.size());
	printDebug("Start optimize declaration nodes in functions");
	for (int i = 0; i < context.declarationNodePool.index; ++i) {
		context.declarationNodePool.objects[i].optimize(in_data);
	}
	for (auto *node : context.declarationNodePool.vecs) {
		node->optimize(in_data);
	}

	printDebug("Start optimize classes");
	size_t sizeNewClasses  = context.newClasses.getSize();
	for (int i = 0; i < sizeNewClasses; ++i) {
		context.newClasses[i]->optimize(in_data);
		context.newClasses[i]->body.optimize(in_data);
	}

	printDebug("Start optimize constructor nodes");
	for (int i = 0; i < sizeNewClasses; ++i) {
		auto *node = context.newClasses[i];
		auto clazz = &compile.classes[node->classId];
		auto classInfo = &context.classInfo[clazz->id];
		if (classInfo->primaryConstructor) {
			classInfo->primaryConstructor->optimize(in_data);
		} else {
			for (auto *constructor : classInfo->secondaryConstructor) {
				constructor->optimize(in_data);
			}
		}
	}
	printDebug("Start optimize static nodes");
	for (auto &node : context.staticNode) {
		node->optimize(in_data);
	}
	printDebug("Start optimize functions");
	size_t sizeNewFunctions = context.newFunctions.getSize();
	for (int i = 0; i < sizeNewFunctions; ++i) {
		context.newFunctions[i]->optimize(in_data);
	}
	printDebug("Start put bytecodes constructor");
	for (int i = 0; i < sizeNewClasses; ++i) {
		auto *node = context.newClasses[i];
		auto classInfo = &context.classInfo[compile.classes[node->classId].id];
		if (classInfo->primaryConstructor) {
			// Put initial bytecodes, example val a = 5 => SetNode
			//  node->body.optimize(in_data);
			auto func =
			    &compile.functions[classInfo->primaryConstructor->funcId];
			auto& bytecodes = func->bytecodes;
			node->body.putBytecodes(in_data, bytecodes);
			node->body.rewrite(in_data, bytecodes);
		} else {
			for (auto &constructor : classInfo->secondaryConstructor) {
				auto func = &compile.functions[constructor->funcId];
				// Put initial bytecodes, example val a = 5 => SetNode a and
				// value 5
				//  node->body.optimize(in_data);
				node->body.putBytecodes(in_data, func->bytecodes);
				node->body.rewrite(in_data, func->bytecodes);
				// Put constructor bytecodes
				constructor->body.optimize(in_data);
				constructor->body.putBytecodes(in_data, func->bytecodes);
				constructor->body.rewrite(in_data, func->bytecodes);
			}
		}
	}
	printDebug("Start put bytecodes static nodes");
	for (auto &node : context.staticNode) {
		node->putBytecodes(in_data,
		                   context.getMainFunction(in_data)->bytecodes);
		node->rewrite(in_data, context.getMainFunction(in_data)->bytecodes);
	}
	context.getMainFunctionInfo(in_data)->block.optimize(in_data);
	printDebug("Start put bytecodes in functions");
	for (int i = 0; i < sizeNewFunctions; ++i) {
		auto *node = context.newFunctions[i];
		auto func = &compile.functions[node->id];
		node->body.optimize(in_data);
		node->body.putBytecodes(in_data, func->bytecodes);
		node->body.rewrite(in_data, func->bytecodes);
	}
	printDebug("Start put bytecodes in main");
	context.getMainFunctionInfo(in_data)->block.putBytecodes(
	    in_data, context.getMainFunction(in_data)->bytecodes);
	context.getMainFunctionInfo(in_data)->block.rewrite(
	    in_data, context.getMainFunction(in_data)->bytecodes);

	printDebug("Real Declarations: " +
	           std::to_string(context.declarationNodePool.index +
	                          context.declarationNodePool.vecs.size()));
	printDebug("Real New classes: " +
	           std::to_string(context.newClasses.getSize()));
	printDebug("Real New functions: " +
	           std::to_string(context.newFunctions.getSize()));
	printDebug("Real ClassInfo: " + std::to_string(context.classInfo.size()));
	printDebug("Real FunctionInfo: " +
	           std::to_string(context.functionInfo.size()));

	printDebug("Real Classes: " + std::to_string(compile.classes.size()));
	printDebug("Real Functions: " + std::to_string(compile.functions.size()));
	printDebug("Real ClassMap: " + std::to_string(compile.classMap.size()));
	printDebug("Real FuncMap: " + std::to_string(compile.funcMap.size()));
	// size_t total = 0;
	// for (auto& [k, v] : compile.funcMap)
	// 	++total;

	// printDebug("TOTAL FUNC IN MAP: " + std::to_string(total));
}

ClassDeclaration loadClassDeclaration(in_func, size_t &i,
                                      uint32_t line) { // Has check
	ClassDeclaration result;
	Lexer::Token *token;
	if (!nextTokenSameLine(&token, context.tokens, i, line) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected class name but not found");
	}
	result.className = context.lexerString[token->indexData];
	if (result.className == "Null")
		throw ParserError(token->line,
		                  "Null cannot used as a type for declaration '" +
		                      result.className + "'");
	if (!nextTokenSameLine(&token, context.tokens, i, line) ||
	    !expect(token, Lexer::TokenType::QMARK)) {
		--i;
		return result;
	}
	result.nullable = true;
	return result;
}

ExprNode *loadLine(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	context.modifierflags = 0;
initial:;
	context.line = token->line;
	bool isInFunction = !context.currentClassId ||
	                    context.currentFunctionId != context.mainFunctionId;
	switch (token->type) {
	case Lexer::TokenType::LBRACE:
	case Lexer::TokenType::LPAREN:
	case Lexer::TokenType::LBRACKET:
	case Lexer::TokenType::NUMBER:
	case Lexer::TokenType::STRING:
	case Lexer::TokenType::IDENTIFIER: {
		if (!isInFunction) {
			goto err_call_func;
		}
		return parsePrimary(in_data, i);
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
			        "' only allowed inside a loop");
		return new SkipNode(token->type, token->line);
	}
	case Lexer::TokenType::IF: {
		if (!isInFunction)
			goto err_call_func;
		return loadIf(in_data, i);
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
			                  "Cannot declare function in function");
		}
		auto node = loadFunc(in_data, i);
		return nullptr;
	}
	case Lexer::TokenType::CONSTRUCTOR: {
		if (isInFunction) {
			throw ParserError(token->line,
			                  "Cannot declare constructor in function");
		}
		loadConstructor(in_data, i);
		return nullptr;
	}
	case Lexer::TokenType::CLASS: {
		if (context.currentClassId) {
			throw ParserError(token->line, "Cannot declare class in class");
		}
		auto node = loadClass(in_data, i);
		return nullptr;
	}
	case Lexer::TokenType::RETURN: {
		if (!isInFunction)
			throw ParserError(token->line,
			                  "Cannot call return outside function");
		return loadReturn(in_data, i);
	}
	case Lexer::TokenType::PUBLIC: {
		if (isInFunction)
			throw ParserError(token->line,
			                  "'public' can only be used for class members");
		if (context.modifierflags & ModifierFlags::PUBLIC)
			throw ParserError(token->line, "Duplicate modifier 'public'");
		if (context.modifierflags & ModifierFlags::PRIVATE)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'public' and 'private'");
		if (context.modifierflags & ModifierFlags::PROTECTED)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'public' and 'protected'");
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "'public' must be followed by a declaration");
		}
		context.modifierflags |= ModifierFlags::PUBLIC;
		goto initial;
	}
	case Lexer::TokenType::PRIVATE: {
		if (isInFunction)
			throw ParserError(token->line,
			                  "'private' can only be used for class members");
		if (context.modifierflags & ModifierFlags::PRIVATE)
			throw ParserError(token->line, "Duplicate modifier 'private'");
		if (context.modifierflags & ModifierFlags::PUBLIC)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'private' and 'public'");
		if (context.modifierflags & ModifierFlags::PROTECTED)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'private' and 'protected'");
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "'private' must be followed by a declaration");
		}
		context.modifierflags |= ModifierFlags::PRIVATE;
		goto initial;
	}
	case Lexer::TokenType::PROTECTED: {
		if (isInFunction)
			throw ParserError(token->line,
			                  "'protected' can only be used for class members");
		if (context.modifierflags & ModifierFlags::PROTECTED)
			throw ParserError(token->line, "Duplicate modifier 'protected'");
		if (context.modifierflags & ModifierFlags::PUBLIC)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'protected' and 'public'");
		if (context.modifierflags & ModifierFlags::PRIVATE)
			throw ParserError(
			    token->line,
			    "Invalid modifier combination: 'protected' and 'private'");
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "'protected' must be followed by a declaration");
		}
		context.modifierflags |= ModifierFlags::PROTECTED;
		goto initial;
	}
	case Lexer::TokenType::STATIC: {
		if (context.modifierflags & ModifierFlags::STATIC)
			throw ParserError(token->line, "Duplicate modifier 'static'");
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "'static' must be followed by a declaration");
		}
		context.modifierflags |= ModifierFlags::STATIC;
		goto initial;
	}
	default:
		throw ParserError(token->line,
		                  "C1 Unexpected token " + token->toString(context));
	}
	++i;
	return nullptr;
err_call_func:;
	printDebug(context.currentClassId
	               ? compile.classes[*context.currentClassId].name
	               : "None");
	throw ParserError(token->line, "Cannot call command outside function ");
err_call_class:;
	throw ParserError(token->line, "Cannot call command outside class ");
}

void loadBody(in_func, std::vector<ExprNode *> &nodes, size_t &i,
              bool createScope) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (createScope)
		context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
	if (token->type != Lexer::TokenType::LBRACE) {
		nodes.push_back(loadLine(in_data, i));
		if (createScope)
			context.getCurrentFunctionInfo(in_data)->popBackScope();
		return;
	}
	while (nextToken(&token, context.tokens, i)) {
		if (token->type == Lexer::TokenType::RBRACE) {
			if (createScope)
				context.getCurrentFunctionInfo(in_data)->popBackScope();
			return;
		}
		try {
			auto node = loadLine(in_data, i);
			ensureEndline(in_data, i);
			if (node == nullptr)
				continue;
			nodes.push_back(node);
		} catch (const std::runtime_error &err) {
			throw err;
		} catch (const ParserError &err) {
			context.hasError = true;
			context.logMessage(err.line, err.message);
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
						return;
					}
					break;
				}
				}
			}
		}
	}
	throw ParserError(firstLine, "Expected } but not found");
}

HasClassIdNode *loadExpression(in_func, int minPrecedence, size_t &i) {
	std::unique_ptr<HasClassIdNode> left(parsePrimary(in_data, i));
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
		case Lexer::TokenType::COMMA:
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET:
		case Lexer::TokenType::RBRACE: {
			--i;
			return left.release();
		}
		default:
			break;
		};
		int precedence = getPrecedence(token->type);
		if (precedence == -1 || precedence < minPrecedence)
			break;
		Lexer::TokenType op = token->type;
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected expression after operator but not found");
		}
		HasClassIdNode *right = loadExpression(in_data, precedence + 1, i);
		if (op == Lexer::TokenType::QMARK_QMARK) {
			left.reset(new NullCoalescingNode(firstLine, left.release(), right));
			continue;
		}
		// auto binaryNode = context.binaryNodePool.push(op, left, right);
		auto binaryNode =
		    std::make_unique<BinaryNode>(firstLine, op, left.release(), right);
		if (minPrecedence == 0) {
			auto node = binaryNode->calculate(in_data);
			if (node != nullptr) {
				// binaryNode->left = nullptr;
				// binaryNode->right = nullptr;
				left.reset(node);
				continue;
			}
		}
		left.reset(binaryNode.release());
		// std::cout<<"op "<<binaryNode<<":"<<Lexer::Token(0, binaryNode->op,
		// "").toString()<<'\n';
	}
	--i;
	return left.release();
}

std::vector<HasClassIdNode *> loadListArgument(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	char openBracket = getOpenBracket(token->type);
	if (openBracket == '\0')
		throw ParserError(token->line,
		                  "Unexpected token " + token->toString(context));
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(0, "Bug: Lexer not ensure close bracket");
	}
	token = &context.tokens[i];
	std::vector<HasClassIdNode *> nodes;
	switch (token->type) {
	case Lexer::TokenType::RPAREN:
	case Lexer::TokenType::RBRACKET:
	case Lexer::TokenType::RBRACE: {
		if (!isCloseBracket(openBracket, token->type)) {
			for (auto *node : nodes)
				ExprNode::deleteNode(node);
			throw ParserError(token->line,
			                  "Bug: Lexer not ensure close bracket");
		}
		return nodes;
	}
	default:
		nodes.push_back(loadExpression(in_data, 0, i));
		break;
	}
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET:
		case Lexer::TokenType::RBRACE: {
			if (!isCloseBracket(openBracket, token->type)) {
				for (auto *node : nodes)
					ExprNode::deleteNode(node);
				throw ParserError(token->line,
				                  "Bug: Lexer not ensure close bracket");
			}
			return nodes;
		}
		case TokenType::COMMA: {
			if (!nextToken(&token, context.tokens, i))
				goto expectedCloseBracket;
			nodes.push_back(loadExpression(in_data, 0, i));
			break;
		}
		default:
			goto expectedCloseBracket;
		}
	}
expectedCloseBracket:
	for (auto *node : nodes)
		ExprNode::deleteNode(node);
	throw ParserError(token->line, "Bug: Lexer not ensure close bracket");
}

std::vector<DeclarationNode *> loadListDeclaration(in_func, size_t &i,
                                                   bool allowVar) {
	Lexer::Token *token = &context.tokens[i];
	std::vector<DeclarationNode *> nodes;
	if (!nextToken(&token, context.tokens, i))
		goto expectedCloseBracket;
	switch (token->type) {
	case Lexer::TokenType::RPAREN: {
		return nodes;
	}
	case Lexer::TokenType::VAR:
	case Lexer::TokenType::VAL: {
		if (!allowVar)
			throw ParserError(token->line, token->toString(context) +
			                                   " can't be allowed here");
	}
	case Lexer::TokenType::IDENTIFIER: {
		--i;
		break;
	}
	default:
		throw ParserError(token->line, "Expected name but not found");
	}
	while (nextToken(&token, context.tokens, i)) {
		bool isVal = true;
		if (allowVar) {
			if (!expect(token, Lexer::TokenType::VAR) &&
			    !expect(token, Lexer::TokenType::VAL)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Expected var or val but not found");
			}
			isVal = token->type == Lexer::TokenType::VAL;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Expected name but not found");
			}
		}
		if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
			throw ParserError(token->line, "Expected identifier but not found");
		}
		std::string &name = context.lexerString[token->indexData];
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::COLON)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected ':' but not found");
		}
		auto classDeclaration = loadClassDeclaration(in_data, i, token->line);
		if (!nextToken(&token, context.tokens, i))
			break;
		auto node = context.makeDeclarationNode(
		    in_data, token->line, false, name,
		    std::move(classDeclaration.className), isVal, false,
		    classDeclaration.nullable, false);
		nodes.push_back(node);
		switch (token->type) {
			using namespace Lexer;
		case Lexer::TokenType::RPAREN: {
			return nodes;
		}
		case TokenType::COMMA: {
			break;
		}
		default:
			goto expectedCloseBracket;
		}
	}
expectedCloseBracket:
	--i;
	throw ParserError(0, "Bug: Lexer not ensure close bracket");
}

HasClassIdNode *parsePrimary(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	std::unique_ptr<HasClassIdNode> node;
	switch (token->type) {
	case Lexer::TokenType::IDENTIFIER:
		node.reset(loadIdentifier(in_data, i));
		break;
	case Lexer::TokenType::PLUS:
	case Lexer::TokenType::EXMARK:
	case Lexer::TokenType::MINUS: {
		auto op = token->type;
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(firstLine,
			                  "Expected value after '" +
			                      Lexer::Token(0, op).toString(context) + "'");
		}
		auto unaryNode = new UnaryNode(
		    token->line,
		    op == Lexer::TokenType::EXMARK ? Lexer::TokenType::NOT : op,
		    parsePrimary(in_data, i));
		auto calculated = unaryNode->calculate(in_data);
		if (calculated == nullptr)
			return unaryNode;
		unaryNode->value = nullptr;
		ExprNode::deleteNode(unaryNode);
		return calculated;
	}
	case Lexer::TokenType::NUMBER: {
		node.reset(loadNumber(in_data, i));
		break;
	}
	case Lexer::TokenType::STRING: {
		node.reset(new ConstValueNode(firstLine,
		                              context.lexerString[token->indexData]));
		break;
	}
	case Lexer::TokenType::LPAREN:
	case Lexer::TokenType::LBRACKET:
	case Lexer::TokenType::LBRACE: {
		auto list = loadListArgument(in_data, i);
		if (list.size() != 1) {
			for (auto *i : list)
				ExprNode::deleteNode(i);
			if (list.size() == 0)
				throw ParserError(firstLine,
				                  "Expected value but empty bracket found");
			throw ParserError(firstLine, "Expected value but arguments found");
		}
		node.reset(list[0]);
		break;
	}
	default:
		throw ParserError(firstLine, "Expected value but token '" +
		                                 token->toString(context) + "' found");
	}
	bool addOptionalNode = false;
	while (true) {
		if (!nextToken(&token, context.tokens, i))
			return node.release();
		switch (token->type) {
		case Lexer::TokenType::QMARK_DOT:
		case Lexer::TokenType::DOT: {
			bool accessNullable = token->type == Lexer::TokenType::QMARK_DOT;
			if (accessNullable)
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
				node.reset(new GetPropNode(
				    token->line, nullptr, context.currentClassId,
				    node.release(), context.lexerString[token->indexData],
				    false, temp->isNullable(), accessNullable));
				ExprNode::deleteNode(temp);
				break;
			}
			case NodeType::UNKNOW: {
				node.reset(new GetPropNode(
				    token->line, nullptr, context.currentClassId,
				    node.release(), context.lexerString[token->indexData],
				    false, static_cast<UnknowNode *>(temp)->nullable,
				    accessNullable));
				ExprNode::deleteNode(temp);
				break;
			}
			case NodeType::CONST: {
				throw ParserError(firstLine, "Cannot call const value");
			}
			default: {
				assert(temp->kind == NodeType::CALL);
				static_cast<CallNode *>(temp)->caller = node.release();
				static_cast<CallNode *>(temp)->accessNullable = accessNullable;
				node.reset(temp);
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
				throw ParserError(firstLine,
				                  "Invalid assignment target, you must use non "
				                  "null varaibles to assignment");
			}
			auto value = loadExpression(in_data, 0, i);
			switch (node->kind) {
			case NodeType::VAR: {
				auto varNode = static_cast<VarNode *>(node.release());
				if (varNode->declaration->isVal) {
					ExprNode::deleteNode(value);
					throw ParserError(token->line,
					                  varNode->declaration->name +
					                      " cannot be changed because val");
				}
				return context.setValuePool.push(token->line, varNode, value,
				                                 op);
			}
			case NodeType::UNKNOW: {
				return context.setValuePool.push(token->line, node.release(),
				                                 value, op);
			}
			case NodeType::GET_PROP: {
				return context.setValuePool.push(token->line, node.release(),
				                                 value, op);
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
		return new OptionalAccessNode(firstLine, node.release());
	}
	return node.release();
}

bool nextTokenIfMarkNonNull(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	if (nextTokenSameLine(&token, context.tokens, i, token->line) &&
	    expect(token, Lexer::TokenType::EXMARK)) {
		return true;
	}
	--i;
	return false;
}

HasClassIdNode *loadIdentifier(in_func, size_t &i, bool allowAddThis) {
	Lexer::Token *identifier = &context.tokens[i];
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		if (!allowAddThis) {
			return new UnknowNode(token->line, context.currentClassId,
			                      context.lexerString[identifier->indexData],
			                      true);
		}
		return findIdentifierNode(
		    in_data, i, context.lexerString[identifier->indexData], true);
	}
	bool nullable = true;
	switch (token->type) {
	case Lexer::TokenType::LPAREN: {
		uint32_t firstLine = token->line;
		auto arguments = loadListArgument(in_data, i);
		return new CallNode(firstLine, context.currentClassId, nullptr,
		                    context.lexerString[identifier->indexData] + "()",
		                    std::move(arguments), context.justFindStatic,
		                    !nextTokenIfMarkNonNull(in_data, i), false);
	}
	case Lexer::TokenType::LBRACKET: {
		auto varNode = findVarNode(
		    in_data, i, context.lexerString[identifier->indexData], true);
		if (varNode->kind != AutoLang::NodeType::VAR) {
			ExprNode::deleteNode(varNode);
			throw ParserError(token->line, "Invalid assignment target");
		}
		uint32_t firstLine = token->line;
		auto arguments = loadListArgument(in_data, i);
		return new CallNode(firstLine, context.currentClassId,
		                    static_cast<AccessNode *>(varNode), "[]", arguments,
		                    context.justFindStatic,
		                    !nextTokenIfMarkNonNull(in_data, i), false);
	}
	case Lexer::TokenType::LBRACE: {
		break;
	}
	case Lexer::TokenType::EXMARK: {
		++i;
		nullable = false;
		break;
	}
	default:
		break;
	}
	--i;
	if (!allowAddThis) {
		return new UnknowNode(token->line, context.currentClassId,
		                      context.lexerString[identifier->indexData],
		                      nullable);
	}
	return findIdentifierNode(
	    in_data, i, context.lexerString[identifier->indexData], nullable);
}

HasClassIdNode *findIdentifierNode(in_func, size_t &i, std::string &name,
                                   bool nullable) {
	auto varNode = findVarNode(in_data, i, name, nullable);
	return varNode ? varNode
	               : new UnknowNode(context.tokens[i].line,
	                                context.currentClassId, name, nullable);
}

HasClassIdNode *findVarNode(in_func, size_t &i, std::string &name,
                            bool nullable) {
	auto constValueNode = findConstValueNode(in_data, i, name);
	if (constValueNode != nullptr)
		return constValueNode;
	auto node =
	    context.findDeclaration(in_data, context.tokens[i].line, name, true);
	if (!node)
		return nullptr;
	if (static_cast<AccessNode *>(node)->nullable) // #
		static_cast<AccessNode *>(node)->nullable = nullable;
	return node;
}

ConstValueNode *findConstValueNode(in_func, size_t &i, std::string &name) {
	auto it = context.constValue.find(name);
	if (it == context.constValue.end()) {
		return nullptr;
	}
	return new ConstValueNode(context.tokens[i].line, it->second.first,
	                          it->second.second);
}

void ensureNoKeyword(in_func, size_t &i) {
	if (!context.modifierflags)
		return;
	throw ParserError(context.tokens[i].line,
	                  "Command doesn't support any keyword");
}

Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t &i) {
	// No keywords
	if (!context.modifierflags)
		return Lexer::TokenType::PUBLIC;
	if (context.modifierflags & ModifierFlags::STATIC)
		throw ParserError(context.tokens[i].line,
		                  "Command doesn't support 'static' keyword");
	switch (context.modifierflags) {
	case ModifierFlags::PUBLIC:
		return Lexer::TokenType::PUBLIC;
	case ModifierFlags::PRIVATE:
		return Lexer::TokenType::PRIVATE;
	case ModifierFlags::PROTECTED:
		return Lexer::TokenType::PROTECTED;
	default:
		throw ParserError(0, "Bug: Parser not ensure one modifier");
	}
}

void ensureEndline(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	if (nextTokenSameLine(&token, context.tokens, i, token->line)) {
		std::string line = token->toString(context);
		while (nextTokenSameLine(&token, context.tokens, i, token->line)) {
			if (token->type == Lexer::TokenType::RBRACE) {
				--i;
				return;
			}
			line += " " + token->toString(context);
			break;
		}
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Command not allowed here because cannot call multi "
		                  "command in a line: " +
		                      line);
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
	case Lexer::TokenType::AND_AND: {
		return 3;
	}
	case Lexer::TokenType::OR_OR: {
		return 2;
	}
	default:
		return -1;
	}
}

ConstValueNode *loadNumber(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t type = AutoLang::DefaultClass::intClassId;
	std::string &data = context.lexerString[token->indexData];
	const char *s = data.c_str();
	while (*s) {
		switch (*s) {
		case '.':
		case 'e':
		case 'E': {
			type = AutoLang::DefaultClass::floatClassId;
			goto foundFlag;
		}
		}
		++s;
	}
foundFlag:
	return type == AutoLang::DefaultClass::intClassId
	           ? new ConstValueNode(token->line,
	                                static_cast<int64_t>(std::stoll(data)))
	           : new ConstValueNode(token->line,
	                                static_cast<double>(std::stod(data)));
}

} // namespace AutoLang

#endif