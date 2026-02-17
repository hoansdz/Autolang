#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/Import.hpp"
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>

namespace AutoLang {

void lexerData(in_func, ACompiler &compiler, LibraryData *library) {
	// auto startLexer = std::chrono::high_resolution_clock::now();
	Lexer::load(&context, library);
	// auto lexerTime = std::chrono::high_resolution_clock::now();
	// auto total = std::chrono::duration_cast<std::chrono::milliseconds>(
	//                  lexerTime - startLexer)
	//                  .count();
	// std::cerr << "Lexer file " << library->path << " in  " << total << "
	// ms\n";
}

LibraryData *loadImport(in_func, std::vector<Lexer::Token> &tokens,
                        ACompiler &compiler, size_t i) {
	Lexer::Token *token = &tokens[i];
	// std::cerr<<i<<" & "<<tokens.size() << "\n";
	Lexer::Token *importToken = token;
	uint32_t firstLine = token->line;
	assert(token->type == Lexer::TokenType::IMPORT);
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "@import expected string value (");
	}
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::STRING)) {
		throw ParserError(firstLine, "@import expected string value (String");
	}
	std::string &path = context.lexerString[token->indexData];
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(firstLine, "@import expects a constant "
		                             "string value, not an expression");
	}
	if (path.empty()) {
		throw ParserError(firstLine, "import path is empty");
	}
	// {
	// 	auto it = context.importMap.find(path);
	// 	if (it != context.importMap.end()) {
	// 		return it->second;
	// 	}
	// }
	LibraryData *library = compiler.requestImport(path.c_str());
	if (!library) {
		throw ParserError(firstLine, "Cannot find library '" + path + "'");
	}
	// context.importMap[path] = library;
	if (!library->lexerContext.tokens.empty()) {
		return library;
	}
	compiler.loadSource(library);
	library->rawData.clear();
	return library;
}

