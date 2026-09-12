#ifndef DEBUGGER_FUNCTION_CPP
#define DEBUGGER_FUNCTION_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/FunctionFlags.hpp"

namespace Autolang {

static void checkGenericFunctionDuplicate(in_func, CreateFuncNode *node,
                                          LexerStringId nameId, uint32_t line) {
	auto it = context.genericFunctionMap.find(nameId);
	if (it == context.genericFunctionMap.end())
		return;
	auto currentFuncInfo = context.functionInfo[node->id];
	if (!currentFuncInfo || !context.preloadGenericData)
		return;
	size_t genericCount =
	    context.preloadGenericData->genericDeclarations.size();
	size_t paramCount = node->parameter->parameters.size();

	for (auto existingNode : it->second) {
		auto existingFuncInfo = context.functionInfo[existingNode->id];
		if (!existingFuncInfo || !existingFuncInfo->genericData)
			continue;
		if (existingFuncInfo->genericData->genericDeclarations.size() !=
		    genericCount)
			continue;
		if (existingNode->parameter->parameters.size() != paramCount)
			continue;

		bool match = true;
		for (size_t p = 0; p < paramCount; ++p) {
			auto p1 = node->parameter->parameters[p];
			auto p2 = existingNode->parameter->parameters[p];
			if (p1->nullable != p2->nullable) {
				match = false;
				break;
			}
			if (p1->classDeclaration && p2->classDeclaration) {
				if (p1->classDeclaration->baseClassLexerStringId !=
				    p2->classDeclaration->baseClassLexerStringId) {
					match = false;
					break;
				}
			} else if (p1->classDeclaration != p2->classDeclaration) {
				match = false;
				break;
			} else if (p1->classId != p2->classId) {
				match = false;
				break;
			}
		}

		if (match) {
			std::string prevPath =
			    existingNode->mode ? existingNode->mode->path : "unknown";
			throw ParserError(
			    line,
			    "Redefined function: " + currentFuncInfo->toString(in_data) +
			        "\nHint: Previously defined at " + prevPath + ":" +
			        std::to_string(existingFuncInfo->line) +
			        ". Ensure the function signature is unique or remove the "
			        "duplicate definition");
		}
	}
}

static void registerExtensionToGenericClass(in_func, ClassId targetClassId,
                                            CreateFuncNode *node) {
	auto extClassInfo = context.classInfo[targetClassId];
	if (!extClassInfo || !extClassInfo->genericData)
		return;
	extClassInfo->createFunctionNodes.push_back(node);

	for (size_t cid = 0; cid < compile.classes.size(); ++cid) {
		auto clazz = compile.classes[cid];
		if (!clazz || clazz->genericBaseClassId != targetClassId)
			continue;
		auto subClassInfo = context.classInfo[cid];
		if (!subClassInfo)
			continue;
		auto paramCopy = node->parameter->copy(in_data);
		if (!paramCopy->parameters.empty() &&
		    paramCopy->parameters[0]->baseName == lexerIdthis) {
			paramCopy->parameters[0]->classId = cid;
		}
		auto newCreateFuncNode = context.newFunctions.push(
		    node->line, node->tokenIndex, cid, node->nameId,
		    nullptr, paramCopy, node->functionFlags & ~FunctionFlags::FUNC_SKIP_LOAD);
		if (node->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			newCreateFuncNode->pushNativeFunction(
			    in_data, compile.functions[node->id]->native);
		} else {
			newCreateFuncNode->pushFunction(in_data);
		}
		auto newFunc = compile.functions[newCreateFuncNode->id];
		auto newFuncInfo = context.functionInfo[newCreateFuncNode->id];
		auto funcInfo = context.functionInfo[node->id];
		newFunc->returnId = compile.functions[node->id]->returnId;
		if (node->classDeclaration) {
			if (!node->classDeclaration->classId) {
				node->classDeclaration->template load<true>(in_data);
				if (node->classDeclaration->classId) {
					newFunc->returnId = *node->classDeclaration->classId;
					node->classDeclaration->classId = std::nullopt;
				}
			} else {
				newFunc->returnId = *node->classDeclaration->classId;
			}
			if (newFunc->returnId == DefaultClass::functionClassId) {
				newFuncInfo->returnClass =
				    node->classDeclaration->copy(in_data);
			}
		}
		auto lastCurrentFunctionId = context.currentFunctionId;
		auto lastCurrentClassId = context.currentClassId;
		context.gotoFunction(newCreateFuncNode->id);
		context.currentClassId = cid;
		newFuncInfo->body.nodes.reserve(funcInfo->body.nodes.size());
		for (auto &[declarationNode, value] : funcInfo->reflectDeclarationMap) {
			value =
			    static_cast<DeclarationNode *>(declarationNode->copy(in_data));
		}
		for (auto *bodyNode : funcInfo->body.nodes) {
			newFuncInfo->body.nodes.push_back(bodyNode->copy(in_data));
		}
		if (funcInfo->inferenceNode) {
			newFuncInfo->inferenceNode =
			    static_cast<ReturnNode *>(newFuncInfo->body.nodes[0]);
			newFuncInfo->inferenceNode->loaded = false;
			context.mustInferenceFunctionType.push_back(newFunc->id);
		}
		context.gotoFunction(lastCurrentFunctionId);
		context.currentClassId = lastCurrentClassId;
	}
}

CreateFuncNode *loadFunc(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	uint32_t tokenIndex = i;

	uint32_t functionFlags = 0;
	if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
		throw ParserError(firstLine,
		                  "@no_constructor is only supported on classes\nHint: "
		                  "Remove @no_constructor from function declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE_DATA) {
		throw ParserError(firstLine,
		                  "@native_data is only supported on classes\nHint: "
		                  "Remove @native_data from function declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		throw ParserError(firstLine,
		                  "@no_extends is only supported on classes\nHint: "
		                  "Remove @no_extends from function declaration");
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
	if (context.annotationFlags & AnnotationFlags::AN_OPERATOR) {
		functionFlags |= FunctionFlags::FUNC_IS_OPERATOR;
	}
	if (context.annotationFlags & AnnotationFlags::AN_IMPLICIT) {
		functionFlags |= FunctionFlags::FUNC_IS_IMPLICIT;
		context.annotationFlags &= ~AnnotationFlags::AN_IMPLICIT;
	}
	if (context.annotationFlags & AnnotationFlags::AN_WAIT_INPUT) {
		throw ParserError(firstLine,
		                  "@wait_input is currently not supported\nHint: "
		                  "Remove @wait_input annotation");
	}
	bool hasStaticFlag = (context.modifierflags & ModifierFlags::MF_STATIC);
	if (!context.currentClassId || hasStaticFlag) {
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
	// Generic parameters (Kotlin-style: fun <T> foo()) or Name
	context.preloadGenericData = nullptr;
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected name after 'fun' but not found\nHint: Provide a valid "
		    "identifier for function name, e.g. 'fun foo()'");
	}
	if (expect(token, Lexer::TokenType::LT)) {
		functionFlags |= FunctionFlags::FUNC_SKIP_LOAD;
		context.preloadGenericData = loadGenericParameters(in_data, i);
		context.isInGeneric = false;
		if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected function name after generic parameters\nHint: "
			    "Provide a function name, e.g. 'fun <T> foo()'");
		}
	}
	if (!expect(token, Lexer::TokenType::IDENTIFIER) &&
	    !expect(token, Lexer::TokenType::TO)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected name after 'fun' but not found\nHint: Provide a valid "
		    "identifier for function name, e.g. 'fun foo()'");
	}
	std::optional<LexerStringId> classNameId;
	LexerStringId nameId = token->indexData;
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '(' after function name but not "
		                  "found\nHint: Add '(' to start parameter list");
	}
	if (token->type == Lexer::TokenType::LT) {
		auto classIt = compile.classMap.find(context.lexerString[nameId]);
		if (classIt != compile.classMap.end() &&
		    context.classInfo[classIt->second]->genericData != nullptr) {
			if (!context.preloadGenericData) {
				context.preloadGenericData = loadGenericParameters(in_data, i);
				context.isInGeneric = false;
			} else {
				int ltDepth = 1;
				while (ltDepth > 0 &&
				       nextTokenSameLine(&token, context.tokens, i, firstLine)) {
					if (token->type == Lexer::TokenType::LT) {
						++ltDepth;
					} else if (token->type == Lexer::TokenType::GT) {
						--ltDepth;
					}
				}
			}
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(
				    firstLine,
				    "Expected '.' after generic class name in extension "
				    "declaration\nHint: Use syntax 'ClassName<...>.methodName()'");
			}
		}
	}
	if (token->type != Lexer::TokenType::DOT && context.currentClassId &&
	    !hasStaticFlag) {
		auto clazz = context.getCurrentClass(in_data);
		if (!(clazz->classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
		    context.lexerString[nameId] == clazz->getName(compile)) {
			auto classInfo = context.getCurrentClassInfo(in_data);
			if (classInfo->primaryConstructor) {
				throw ParserError(
				    firstLine,
				    "Cannot declare constructor in a data class\nHint: "
				    "Data classes use primary constructor header 'class "
				    "Name(...)'");
			}
			if (functionFlags & FunctionFlags::FUNC_OVERRIDE) {
				throw ParserError(
				    firstLine,
				    "@override is only supported on functions\nHint: "
				    "Remove @override from constructor declaration");
			}
			if (functionFlags & FunctionFlags::FUNC_NO_OVERRIDE) {
				throw ParserError(
				    firstLine,
				    "@no_override is only supported on functions\nHint: "
				    "Remove @no_override from constructor declaration");
			}
			if (functionFlags & FunctionFlags::FUNC_IS_OPERATOR) {
				throw ParserError(
				    firstLine,
				    "@operator is not supported on constructors\nHint: "
				    "Remove @operator from constructor declaration");
			}
			if ((clazz->classFlags & ClassFlags::CLASS_NATIVE_DATA) &&
			    !(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
				throw ParserError(
				    firstLine,
				    "Class " + clazz->getName(compile) +
				        " is marked @native_data, so the constructor must be "
				        "native\nHint: Add @native(\"name\") to constructor");
			}
			loadConstructorBody(in_data, i, firstLine, functionFlags, clazz,
			                    classInfo);
			return nullptr;
		}
	}
	if (token->type == Lexer::TokenType::DOT) {
		if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
			--i;
			throw ParserError(
			    firstLine, "Expected function name after class name: '" +
			                   context.lexerString[*classNameId] +
			                   "' but not found\nHint: Specify member function "
			                   "name after dot, e.g. 'Class.foo()'");
		}
		switch (token->type) {
			case Lexer::TokenType::LT: {
				throw ParserError(
				    firstLine,
				    "Generic class extension hasn't supported yet\nHint: "
				    "Remove generic type arguments from class extension");
			}
			case Lexer::TokenType::IDENTIFIER: {
				classNameId = nameId;
				nameId = token->indexData;
				functionFlags |= FunctionFlags::FUNC_UNUSABLE;
				if (!hasStaticFlag) {
					functionFlags &= ~FunctionFlags::FUNC_IS_STATIC;
				}
				if (context.currentClassId) {
					throw ParserError(
					    token->line,
					    "Error: Extension function are not "
					    "allowed inside class\nHint: Declare extension "
					    "function at file scope outside class");
				}
				if (context.currentFunctionId != context.mainFunctionId) {
					throw ParserError(
					    token->line,
					    "Error: Extension function are not "
					    "allowed inside function\nHint: Declare extension "
					    "function at file scope outside function");
				}
				if (context.currentClosureNode) {
					throw ParserError(
					    token->line,
					    "Error: Extension function are not "
					    "allowed inside closure\nHint: Declare extension "
					    "function at file scope outside closure");
				}
				auto it =
				    compile.classMap.find(context.lexerString[*classNameId]);
				if (it == compile.classMap.end()) {
					throw ParserError(firstLine,
					                  "Cannot find class name: '" +
					                      context.lexerString[*classNameId] +
					                      "'. Extension must be declared after "
					                      "class\nHint: Ensure target class is "
					                      "declared before creating extension");
				}
				context.currentClassId = it->second;
				auto extClassInfo = context.classInfo[it->second];
				if (extClassInfo && extClassInfo->genericData) {
					if (context.preloadGenericData) {
						for (size_t g = 0;
						     g < context.preloadGenericData->genericDeclarations.size();
						     ++g) {
							auto srcDecl =
							    context.preloadGenericData->genericDeclarations[g];
							GenericDeclarationNode *targetDecl = nullptr;
							targetDecl = extClassInfo->findGenericDeclaration(
							    srcDecl->nameId);
							if (!targetDecl &&
							    g < extClassInfo->genericData
							            ->genericDeclarations.size()) {
								targetDecl = extClassInfo->genericData
								                 ->genericDeclarations[g];
							}
							if (targetDecl) {
								for (auto *cd : srcDecl->allClassDeclarations) {
									targetDecl->allClassDeclarations.push_back(cd);
								}
								for (auto *cn : srcDecl->allCallNodes) {
									targetDecl->allCallNodes.push_back(cn);
								}
							}
						}
						context.preloadGenericData = nullptr;
					}
				}
				if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
					--i;
					throw ParserError(
					    firstLine,
					    "Expected '(' after function name but not found\nHint: "
					    "Add '(' to start parameter list");
				}
				break;
			}
			default: {
				throw ParserError(firstLine,
				                  "Expected function name after class name: '" +
				                      context.lexerString[*classNameId] +
				                      "' but not found\nHint: Specify member "
				                      "function name after dot");
			}
		}
	}
	if (!context.currentClassId && context.modifierflags) {
		switch (accessModifier) {
			case Lexer::TokenType::PUBLIC: {
				throw ParserError(
				    token->line,
				    "Error: 'public' can only be used for class members\nHint: "
				    "Remove 'public' modifier from file-level function");
				break;
			}
			case Lexer::TokenType::PRIVATE: {
				throw ParserError(token->line,
				                  "Error: 'private' can only be used for class "
				                  "members\nHint: Remove 'private' modifier "
				                  "from file-level function");
				break;
			}
			case Lexer::TokenType::PROTECTED: {
				throw ParserError(token->line,
				                  "Error: 'protected' can only be used for "
				                  "class members\nHint: Remove 'protected' "
				                  "modifier from file-level function");
				break;
			}
			default:
				break;
		}
	}
	switch (nameId) {
		case lexerId__FILE__: {
			throw ParserError(firstLine, "__FILE__ is a magic const\nHint: "
			                             "Choose a different function name");
		}
		case lexerId__LINE__: {
			throw ParserError(firstLine, "__LINE__ is a magic const\nHint: "
			                             "Choose a different function name");
		}
		case lexerId__FUNC__: {
			throw ParserError(firstLine, "__FUNC__ is a magic const\nHint: "
			                             "Choose a different function name");
		}
		case lexerId__CLASS__: {
			if (!context.currentClassId) {
				throw ParserError(
				    firstLine,
				    "Function name cannot be empty; __CLASS__ "
				    "must be used inside a class\nHint: Use __CLASS__ only "
				    "within class method declarations");
			}
			auto clazz = context.getCurrentClass(in_data);
			auto classInfo = context.getCurrentClassInfo(in_data);
			if (!classInfo->genericData) {
				nameId = context.createLexerStringIfNotExists(
				    clazz->getName(compile));
				break;
			}
			nameId = lexerId__CLASS__;
			break;
		}
		case lexerIdsuper: {
			throw ParserError(firstLine, "'super' is a reserved keyword\nHint: "
			                             "Choose a different function name");
		}
		default: {
			break;
		}
	}

	if (functionFlags & FunctionFlags::FUNC_IS_OPERATOR) {
		if (nameId != lexerIdget && nameId != lexerIdset && nameId != lexerIdcontains) {
			throw ParserError(
			    firstLine,
			    "'" + context.lexerString[nameId] +
			        "' is not a supported operator function name\nHint: Supported operator "
			        "function names are 'get', 'set', 'contains'");
		}
	}

	if (!context.preloadGenericData && expect(token, Lexer::TokenType::LT)) {
		functionFlags |= FunctionFlags::FUNC_SKIP_LOAD;
		context.preloadGenericData = loadGenericParameters(in_data, i);
		context.isInGeneric = false;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Generics function must have body\nHint: "
			                  "Provide function body or implementation");
		}
	}

	// Arguments
	if (!expect(token, Lexer::TokenType::LPAREN)) {
		throw ParserError(firstLine,
		                  "Expected '(' but '" +
		                      context.tokens[i].toString(context) +
		                      "' found\nHint: Ensure function parameters are "
		                      "enclosed in '(' and ')'");
	}
	auto parameter = loadListDeclaration(in_data, i);
	if (functionFlags & FunctionFlags::FUNC_IS_OPERATOR) {
		size_t paramCount = parameter->parameters.size();
		if (nameId == lexerIdget && paramCount < 1) {
			throw ParserError(
			    firstLine,
			    "Operator 'get' requires at least 1 parameter\nHint: "
			    "Define 'get' with at least 1 index parameter, e.g. '@operator fun get(index: Int)'");
		}
		if (nameId == lexerIdset && paramCount < 2) {
			throw ParserError(
			    firstLine,
			    "Operator 'set' requires at least 2 parameters\nHint: "
			    "Define 'set' with index and value parameters, e.g. '@operator fun set(index: Int, value: T)'");
		}
		if (nameId == lexerIdcontains && paramCount != 1) {
			throw ParserError(
			    firstLine,
			    "Operator 'contains' requires exactly 1 parameter\nHint: "
			    "Define 'contains' with 1 parameter, e.g. '@operator fun contains(item: T): Bool'");
		}
	}
	if (functionFlags & FunctionFlags::FUNC_IS_IMPLICIT) {
		if (parameter->defaultValuePos > 1) {
			throw ParserError(
			    firstLine,
			    "Implicit function must have at most 1 required parameter\nHint: "
			    "Provide default values for additional parameters or declare function with at most 1 parameter");
		}
	}
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
		throw ParserError(firstLine,
		                  "Expected body but not found\nHint: Provide function "
		                  "body '{ ... }' or '@native' annotation");
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
			throw ParserError(firstLine,
			                  "Expected body but not found\nHint: Provide "
			                  "function body '{ ... }' or '=' expression");
		}
	}

	switch (token->type) {
		case Lexer::TokenType::LBRACE: {
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
				--i;
				throw ParserError(
				    firstLine,
				    "@native function must not have a body\nHint: End native "
				    "function header without '{ ... }' body");
			}
			break;
		}
		case Lexer::TokenType::EQUAL: {
			if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
				--i;
				throw ParserError(
				    firstLine,
				    "@native function must not have a body\nHint: End native "
				    "function header without '=' expression");
			}
			if (!nextToken(&token, context.tokens, i)) {
				throw ParserError(
				    firstLine, "Expected value after '=' but not found\nHint: "
				               "Provide an expression after '='");
			}
			if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
			    context.currentClassId) {
				parameter->parameters.insert(
				    parameter->parameters.begin(),
				    context.getCurrentClassInfo(in_data)->declarationThis);
				parameter->defaultValuePos += 1;
			}
			CreateFuncNode *node = context.newFunctions.push(
			    firstLine, tokenIndex, context.currentClassId, nameId,
			    classDeclaration, std::move(parameter), functionFlags);
			node->pushFunction(in_data);
			auto func = compile.functions[node->id];
			auto funcInfo = context.functionInfo[node->id];
			if (context.preloadGenericData) {
				if (context.currentClassId) {
					auto currentClassInfo = context.getCurrentClassInfo(in_data);
					currentClassInfo->genericFunctionMap[nameId].push_back(node);
					funcInfo->genericData = context.preloadGenericData;
				} else {
					checkGenericFunctionDuplicate(in_data, node, nameId, firstLine);
					context.genericFunctionMap[nameId].push_back(node);
					funcInfo->genericData = context.preloadGenericData;
				}
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
			} else if (!context.preloadGenericData) {
				context.mustInferenceFunctionType.push_back(node->id);
			}
			context.gotoFunction(context.mainFunctionId);
			context.preloadGenericData = nullptr;
			if (context.isInGeneric)
				context.isInGeneric = false;
			if (classNameId) {
				if (context.currentClassId) {
					registerExtensionToGenericClass(in_data, *context.currentClassId, node);
					context.currentClassId = std::nullopt;
				}
			}
			return node;
		}
		default: {
			--i;
			if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
				throw ParserError(firstLine,
				                  "Expected body but not found\nHint: Provide "
				                  "function body '{ ... }' or '=' expression");
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

	CreateFuncNode *node = context.newFunctions.push(
	    firstLine, tokenIndex, context.currentClassId, nameId, classDeclaration,
	    parameter, functionFlags);
	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto &token =
		    context.annotationMetadata[AnnotationMetadataIndex::AMI_NATIVE];
		const auto &name = context.lexerString[token.indexData];
		auto it = context.mode->nativeFuncMap.find(name);
		if (it == context.mode->nativeFuncMap.end()) {
			throw ParserError(firstLine,
			                  "Native function name '" + name +
			                      "' could not be found\nHint: Register native "
			                      "function binding in host environment");
		}
		node->pushNativeFunction(in_data, &it->second);
		auto func = compile.functions[node->id];
		auto funcInfo = context.functionInfo[node->id];
		if (context.preloadGenericData) {
			if (context.currentClassId) {
				auto currentClassInfo = context.getCurrentClassInfo(in_data);
				currentClassInfo->genericFunctionMap[nameId].push_back(node);
				funcInfo->genericData = context.preloadGenericData;
				context.preloadGenericData = nullptr;
			} else {
				checkGenericFunctionDuplicate(in_data, node, nameId, firstLine);
				context.genericFunctionMap[nameId].push_back(node);
				funcInfo->genericData = context.preloadGenericData;
				context.preloadGenericData = nullptr;
			}
		}
		// auto func = compile.functions[node->id];
		// context.gotoFunction(node->id);
		// for (size_t i = 1; i < node->parameter->parameters.size(); ++i) {
		// 	auto *param = node->parameter->parameters[i];
		// 	param->id = i;
		// }
		// context.gotoFunction(context.mainFunctionId);
		if (context.isInGeneric)
			context.isInGeneric = false;
		if (classNameId) {
			if (context.currentClassId) {
				registerExtensionToGenericClass(in_data, *context.currentClassId, node);
				context.currentClassId = std::nullopt;
			}
		}
		return node;
	} else {
		node->pushFunction(in_data);
	}
	// compile.funcMap[compile.functions[node->id].name]->push_back(node->id);
	auto func = compile.functions[node->id];
	auto funcInfo = context.functionInfo[node->id];
	if (!classDeclaration) {
		func->returnId = DefaultClass::voidClassId;
	}
	if (context.preloadGenericData) {
		if (context.currentClassId) {
			auto currentClassInfo = context.getCurrentClassInfo(in_data);
			currentClassInfo->genericFunctionMap[nameId].push_back(node);
			funcInfo->genericData = context.preloadGenericData;
		} else {
			checkGenericFunctionDuplicate(in_data, node, nameId, firstLine);
			context.genericFunctionMap[nameId].push_back(node);
			funcInfo->genericData = context.preloadGenericData;
		}
	}
	// std::cerr<<"Created "<<name+"()"<<" ->
	// "<<compile.classes[func->returnId]->getName(compile)<<"\n";
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
		bool finished = loadBody<false>(in_data, funcInfo->body.nodes, i);
		context.justFindStaticMember = false;
		context.gotoFunction(context.mainFunctionId);
		context.preloadGenericData = nullptr;
		if (context.isInGeneric)
			context.isInGeneric = false;
		if (finished && classDeclaration &&
		    classDeclaration->baseClassLexerStringId != lexerIdVoid) {
			auto checkHasReturn = [](auto &self, ExprNode *exprNode) -> bool {
				if (!exprNode)
					return false;
				if (exprNode->kind == NodeType::RET)
					return true;
				if (exprNode->kind == NodeType::TRY_CATCH) {
					auto tc = static_cast<TryCatchNode *>(exprNode);
					for (auto child : tc->body.nodes) {
						if (self(self, child))
							return true;
					}
					if (tc->hasCatch) {
						for (auto &clause : tc->catchClauses) {
							for (auto child : clause.body.nodes) {
								if (self(self, child))
									return true;
							}
						}
					}
					if (tc->hasFinally) {
						for (auto child : tc->finallyBody.nodes) {
							if (self(self, child))
								return true;
						}
					}
				}
				return false;
			};
			bool hasReturn = false;
			for (size_t i = funcInfo->body.nodes.size(); i-- > 0;) {
				if (checkHasReturn(checkHasReturn, funcInfo->body.nodes[i])) {
					hasReturn = true;
					break;
				}
			}
			if (!hasReturn) {
				throw ParserError(firstLine,
				                  "Function " + func->getName(compile) +
				                      " is missing a return statement\nHint: "
				                      "Return a value of type '" +
				                      classDeclaration->getName(in_data) + "'");
			}
		}
	} catch (const ParserError &err) {
		context.gotoFunction(context.mainFunctionId);
		context.preloadGenericData = nullptr;
		if (context.isInGeneric)
			context.isInGeneric = false;
		if (classNameId) {
			context.currentClassId = std::nullopt;
		}
		throw err;
	}

	if (classNameId) {
		if (context.currentClassId) {
			registerExtensionToGenericClass(in_data, *context.currentClassId, node);
			context.currentClassId = std::nullopt;
		}
	}
	return node;
}

