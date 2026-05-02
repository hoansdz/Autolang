#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
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
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE_DATA) {
		throw ParserError(firstLine, "@native_data is only supported classes");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		throw ParserError(firstLine, "@no_extends is only supported classes");
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
	LexerStringId nameId = token->indexData;
	// ClassDeclaration *mustRenameFunctionDeclaration = nullptr;
	switch (token->type) {
		case Lexer::TokenType::IDENTIFIER: {
			switch (token->indexData) {
				case lexerId__FILE__: {
					throw ParserError(firstLine, "__FILE__ is a magic const");
				}
				case lexerId__LINE__: {
					throw ParserError(firstLine, "__LINE__ is a magic const");
				}
				case lexerId__FUNC__: {
					throw ParserError(firstLine, "__FUNC__ is a magic const");
				}
				case lexerId__CLASS__: {
					if (!context.currentClassId) {
						throw ParserError(firstLine,
						                  "Function name must not empty, must "
						                  "declare in class");
					}
					auto clazz = context.getCurrentClass(in_data);
					auto classInfo = context.getCurrentClassInfo(in_data);
					if (!classInfo->genericData) {
						nameId =
						    context.createLexerStringIfNotExists(clazz->name);
						break;
					}
					nameId = lexerId__CLASS__;
					break;
				}
				case lexerIdsuper: {
					throw ParserError(firstLine, "Super is a keyword");
				}
				default: {
					break;
				}
			}
			break;
		}
		default: {
			--i;
			throw ParserError(firstLine, "Expected name but not found");
		}
	}

	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine, "Expected ( but not found");
	}

	context.preloadGenericData = nullptr;

	switch (token->type) {
		case Lexer::TokenType::LT: {
			if (context.currentClassId) {
				throw ParserError(
				    token->line,
				    "Generic functions in class doesn't supported now");
			}
			functionFlags |= FunctionFlags::FUNC_SKIP_LOAD;
			context.isInGeneric = false;
			context.preloadGenericData = context.genericDataPool.push();
			while (true) {
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::IDENTIFIER)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected class name but not found");
				}
				auto &genericDeclarationName =
				    context.lexerString[token->indexData];

				if (context.preloadGenericData->findDeclaration(
				        token->indexData)) {
					throw ParserError(firstLine,
					                  "Redefined " + genericDeclarationName);
				}

				Offset id =
				    context.preloadGenericData->genericDeclarations.size();
				auto declarationData =
				    new GenericDeclarationNode(firstLine, token->indexData);
				context.preloadGenericData->genericDeclarations.push_back(
				    declarationData);
				context.preloadGenericData
				    ->genericDeclarationMap[token->indexData] = id;
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected '>' after class name but not found");
				}
				switch (token->type) {
					// case Lexer::TokenType::IS:
					case Lexer::TokenType::EXTENDS: {
						// auto condition =
						//     (token->type == Lexer::TokenType::EXTENDS)
						//         ? GenericDeclarationCondition::MUST_EXTENDS
						//         : GenericDeclarationCondition::MUST_IS;
						auto classDeclaration = loadClassDeclaration(
						    in_data, i, token->line, false);
						if (!nextToken(&token, context.tokens, i)) {
							throw ParserError(
							    firstLine,
							    "Expected '>' after class name but not found");
						}
						declarationData->condition =
						    GenericDeclarationCondition{classDeclaration};
						switch (token->type) {
							case Lexer::TokenType::COMMA: {
								break;
							}
							case Lexer::TokenType::GT: {
								goto finishedGenerics;
							}
							default: {
								throw ParserError(firstLine,
								                  "Expected '>' after class "
								                  "name but not found");
							}
						}
						break;
					}
					case Lexer::TokenType::COMMA: {
						break;
					}
					case Lexer::TokenType::GT: {
						goto finishedGenerics;
					}
					default: {
						throw ParserError(
						    firstLine,
						    "Expected '>' after class name but not found");
					}
				}
			}
		finishedGenerics:;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Generics class must have body");
			}
		}
	}

	// Arguments
	if (!expect(token, Lexer::TokenType::LPAREN)) {
		throw ParserError(firstLine, "Expected ( but '" +
		                                 context.tokens[i].toString(context) +
		                                 "' found");
	}
	auto parameter = loadListDeclaration(in_data, i);
	if (!parameter->parameterDefaultValues.empty() &&
	    !context.preloadGenericData) {
		if (context.currentClassId) {
			auto classInfo = context.classInfo[*context.currentClassId];
			if (!classInfo->genericData) {
				context.defaultValueParameter.push_back(parameter);
			}
		} else {
			context.defaultValueParameter.push_back(parameter);
		}
	}
	ClassDeclaration *classDeclaration = nullptr;
	// Return class name
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE)
			goto createFunc;
		throw ParserError(firstLine, "Expected body but not found");
	}
	if (token->type == Lexer::TokenType::COLON) {
		classDeclaration = loadClassDeclaration(in_data, i, token->line, true);
		if (!classDeclaration->isGenerics(in_data)) {
			context.allClassDeclarations.push_back(classDeclaration);
		}
		if (classDeclaration->nullable) {
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
				parameter->parameters.insert(
				    parameter->parameters.begin(),
				    context.getCurrentClassInfo(in_data)->declarationThis);
				parameter->defaultValuePos += 1;
			}
			CreateFuncNode *node = context.newFunctions.push(
			    firstLine, context.currentClassId, nameId, classDeclaration,
			    std::move(parameter), functionFlags);
			node->pushFunction(in_data);
			auto func = compile.functions[node->id];
			auto funcInfo = context.functionInfo[node->id];
			if (context.preloadGenericData) {
				context.genericFunctionMap[func->name] = node;
				funcInfo->genericData = context.preloadGenericData;
			}
			func->returnId = DefaultClass::nullClassId;
			context.gotoFunction(node->id);
			auto &scope = funcInfo->scopes.back();

			for (size_t i = 0; i < node->parameter->parameters.size(); ++i) {
				auto *param = node->parameter->parameters[i];
				param->id = i;
				scope[param->baseName] = param;
			}

			auto returnNode =
			    context.returnPool.push(firstLine, context.currentFunctionId,
			                            loadExpression(in_data, 0, i));
			funcInfo->body.nodes.push_back(returnNode);
			funcInfo->inferenceNode = returnNode;
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				if (!classInfo->genericData) {
					context.mustInferenceFunctionType.push_back(node->id);
				} else {
					returnNode->loaded = true;
				}
			} else {
				context.mustInferenceFunctionType.push_back(node->id);
			}
			context.gotoFunction(context.mainFunctionId);
			context.preloadGenericData = nullptr;
			if (context.isInGeneric)
				context.isInGeneric = false;
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
		parameter->parameters.insert(
		    parameter->parameters.begin(),
		    context.getCurrentClassInfo(in_data)->declarationThis);
		parameter->defaultValuePos += 1;
	}

	CreateFuncNode *node =
	    context.newFunctions.push(firstLine, context.currentClassId, nameId,
	                              classDeclaration, parameter, functionFlags);
	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto &token = context.annotationMetadata[AnnotationFlags::AN_NATIVE];
		const auto &name = context.lexerString[token.indexData];
		auto it = context.mode->nativeFuncMap.find(name);
		if (it == context.mode->nativeFuncMap.end()) {
			throw ParserError(firstLine, "Native function name '" + name +
			                                 "' could not be found");
		}
		node->pushNativeFunction(in_data, &it->second);
		// auto func = compile.functions[node->id];
		// context.gotoFunction(node->id);
		// for (size_t i = 1; i < node->parameter->parameters.size(); ++i) {
		// 	auto *param = node->parameter->parameters[i];
		// 	param->id = i;
		// }
		// context.gotoFunction(context.mainFunctionId);
		context.preloadGenericData = nullptr;
		if (context.isInGeneric)
			context.isInGeneric = false;
		return node;
	} else {
		node->pushFunction(in_data);
	}
	// compile.funcMap[compile.functions[node->id].name]->push_back(node->id);
	auto func = compile.functions[node->id];
	auto funcInfo = context.functionInfo[node->id];
	if (context.preloadGenericData) {
		context.genericFunctionMap[func->name] = node;
		funcInfo->genericData = context.preloadGenericData;
	}
	// std::cerr<<"Created "<<name+"()"<<" ->
	// "<<compile.classes[func->returnId]->name<<"\n";
	context.gotoFunction(node->id);
	auto &scope = funcInfo->scopes.back();

	for (size_t i = 0; i < node->parameter->parameters.size(); ++i) {
		auto *param = node->parameter->parameters[i];
		param->id = i;
		scope[param->baseName] = param;
	}
	try {
		context.justFindStaticMember =
		    (func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
		loadBody<false>(in_data, funcInfo->body.nodes, i);
		context.justFindStaticMember = false;
		context.gotoFunction(context.mainFunctionId);
		context.preloadGenericData = nullptr;
		if (context.isInGeneric)
			context.isInGeneric = false;
	} catch (const ParserError &err) {
		context.gotoFunction(context.mainFunctionId);
		context.preloadGenericData = nullptr;
		if (context.isInGeneric)
			context.isInGeneric = false;
		throw err;
	}

	if (classDeclaration) {
		bool hasReturn = false;
		for (size_t i = funcInfo->body.nodes.size(); i-- > 0;) {
			if (funcInfo->body.nodes[i]->kind == NodeType::RET) {
				hasReturn = true;
				break;
			}
		}
		if (!hasReturn) {
			throw ParserError(firstLine, "Function " + func->name +
			                                 " didn't declare return");
		}
	}

	return node;
}