void freeData(in_func) {
	for (auto *funcInfo : context.functionInfo) {
		funcInfo->block.refresh();
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
	context.staticNode.clear();
	context.createConstructorPool.destroy();
	context.newClasses.refresh();
	context.newFunctions.refresh();
	// context.binaryNodePool.refresh();
}

ClassId loadGenerics(in_func, std::string &name,
                     ClassDeclaration *classDeclaration) {
	auto it =
	    context.defaultClassMap.find(classDeclaration->baseClassLexerStringId);
	if (it == context.defaultClassMap.end()) {
		throw ParserError(
		    classDeclaration->line,
		    "Bug: Cannot find class " +
		        context.lexerString[classDeclaration->baseClassLexerStringId]);
	}
	ClassId classId = it->second;
	auto clazz = compile.classes[it->second];
	auto classInfo = context.classInfo[it->second];
	if (compile.classMap.find(name) != compile.classMap.end()) {
		return classId;
	}
	auto baseCreateClassNode = context.findCreateClassNode(clazz->id);
	LexerStringId newNameId;
	{
		auto it = context.lexerStringMap.find(name);
		if (it == context.lexerStringMap.end()) {
			newNameId = context.lexerString.size();
			context.lexerStringMap[name] = newNameId;
			context.lexerString.push_back(name);
		} else {
			newNameId = it->second;
		}
	}

	auto newCreateClassNode = context.newClasses.push(
	    baseCreateClassNode->line, newNameId, baseCreateClassNode->classFlags);
	newCreateClassNode->pushClass(in_data);
	newCreateClassNode->superDeclaration =
	    baseCreateClassNode->superDeclaration;
	auto newClassId = newCreateClassNode->classId;
	context.defaultClassMap[newNameId] = newClassId;

	for (size_t i = 0; i < classInfo->genericDeclarations.size(); ++i) {
		auto &genericDeclaration = classInfo->genericDeclarations[i];
		auto &inputClassId = classDeclaration->inputClassId[i];

		// Change callnode name
		if (!genericDeclaration->allCallNodes.empty()) {
			std::string name =
			    compile.classes[*inputClassId->classId]->name + "()";
			for (auto *callNode : genericDeclaration->allCallNodes) {
				callNode->name = name;
			}
		}
		// Change generics type
		genericDeclaration->classId = *inputClassId->classId;
		genericDeclaration->nullable = inputClassId->nullable;

		for (auto *classDeclaration :
		     genericDeclaration->allClassDeclarations) {
			classDeclaration->classId = *inputClassId->classId;
			if (classDeclaration->mustInference) {
				classDeclaration->nullable = inputClassId->nullable;
				classDeclaration->mustInference = false;
			}
			classDeclaration->baseClassLexerStringId =
			    inputClassId->baseClassLexerStringId;
		}
	}

	auto newClass = compile.classes[newClassId];
	newClass->memberMap = clazz->memberMap;
	newClass->memberId = clazz->memberId;
	auto newClassInfo = context.classInfo[newClassId];
	newClassInfo->declarationThis = context.declarationNodePool.push(
	    classInfo->declarationThis->line, newClassId, "this", nullptr, true,
	    false, false);
	newClassInfo->declarationThis->id = 0;
	newClassInfo->declarationThis->classId = newClassId;
	auto lastCurrentClassId = context.currentClassId;
	context.currentClassId = newClassId;

	for (auto *member : classInfo->member) {
		newClassInfo->member.push_back(
		    static_cast<DeclarationNode *>(member->copy(in_data)));
		// if (member->classDeclaration) {
		// 	std::cerr
		// 	    << compile.classes[*member->classDeclaration->classId]->name
		// 	    << "\n";
		// } else
		// 	std::cerr << "No" << "\n";
	}

	newClassInfo->declarationThis->classId = newClassId;
	// newClassInfo->func = classInfo->func;
	newClassInfo->staticMember = classInfo->staticMember;
	newClassInfo->parent = classInfo->parent;
	newClassInfo->staticFunc = classInfo->staticFunc;

	// newClass->funcMap = clazz->funcMap;

	for (auto &[classDeclaration, node] : classInfo->mustRenameNodes) {
		switch (node->kind) {
			case NodeType::UNKNOW: {
				auto unknowNode = static_cast<UnknowNode *>(node);
				auto it = context.lexerStringMap.find(
				    classDeclaration->getName(in_data));
				if (it == context.lexerStringMap.end()) {
					throw ParserError(classDeclaration->line,
					                  "Unsolved " +
					                      classDeclaration->getName(in_data));
				}
				unknowNode->nameId = it->second;
				break;
			}
			case NodeType::CALL: {
				auto callNode = static_cast<CallNode *>(node);
				auto it = context.lexerStringMap.find(
				    classDeclaration->getName(in_data));
				if (it == context.lexerStringMap.end()) {
					throw ParserError(classDeclaration->line,
					                  "Unsolved " +
					                      classDeclaration->getName(in_data));
				}
				callNode->name = context.lexerString[it->second];
			}
		}
	}

	ParserContext::mode = baseCreateClassNode->mode;
	newCreateClassNode->body.nodes.reserve(
	    baseCreateClassNode->body.nodes.size());
	for (auto *node : baseCreateClassNode->body.nodes) {
		newCreateClassNode->body.nodes.push_back(node->copy(in_data));
	}

	std::string newName = context.lexerString[newNameId] + "()";
	if (classInfo->primaryConstructor) {
		std::vector<DeclarationNode *> arguments;
		arguments.reserve(classInfo->primaryConstructor->arguments.size());
		for (auto argument : classInfo->primaryConstructor->arguments) {
			arguments.push_back(
			    static_cast<DeclarationNode *>(argument->copy(in_data)));
		}
		auto constructor = context.createConstructorPool.push(
		    classInfo->primaryConstructor->line, newClassId, newName, arguments,
		    true, classInfo->primaryConstructor->functionFlags);
		newClassInfo->primaryConstructor = constructor;
		newClassInfo->primaryConstructor->pushFunction(in_data);
	} else {
		newClassInfo->secondaryConstructor.reserve(
		    classInfo->secondaryConstructor.size());
		for (auto *constructor : classInfo->secondaryConstructor) {
			std::vector<DeclarationNode *> arguments;
			arguments.reserve(constructor->arguments.size());
			for (auto argument : constructor->arguments) {
				arguments.push_back(
				    static_cast<DeclarationNode *>(argument->copy(in_data)));
			}
			auto newConstructor = context.createConstructorPool.push(
			    constructor->line, newClassId, newName, arguments, false,
			    constructor->functionFlags);
			newClassInfo->secondaryConstructor.push_back(newConstructor);
			newConstructor->pushFunction(in_data);
			// ParserContext::mode = constructor->mode;
			auto lastCurrentFunctionId = context.currentFunctionId;
			context.currentFunctionId = newConstructor->funcId;
			newConstructor->body.nodes.reserve(constructor->body.nodes.size());
			for (auto *node : constructor->body.nodes) {
				newConstructor->body.nodes.push_back(node->copy(in_data));
			}
			context.currentFunctionId = lastCurrentFunctionId;
		}
	}

	for (auto *createFuncNode : classInfo->createFunctionNodes) {
		std::vector<DeclarationNode *> arguments;
		arguments.reserve(createFuncNode->arguments.size());
		for (auto argument : createFuncNode->arguments) {
			arguments.push_back(
			    static_cast<DeclarationNode *>(argument->copy(in_data)));
		}
		auto newCreateFuncNode = context.newFunctions.push(
		    createFuncNode->line, newClassId, createFuncNode->name, nullptr,
		    arguments, createFuncNode->functionFlags);
		if (createFuncNode->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			newCreateFuncNode->pushNativeFunction(
			    in_data, compile.functions[createFuncNode->id]->native);
		} else {
			newCreateFuncNode->pushFunction(in_data);
		}
		if (createFuncNode->classDeclaration) {
			compile.functions[newCreateFuncNode->id]->returnId =
			    *createFuncNode->classDeclaration->classId;
		}
		auto lastCurrentFunctionId = context.currentFunctionId;
		context.currentFunctionId = newCreateFuncNode->id;
		// ParserContext::mode = createFuncNode->mode; //Loaded in new class
		newCreateFuncNode->body.nodes.reserve(
		    createFuncNode->body.nodes.size());
		for (auto *node : createFuncNode->body.nodes) {
			newCreateFuncNode->body.nodes.push_back(node->copy(in_data));
		}
		context.currentFunctionId = lastCurrentFunctionId;
	}

	context.currentClassId = lastCurrentClassId;

	for (auto *declarationNode : classInfo->allDeclarationNode) {
		if (declarationNode->classDeclaration) {
			if (!declarationNode->classDeclaration->classId) {
				declarationNode->classDeclaration->load<false>(in_data);
				if (!declarationNode->classDeclaration->classId) {
					throw ParserError(
					    declarationNode->classDeclaration->line,
					    "Bug: Cannot find class name " +
					        declarationNode->classDeclaration->getName(
					            in_data));
				}
				declarationNode->optimize(in_data);
				declarationNode->classDeclaration->classId = std::nullopt;
				// declarationNode->classDeclaration = nullptr;
				continue;
			}
		}
		declarationNode->optimize(in_data);
		// declarationNode->classDeclaration = nullptr;
	}

	// std::cerr << "Created " << newClass->name << "\n";
	return newClassId;
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

ExprNode *loadLine(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	context.modifierflags = 0;
	context.annotationFlags = 0;
initial:;
	bool isInFunction = !context.currentClassId ||
	                    context.currentFunctionId != context.mainFunctionId;
	switch (token->type) {
		case Lexer::TokenType::END_IMPORT: {
			context.loadingLibs.pop_back();
			ParserContext::mode = context.loadingLibs.back();
			nextToken(&token, context.tokens, i);
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
			if (!node)
				throw ParserError(0, "Bug return");
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				classInfo->createFunctionNodes.push_back(
				    static_cast<CreateFuncNode *>(node));
			}
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
		case Lexer::TokenType::AT_SIGN: {
			if (context.currentFunctionId != context.mainFunctionId)
				throw ParserError(token->line,
				                  "Annotation can only be used for functions");
			loadAnnotations(in_data, i);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Annotation must be followed by function");
			}
			goto initial;
		}
		case Lexer::TokenType::PUBLIC: {
			if (isInFunction)
				throw ParserError(
				    token->line, "'public' can only be used for class members");
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(token->line, "Duplicate modifier 'public'");
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'public' and 'private'");
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'public' and 'protected'");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "'public' must be followed by a declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PUBLIC;
			goto initial;
		}
		case Lexer::TokenType::PRIVATE: {
			if (isInFunction)
				throw ParserError(
				    token->line,
				    "'private' can only be used for class members");
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(token->line, "Duplicate modifier 'private'");
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'private' and 'public'");
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'private' and 'protected'");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "'private' must be followed by a declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PRIVATE;
			goto initial;
		}
		case Lexer::TokenType::PROTECTED: {
			if (isInFunction)
				throw ParserError(
				    token->line,
				    "'protected' can only be used for class members");
			if (context.modifierflags & ModifierFlags::MF_PROTECTED)
				throw ParserError(token->line,
				                  "Duplicate modifier 'protected'");
			if (context.modifierflags & ModifierFlags::MF_PUBLIC)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'protected' and 'public'");
			if (context.modifierflags & ModifierFlags::MF_PRIVATE)
				throw ParserError(
				    token->line,
				    "Invalid modifier combination: 'protected' and 'private'");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "'protected' must be followed by a declaration");
			}
			context.modifierflags |= ModifierFlags::MF_PROTECTED;
			goto initial;
		}
		case Lexer::TokenType::STATIC: {
			if (context.modifierflags & ModifierFlags::MF_STATIC)
				throw ParserError(token->line, "Duplicate modifier 'static'");
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "'static' must be followed by a declaration");
			}
			context.modifierflags |= ModifierFlags::MF_STATIC;
			goto initial;
		}
		default:
			throw ParserError(token->line, "C1 Unexpected token " +
			                                   token->toString(context));
	}
	++i;
	return nullptr;