template <bool hasParams> CreateClosureNode *loadClosure(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine, "Expected body but not found\nHint: "
		                             "Provide closure body inside '{ ... }'");
	}
	Autolang::Parameter *parameter;
	auto classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->classId = DefaultClass::functionClassId;
	classDeclaration->baseClassLexerStringId = lexerIdFunction;
	classDeclaration->nullable = false;
	bool loadedLBrace = true;
	if constexpr (hasParams) {
		if (token->type == Lexer::TokenType::OR) {
			parameter = loadListDeclaration<Autolang::Lexer::OR, false, false>(
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
				throw ParserError(firstLine,
				                  "Expected body but not found\nHint: Provide "
				                  "closure body inside '{ ... }'");
			}
			if (!expect(token, Lexer::TokenType::MINUS_GT)) {
				--i;
				goto createClosure;
			}
			loadedLBrace = false;
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LBRACE)) {
				--i;
				throw ParserError(firstLine,
				                  "Expected body but not found\nHint: Provide "
				                  "closure body inside '{ ... }'");
			}
		} else if (token->type == Lexer::TokenType::MINUS_GT) {
			parameter = context.parameterPool.push();
			classDeclaration->inputClassId.push_back(nullptr);
			classDeclaration->line = firstLine;
			loadedLBrace = true;
			goto createClosure;
		} else if (token->type == Lexer::TokenType::OR_OR) {
			parameter = context.parameterPool.push();
			classDeclaration->inputClassId.push_back(nullptr);
			classDeclaration->line = firstLine;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(firstLine,
				                  "Expected body but not found\nHint: Provide "
				                  "closure body inside '{ ... }'");
			}
			if (!expect(token, Lexer::TokenType::MINUS_GT)) {
				--i;
				goto createClosure;
			}
			loadedLBrace = false;
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LBRACE)) {
				--i;
				throw ParserError(firstLine,
				                  "Expected body but not found\nHint: Provide "
				                  "closure body inside '{ ... }'");
			}
		} else if (hasArrowAtCurrentBraceLevel(context.tokens, i)) {
			--i;
			parameter = loadListDeclaration<Lexer::TokenType::MINUS_GT, false, false>(
			    in_data, i, false);
			classDeclaration->inputClassId.reserve(
			    parameter->parameters.size() + 1);
			classDeclaration->inputClassId.push_back(nullptr);
			classDeclaration->line = firstLine;
			loadedLBrace = true;
			goto createClosure;
		} else {
			--i;
			parameter = context.parameterPool.push();
			classDeclaration->inputClassId.push_back(nullptr);
			classDeclaration->line = firstLine;
			loadedLBrace = true;
			goto createClosure;
		}
	} else {
		parameter = context.parameterPool.push();
		classDeclaration->inputClassId.push_back(nullptr);
		classDeclaration->line = firstLine;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(firstLine,
			                  "Expected body but not found\nHint: Provide "
			                  "closure body inside '{ ... }'");
		}
		if (!expect(token, Lexer::TokenType::MINUS_GT)) {
			--i;
			goto createClosure;
		}
		loadedLBrace = false;
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LBRACE)) {
			--i;
			throw ParserError(firstLine,
			                  "Expected body but not found\nHint: Provide "
			                  "closure body inside '{ ... }'");
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

	if (context.currentClassId) {
		auto classInfo = context.classInfo[*context.currentClassId];
		if (!classInfo->genericData) {
			context.allClosureNode.push_back(createClosureNode);
		}
	} else {
		auto funcInfo = context.getCurrentFunctionInfo(in_data);
		if (!funcInfo->genericData) {
			context.allClosureNode.push_back(createClosureNode);
		}
	}

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
	if (!context.currentClosureNode &&
	    context.currentFunctionId == context.mainFunctionId) {
		throw ParserError(
		    firstLine,
		    context.currentClassId
		        ? "'return' cannot be used directly in class body\nHint: Place "
		          "'return' inside a method body"
		        : "'return' statement outside of a function\nHint: Place "
		          "'return' inside a function or closure body");
	}

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
		throw ParserError(token->line,
		                  "Cannot return value in constructor\nHint: Use empty "
		                  "'return' without a return value in constructor");
	return context.returnPool.push(firstLine, context.currentFunctionId,
	                               loadExpression(in_data, 0, i));
}

template CreateClosureNode *loadClosure<false>(in_func, size_t &i);
template CreateClosureNode *loadClosure<true>(in_func, size_t &i);

} // namespace Autolang

#endif