template <bool hasParams> CreateClosureNode *loadClosure(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine, "Expected body but not found");
	}
	AutoLang::Parameter *parameter;
	auto classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->classId = DefaultClass::functionClassId;
	classDeclaration->baseClassLexerStringId = lexerIdFunction;
	classDeclaration->nullable = false;
	bool loadedLBrace = true;
	if constexpr (hasParams) {
		if (token->type == Lexer::TokenType::OR) {
			parameter = loadListDeclaration<AutoLang::Lexer::OR, false, false>(
			    in_data, i, false);
			classDeclaration->inputClassId.reserve(
			    parameter->parameters.size() + 1);
			classDeclaration->inputClassId.push_back(nullptr);
			classDeclaration->line = firstLine;
			// if (!isGeneric) {
			// 	context.allClassDeclarations.push_back(classDeclaration);
			// }
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(firstLine, "Expected body but not found");
			}
			if (!expect(token, Lexer::TokenType::MINUS_GT)) {
				--i;
				goto createClosure;
			}
			loadedLBrace = false;
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LBRACE)) {
				--i;
				throw ParserError(firstLine, "Expected body but not found");
			}
		} else {
			throw ParserError(
			    firstLine,
			    "Error: Empty closure parameter list is not allowed\nNote: Use "
			    "{|param|} instead of {} to declare parameters explicitly");
			--i;
			parameter = context.parameterPool.push();
		}
	} else {
		parameter = context.parameterPool.push();
		classDeclaration->inputClassId.push_back(nullptr);
		classDeclaration->line = firstLine;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(firstLine, "Expected body but not found");
		}
		if (!expect(token, Lexer::TokenType::MINUS_GT)) {
			--i;
			goto createClosure;
		}
		loadedLBrace = false;
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LBRACE)) {
			--i;
			throw ParserError(firstLine, "Expected body but not found");
		}
	}