err_call_func:;
	printDebug(context.currentClassId
	               ? compile.classes[*context.currentClassId]->name
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
			if (context.annotationFlags || context.modifierflags) {
				ExprNode::deleteNode(node);
				ensureNoAnnotations(in_data, i);
				ensureNoKeyword(in_data, i);
			}
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
	HasClassIdNode *left = parsePrimary(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
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
		Lexer::TokenType op = token->type;
		if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected expression after operator but not found");
		}
		HasClassIdNode *right = loadExpression(in_data, precedence + 1, i);
		if (op == Lexer::TokenType::QMARK_QMARK) {
			left = context.nullCoalescingPool.push(firstLine, left, right);
			continue;
		}
		// auto binaryNode = context.binaryNodePool.push(op, left, right);
		left = context.binaryNodePool.push(firstLine, op, left, right);
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
		// std::cout<<"op "<<binaryNode<<":"<<Lexer::Token(0,
		// binaryNode->op,
		// "").toString()<<'\n';
	}
	--i;
	return left;
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
		auto classDeclaration =
		    loadClassDeclaration(in_data, i, token->line, false);
		context.allClassDeclarations.push_back(classDeclaration);
		if (!nextToken(&token, context.tokens, i))
			break;
		auto node = context.makeDeclarationNode(
		    in_data, token->line, false, name, classDeclaration, isVal, false,
		    classDeclaration->nullable, false);
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
	HasClassIdNode *node;
	switch (token->type) {
		case Lexer::TokenType::IDENTIFIER:
			node = loadIdentifier(in_data, i);
			break;
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
				throw ParserError(firstLine,
				                  "Expected value but arguments found");
			}
			node = list[0];
			break;
		}
		default:
			throw ParserError(firstLine, "Expected value but token '" +
			                                 token->toString(context) +
			                                 "' found");
	}
	bool addOptionalNode = false;
	while (true) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			return node;
		}
		switch (token->type) {
			case Lexer::TokenType::QMARK_DOT:
			case Lexer::TokenType::DOT: {
				bool accessNullable =
				    token->type == Lexer::TokenType::QMARK_DOT;
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
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    context.lexerString[token->indexData], false,
						    temp->isNullable(), accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::UNKNOW: {
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    context.lexerString[token->indexData], false,
						    static_cast<UnknowNode *>(temp)->nullable,
						    accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::CONST: {
						throw ParserError(firstLine, "Cannot call const value");
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
					    "null varaibles to assignment");
				}
				auto value = loadExpression(in_data, 0, i);
				switch (node->kind) {
					case NodeType::VAR: {
						auto varNode = static_cast<VarNode *>(node);
						if (varNode->declaration->isVal) {
							ExprNode::deleteNode(value);
							throw ParserError(
							    token->line,
							    varNode->declaration->name +
							        " cannot be changed because val");
						}
						return context.setValuePool.push(token->line, varNode,
						                                 value, op);
					}
					case NodeType::UNKNOW: {
						return context.setValuePool.push(token->line, node,
						                                 value, op);
					}
					case NodeType::GET_PROP: {
						return context.setValuePool.push(token->line, node,
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
		return context.optionalAccessNodePool.push(firstLine, node);
	}
	return node;
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
			return context.unknowNodePool.push(token->line,
			                                   context.currentClassId,
			                                   identifier->indexData, true);
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
			if (!nextToken(&token, context.tokens, i) ||
			    (token->type != Lexer::TokenType::GT &&
			     token->type != Lexer::TokenType::COMMA)) {
				i -= 2;
				break;
			}
			auto firstLine = token->line;
			i -= 4;
			auto classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, true);
			context.allClassDeclarations.push_back(classDeclaration);
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				--i;
				auto name = classDeclaration->getName(in_data);
				std::cerr << "Created unknownode: " << name << "\n";
				LexerStringId newNameId;
				{
					auto it = context.lexerStringMap.find(name);
					if (it == context.lexerStringMap.end()) {
						newNameId = context.lexerString.size();
						context.lexerStringMap[name] = newNameId;
						context.lexerString.push_back(name);
					} else {
						newNameId = it->second;
					}
				}
				auto node = context.unknowNodePool.push(context.tokens[i].line,
				                                        context.currentClassId,
				                                        newNameId, true);
				if (context.currentClassId) {
					auto classInfo = context.getCurrentClassInfo(in_data);
					classInfo->mustRenameNodes[classDeclaration] = node;
				}
				return node;
			}
			auto arguments = loadListArgument(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, context.currentClassId, nullptr,
			    classDeclaration->getName(in_data) + "()", std::move(arguments),
			    context.justFindStatic, !nextTokenIfMarkNonNull(in_data, i),
			    false);
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				classInfo->mustRenameNodes[classDeclaration] = callNode;
			}
			return callNode;
		}
		case Lexer::TokenType::LPAREN: {
			uint32_t firstLine = token->line;
			auto arguments = loadListArgument(in_data, i);
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
			}
			auto callNode = context.callNodePool.push(
			    firstLine, context.currentClassId, nullptr,
			    context.lexerString[identifier->indexData] + "()",
			    std::move(arguments), context.justFindStatic,
			    !nextTokenIfMarkNonNull(in_data, i), false);
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				if (!classInfo->genericDeclarations.empty()) {
					auto it = classInfo->genericDeclarationMap.find(
					    identifier->indexData);
					if (it != classInfo->genericDeclarationMap.end()) {
						classInfo->genericDeclarations[it->second]
						    ->allCallNodes.push_back(callNode);
					}
				}
			}
			return callNode;
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
			return context.callNodePool.push(
			    firstLine, context.currentClassId,
			    static_cast<AccessNode *>(varNode), "[]", arguments,
			    context.justFindStatic, !nextTokenIfMarkNonNull(in_data, i),
			    false);
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
	token = &context.tokens[i];
	if (!allowAddThis) {
		return context.unknowNodePool.push(token->line, context.currentClassId,
		                                   identifier->indexData, nullable);
	}
	return findIdentifierNode(in_data, i, identifier->indexData, nullable);
}

HasClassIdNode *findIdentifierNode(in_func, size_t &i, LexerStringId nameId,
                                   bool nullable) {
	auto varNode =
	    findVarNode(in_data, i, context.lexerString[nameId], nullable);
	return varNode ? varNode
	               : context.unknowNodePool.push(context.tokens[i].line,
	                                             context.currentClassId, nameId,
	                                             nullable);
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
	return context.constValuePool.push(context.tokens[i].line, it->second.first,
	                                   it->second.second);
}

void ensureNoKeyword(in_func, size_t &i) {
	if (!context.modifierflags)
		return;
	throw ParserError(context.tokens[i].line,
	                  "Command doesn't support any keyword");
}

void ensureNoAnnotations(in_func, size_t &i) {
	if (!context.annotationFlags)
		return;
	throw ParserError(context.tokens[i].line,
	                  "Command doesn't support any annotations");
}

Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t &i) {
	// No keywords
	if (!context.modifierflags)
		return Lexer::TokenType::PUBLIC;
	if (context.modifierflags & ModifierFlags::MF_STATIC)
		throw ParserError(context.tokens[i].line,
		                  "Command doesn't support 'static' keyword");
	switch (context.modifierflags) {
		case ModifierFlags::MF_PUBLIC:
			return Lexer::TokenType::PUBLIC;
		case ModifierFlags::MF_PRIVATE:
			return Lexer::TokenType::PRIVATE;
		case ModifierFlags::MF_PROTECTED:
			return Lexer::TokenType::PROTECTED;
		default:
			throw ParserError(0, "Bug: Parser not ensure one modifier");
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
		std::string line = token->toString(context);
		while (nextTokenSameLine(&token, context.tokens, i, token->line)) {
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
	           ? context.constValuePool.push(
	                 token->line, static_cast<int64_t>(std::stoll(data)))
	           : context.constValuePool.push(
	                 token->line, static_cast<double>(std::stod(data)));
}

} // namespace AutoLang

#endif