createClosure:;
	auto createClosureNode =
	    context.createClosurePool.push(firstLine, parameter);
	createClosureNode->classDeclaration = classDeclaration;

	for (auto declaration : parameter->parameters) {
		classDeclaration->inputClassId.push_back(declaration->classDeclaration);
		auto &scope = createClosureNode->scopes.back();
		scope[declaration->baseName] = declaration;
		if (!classDeclaration->isGeneric) {
			if (!declaration->classDeclaration) {
				if (!createClosureNode->mustInfer) {
					createClosureNode->mustInfer = true;
				}
				continue;
			}
			classDeclaration->isGeneric =
			    declaration->classDeclaration->isGeneric;
		}
	}
	context.allClosureNode.push_back(createClosureNode);

	auto lastCurrentClosureNode = context.currentClosureNode;
	context.currentClosureNode = createClosureNode;
	context.closureScopes.push_back(createClosureNode);
	if (loadedLBrace) {
		loadBody<true>(in_data, createClosureNode->body.nodes, i);
	} else {
		loadBody<false>(in_data, createClosureNode->body.nodes, i);
		nextToken(&token, context.tokens, i);
	}
	context.currentClosureNode = lastCurrentClosureNode;
	context.closureScopes.pop_back();
	return createClosureNode;
}

ReturnNode *loadReturn(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	auto func = context.getCurrentFunction(in_data);
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		--i;
		if (context.currentClassId) {
			auto declarartionThis =
			    context.getCurrentClassInfo(in_data)->declarationThis;
			auto value =
			    (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR)
			        ? context.varPool.push(firstLine, declarartionThis, false,
			                               false)
			        : nullptr;
			return context.returnPool.push(firstLine, context.currentFunctionId,
			                               value);
		}
		return context.returnPool.push(firstLine, context.currentFunctionId,
		                               nullptr);
	}
	if (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR)
		throw ParserError(token->line, "Cannot return value in constructor");
	return context.returnPool.push(firstLine, context.currentFunctionId,
	                               loadExpression(in_data, 0, i));
}

} // namespace AutoLang

#endif