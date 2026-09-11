#ifndef DEBUGGER_GENERIC_CLASS_CPP
#define DEBUGGER_GENERIC_CLASS_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

GenericData *loadGenericParameters(in_func, size_t &i) {
	Lexer::Token *token = nullptr;
	uint32_t firstLine = context.tokens[i].line;
	GenericData *genericData = context.genericDataPool.push();
	context.isInGeneric = true;
	while (true) {
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::IDENTIFIER)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected class name but not found\nHint: Provide "
			    "generic type parameter name, e.g. '<T>'");
		}
		auto &genericDeclarationName = context.lexerString[token->indexData];
		if (genericData->findDeclaration(token->indexData)) {
			throw ParserError(
			    firstLine,
			    "Redefined " + genericDeclarationName +
			        "\nHint: Use unique generic type parameter names");
		}
		Offset id = genericData->genericDeclarations.size();
		auto declarationData = context.genericDeclarationNodePool.push(
		    firstLine, token->indexData);
		genericData->genericDeclarations.push_back(declarationData);
		genericData->genericDeclarationMap[token->indexData] = id;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected '>' after class name but not found\nHint: "
			    "Close generic parameter list with '>'");
		}
		switch (token->type) {
			case Lexer::TokenType::COLON:
			case Lexer::TokenType::EXTENDS: {
				auto classDeclaration =
				    loadClassDeclaration(in_data, i, token->line, false);
				if (!nextToken(&token, context.tokens, i)) {
					throw ParserError(
					    firstLine, "Expected '>' after class name but "
					               "not found\nHint: Close generic "
					               "parameter list with '>'");
				}
				declarationData->condition =
				    GenericDeclarationCondition{classDeclaration};
				switch (token->type) {
					case Lexer::TokenType::COMMA: {
						break;
					}
					case Lexer::TokenType::GT: {
						return genericData;
					}
					default: {
						throw ParserError(
						    firstLine,
						    "Expected '>' after class "
						    "name but not found\nHint: Close generic "
						    "parameter list with '>'");
					}
				}
				break;
			}
			case Lexer::TokenType::COMMA: {
				break;
			}
			case Lexer::TokenType::GT: {
				return genericData;
			}
			default: {
				throw ParserError(firstLine,
				                  "Expected '>' after class name but "
				                  "not found\nHint: Close generic "
				                  "parameter list with '>'");
			}
		}
	}
}

// Helper: reset classId đệ quy trên cây ClassDeclaration để chuẩn bị load lại
static void resetClassDeclTreeHelper(ClassDeclaration *decl, int depth) {
	if (!decl || depth > 16)
		return;
	if (!decl->isGenericDeclaration && !decl->inputClassId.empty()) {
		decl->classId = std::nullopt;
	}
	for (auto *child : decl->inputClassId) {
		if (child && child != decl) {
			resetClassDeclTreeHelper(child, depth + 1);
		}
	}
}

void resetClassDeclTree(ClassDeclaration *decl) {
	resetClassDeclTreeHelper(decl, 0);
}

template <bool isLazy>
ClassId loadClassGenerics(in_func, std::string &name,
                          ClassDeclaration *classDeclaration) {
	auto it =
	    context.defaultClassMap.find(classDeclaration->baseClassLexerStringId);
	if (it == context.defaultClassMap.end()) {
		std::string targetName =
		    context.lexerString[classDeclaration->baseClassLexerStringId];
		std::string bestSuggestion;
		double bestScore = 0.0;
		auto checkSuggestion = [&](const std::string &candidate) {
			double score = rapidfuzz::fuzz::ratio(targetName, candidate);
			if (score > bestScore && score >= 60.0) {
				bestScore = score;
				bestSuggestion = candidate;
			}
		};
		for (const auto &pair : context.defaultClassMap) {
			checkSuggestion(context.lexerString[pair.first]);
		}
		for (const auto &clazz : compile.classes) {
			if (clazz) {
				checkSuggestion(clazz->getName(compile));
			}
		}

		std::string errorMsg =
		    "Cannot find class name '" + targetName + "'";
		if (!bestSuggestion.empty() && bestSuggestion != targetName) {
			errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
		}
		errorMsg +=
		    "\nHint: Ensure base class for generic instantiation is defined "
		    "and imported.";
		classDeclaration->throwError(errorMsg);
	}
	ClassId classId = it->second;
	auto clazz = compile.classes[it->second];
	auto classInfo = context.classInfo[it->second];
	{
		auto it = compile.classMap.find(name);
		if (it != compile.classMap.end()) {
			return it->second;
		}
	}
	if (!classInfo->genericData) {
		classDeclaration->throwError(
		    clazz->getName(compile) +
		    " is not generic\nHint: Do not pass type parameters '<...>' to a non-generic "
		    "class");
	}
	auto baseCreateClassNode = context.findCreateClassNode(clazz->id);
	LexerStringId newNameId = context.createLexerStringIfNotExists(name);

	auto newCreateClassNode = context.newClasses.push(
	    baseCreateClassNode->line, newNameId, baseCreateClassNode->classFlags);
	newCreateClassNode->pushClass(in_data);
	// std::cerr << "CLASS " << context.lexerString[newNameId] << "\n";
	newCreateClassNode->superDeclaration =
	    baseCreateClassNode->superDeclaration;
	auto newClassId = newCreateClassNode->classId;
	context.defaultClassMap[newNameId] = newClassId;
	context.newDefaultClassesMap[newClassId] = newCreateClassNode;

	std::vector<ClassDeclaration *> genericTypeId;
	genericTypeId.reserve(classInfo->genericData->genericDeclarations.size());

	// Snapshot state của GenericDeclarationNode để restore sau khi dùng xong
	struct GenericDeclSnapshot {
		ClassId classId;
		bool nullable;
	};
	std::vector<GenericDeclSnapshot> genericDeclSnapshots;
	genericDeclSnapshots.reserve(classInfo->genericData->genericDeclarations.size());

	for (size_t i = 0; i < classInfo->genericData->genericDeclarations.size();
	     ++i) {
		auto &genericDeclaration =
		    classInfo->genericData->genericDeclarations[i];
		auto &inputClass = classDeclaration->inputClassId[i];

		ClassId inputClassId = *inputClass->classId;

		// Lưu snapshot trước khi mutate
		genericDeclSnapshots.emplace_back(
		    GenericDeclSnapshot{genericDeclaration->classId, genericDeclaration->nullable});

		// Change callnode name
		if (!genericDeclaration->allCallNodes.empty()) {
			const std::string &name =
			    compile.classes[inputClassId]->getName(compile);
			auto nameId = context.createLexerStringIfNotExists(name);
			for (auto *callNode : genericDeclaration->allCallNodes) {
				callNode->nameId = nameId;
			}
		}
		// Change generics type
		genericDeclaration->classId = inputClassId;
		genericDeclaration->nullable = inputClass->nullable;
		ClassDeclaration *newClassDeclaration;
		if (inputClass->isGeneric) {
			newClassDeclaration = context.classDeclarationAllocator.push();
			newClassDeclaration->classId = inputClassId;
			newClassDeclaration->nullable = inputClass->nullable;
			newClassDeclaration->line = genericDeclaration->line;
			if (inputClassId == DefaultClass::functionClassId) {
				newClassDeclaration->inputClassId.reserve(
				    inputClass->inputClassId.size());
				for (auto classDeclaration : inputClass->inputClassId) {
					newClassDeclaration->inputClassId.push_back(
					    classDeclaration->copy(in_data));
				}
			}
		} else {
			newClassDeclaration = inputClass;
		}

		genericTypeId.push_back(newClassDeclaration);

		if (genericDeclaration->condition) {
			auto &condition = *genericDeclaration->condition;
			// if (condition.condition ==
			// GenericDeclarationCondition::MUST_EXTENDS) {
			if (!condition.classDeclaration->classId) {
				condition.classDeclaration->template load<true>(in_data);
				if (!condition.classDeclaration->classId) {
					condition.classDeclaration->throwError(
					    "Unresolved " +
					    condition.classDeclaration->getName(in_data) +
					    "\nHint: Ensure type constraint class is defined or imported");
				}
			} else if (condition.classDeclaration->classId ==
			           DefaultClass::functionClassId) {
				condition.classDeclaration->template load<true>(in_data);
			}
			context.checkValidateExtends[genericDeclaration].push_back(
			    newClassDeclaration);
			// }
		}

		for (auto *classDeclaration :
		     genericDeclaration->allClassDeclarations) {
			classDeclaration->classId = inputClassId;
			classDeclaration->inputClassId = newClassDeclaration->inputClassId;
			if (classDeclaration->mustInference) {
				classDeclaration->nullable = inputClass->nullable;
				// classDeclaration->mustInference = false;
			}
			classDeclaration->baseClassLexerStringId =
			    inputClass->baseClassLexerStringId;
		}
	}

	for (auto &[classDeclaration, node] :
	     classInfo->genericData->mustRenameNodes) {
		if (!classDeclaration->classId) {
			if (node->kind == NodeType::CALL &&
			    static_cast<CallNode *>(node)->caller == nullptr &&
			    context.defaultClassMap.find(
			        classDeclaration->baseClassLexerStringId) ==
			        context.defaultClassMap.end()) {
				classDeclaration->template load<false, true, isLazy, true>(in_data);
				if (!classDeclaration->classId) {
					classDeclaration->template load<true, true, isLazy, true>(in_data);
				}
			} else {
				classDeclaration->template load<false, true, isLazy, false>(in_data);
				if (!classDeclaration->classId) {
					classDeclaration->template load<true, true, isLazy, false>(in_data);
				}
			}
			if (!classDeclaration->classId && !classDeclaration->isFunction) {
				classDeclaration->throwError(
				    "Unsolved " + classDeclaration->getName(in_data) +
				    "\nHint: Ensure type parameter or class is defined");
			}
		}
		auto name = classDeclaration->getName(in_data);
		switch (node->kind) {
			case NodeType::UNKNOW: {
				auto unknowNode = static_cast<UnknowNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError(
					    "Unsolved " + name +
					    "\nHint: Symbol or type name cannot be resolved in "
					    "current scope");
				}
				unknowNode->nameId = it->second;
				break;
			}
			case NodeType::CALL: {
				auto callNode = static_cast<CallNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError(
					    "Unsolved " + name +
					    "\nHint: Called symbol or generic type cannot be resolved");
				}
				callNode->nameId = it->second;
				break;
			}
			default:
				break;
		}
		// Reset: classId và isFunction để tránh state leak sang instantiation tiếp theo
		classDeclaration->classId = std::nullopt;
		classDeclaration->isFunction = false;
	}

	auto newClass = compile.classes[newClassId];
	newClass->memberMap = clazz->memberMap;
	auto newClassInfo = context.classInfo[newClassId];
	newClassInfo->memberMap = classInfo->memberMap;
	newClass->genericType.offset = compile.allGenericType.size();
	newClass->genericType.size = genericTypeId.size();
	newClass->genericBaseClassId = classId;
	for (auto genericType : genericTypeId) {
		compile.allGenericType.push_back(*genericType->classId);
		compile.allGenericTypeNullable.push_back(genericType->nullable);
	}
	newClassInfo->genericTypeId = std::move(genericTypeId);
	newClassInfo->declarationThis = context.declarationNodePool.push(
	    classInfo->declarationThis->line, newClassId, lexerIdthis, "this",
	    nullptr, true, false, false);
	newClassInfo->declarationThis->id = 0;
	newClassInfo->declarationThis->classId = newClassId;
	auto lastCurrentClassId = context.currentClassId;
	context.currentClassId = newClassId;

	// std::cerr << "Created " << newClass->getName(compile) << "\n ";

	if (newCreateClassNode->superDeclaration &&
	    !newCreateClassNode->superDeclaration->classId) {
		newCreateClassNode->superDeclaration->template load<true>(in_data);
		// std::cerr << "Created "
		//           << newCreateClassNode->superDeclaration->getName(in_data)
		//           << "\n ";
	}

	for (auto *member : classInfo->member) {
		if (member->classDeclaration) {
			if (!member->classDeclaration->classId) {
				member->classDeclaration->template load<true>(in_data);
				if (!member->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Unsolved " +
					    member->classDeclaration->getName(in_data) +
					    "\nHint: Ensure member variable type is defined or imported");
				}
			}
			member->classId = *member->classDeclaration->classId;
			member->nullable =
			    member->nullable || member->classDeclaration->nullable;
			if (!member->classDeclaration->isGenerics(in_data)) {
				newClassInfo->member.push_back(member);
				continue;
			}
			// std::cerr
			//     << "Member declaration: "
			//     <<
			//     compile.classes[*member->classDeclaration->classId]->getName(compile)
			//     << "\n";
		} else {
			// std::cerr << "No" << "\n";
		}
		newClassInfo->member.push_back(
		    static_cast<DeclarationNode *>(member->copy(in_data)));
	}

	newClassInfo->declarationThis->classId = newClassId;
	// newClassInfo->func = classInfo->func;
	newClassInfo->staticFunc = classInfo->staticFunc;
	newClass->parentId = clazz->parentId;

	// newClass->funcMap = clazz->funcMap;

	ParserContext::mode = baseCreateClassNode->mode;

	for (auto *declarationNode : classInfo->allDeclarationNode) {
		if (declarationNode->classDeclaration) {
			if (!declarationNode->classDeclaration->classId) {
				declarationNode->classDeclaration->template load<false>(in_data);
				if (!declarationNode->classDeclaration->classId) {
					declarationNode->classDeclaration->template load<true>(in_data);
				}
				if (!declarationNode->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot find class name " +
					    declarationNode->classDeclaration->getName(in_data) +
					    "\nHint: Check variable declaration type in generic class");
				}
				// std::cerr << "loaded "
				//           <<
				//           declarationNode->classDeclaration->getName(in_data)
				//           << "\n";
				declarationNode->optimize(in_data);
				declarationNode->classDeclaration->classId = std::nullopt;
				// declarationNode->classDeclaration = nullptr;
				continue;
			}
		}
		declarationNode->optimize(in_data);
		// declarationNode->classDeclaration = nullptr;
	}

	for (auto &[declarationNode, value] :
	     classInfo->genericData->staticDeclaration) {
		auto node = context.makeDeclarationNode(
		    in_data, declarationNode->line, declarationNode->baseName,
		    newClass->getName(compile) + "." +
		        context.lexerString[declarationNode->baseName],
		    declarationNode->classDeclaration, declarationNode->isVal, true,
		    declarationNode->nullable, false, true);
		newClassInfo->staticMember[declarationNode->baseName] = node;
		if (node->classDeclaration) {
			if (!node->classDeclaration->classId) {
				node->classDeclaration->template load<false, false, isLazy>(in_data);
				if (!node->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot find class name " +
					    node->classDeclaration->getName(in_data) +
					    "\nHint: Check static member type in generic class");
				}
				node->optimize(in_data);
				node->classDeclaration->classId = std::nullopt;
				// declarationNode->classDeclaration = nullptr;
			} else {
				node->classId = *node->classDeclaration->classId;
			}
			// std::cerr << "loaded " <<
			// (compile.classes[node->classId]->getName(compile))
			//           << " "
			//           << declarationNode->classDeclaration->getName(in_data)
			//           << "\n";
		}
		node->optimize(in_data);
		if (value) {
			auto varNode =
			    context.varPool.push(declarationNode->line, node, false, true);
			auto setNode = context.setValuePool.push(declarationNode->line,
			                                         varNode, value, true);
			context.staticNode.push_back(setNode);
			// std::cerr << "Added " << node->getName(compile) << "\n";
		}
		// declarationNode->classDeclaration = nullptr;
	}

	newCreateClassNode->body.nodes.reserve(
	    baseCreateClassNode->body.nodes.size());
	for (auto *node : baseCreateClassNode->body.nodes) {
		newCreateClassNode->body.nodes.push_back(node->copy(in_data));
	}

	if (classInfo->primaryConstructor) {
		auto constructor = context.createConstructorPool.push(
		    classInfo->primaryConstructor->line, newClassId, newNameId,
		    classInfo->primaryConstructor->parameter->copy(in_data), true,
		    classInfo->primaryConstructor->functionFlags);
		newClassInfo->primaryConstructor = constructor;
		constructor->pushFunction(in_data);
		constructor->optimize(in_data);
	} else {
		newClassInfo->secondaryConstructor.reserve(
		    classInfo->secondaryConstructor.size());
		for (auto *constructor : classInfo->secondaryConstructor) {
			auto newConstructor = context.createConstructorPool.push(
			    constructor->line, newClassId, newNameId,
			    constructor->parameter->copy(in_data), false,
			    constructor->functionFlags);
			newClassInfo->secondaryConstructor.push_back(newConstructor);
			newConstructor->pushFunction(in_data);
			// ParserContext::mode = constructor->mode;
			auto lastCurrentFunctionId = context.currentFunctionId;
			context.gotoFunction(newConstructor->funcId);
			newConstructor->body.nodes.reserve(constructor->body.nodes.size());
			for (auto &[declarationNode, value] :
			     context.functionInfo[constructor->funcId]
			         ->reflectDeclarationMap) {
				value = static_cast<DeclarationNode *>(
				    declarationNode->copy(in_data));
			}
			for (auto *node : constructor->body.nodes) {
				newConstructor->body.nodes.push_back(node->copy(in_data));
			}
			context.gotoFunction(lastCurrentFunctionId);
			if constexpr (isLazy) {
				constructor->optimize(in_data);
			}
		}
	}

	auto lastCurrentFunctionId = context.currentFunctionId;

	for (auto *createFuncNode : classInfo->createFunctionNodes) {
		auto funcInfo = context.functionInfo[createFuncNode->id];
		auto paramCopy = createFuncNode->parameter->copy(in_data);
		if (!paramCopy->parameters.empty() &&
		    paramCopy->parameters[0]->baseName == lexerIdthis) {
			paramCopy->parameters[0] = newClassInfo->declarationThis;
		}
		if (funcInfo->genericData != nullptr) {
			// Vấn đề 4: Lưu và restore currentClassId khi tạo generic function node
			auto savedClassId = context.currentClassId;
			context.currentClassId = newClassId;
			auto newCreateFuncNode = context.newFunctions.push(
			    createFuncNode->line, createFuncNode->tokenIndex, newClassId,
			    createFuncNode->nameId == lexerId__CLASS__ ? newNameId
			                                               : createFuncNode->nameId,
			    nullptr, paramCopy,
			    createFuncNode->functionFlags | FunctionFlags::FUNC_SKIP_LOAD);
			newCreateFuncNode->pushFunction(in_data);
			auto newFuncInfo = context.functionInfo[newCreateFuncNode->id];
			newFuncInfo->genericData = funcInfo->genericData;
			newFuncInfo->body = funcInfo->body;
			newCreateFuncNode->classDeclaration = createFuncNode->classDeclaration;
			compile.functions[newCreateFuncNode->id]->maxDeclaration =
			    compile.functions[createFuncNode->id]->maxDeclaration;
			newFuncInfo->declaration = funcInfo->declaration;
				
			// Đồng bộ reflectDeclarationMap của clone generic function trong class,
			// để class-scoped member declaration không bị thiếu khi later compile
			// generic function bodies from this generic class instance.
			if (newClassInfo->declarationThis) {
				newFuncInfo->reflectDeclarationMap[newClassInfo->declarationThis] =
				    newClassInfo->declarationThis;
			}
			for (auto *memberDecl : newClassInfo->member) {
				if (memberDecl &&
				    newFuncInfo->reflectDeclarationMap.find(memberDecl) ==
				        newFuncInfo->reflectDeclarationMap.end()) {
					newFuncInfo->reflectDeclarationMap[memberDecl] = memberDecl;
				}
			}

			for (auto *param : paramCopy->parameters) {
				if (!param || !param->classDeclaration) continue;
				auto addClassDeclToGen = [&](auto &self, ClassDeclaration *cd) -> void {
					if (!cd) return;
					if (cd->isGenericDeclaration) {
						for (auto &genDecl : newFuncInfo->genericData->genericDeclarations) {
							if (cd->baseClassLexerStringId == genDecl->nameId) {
								genDecl->allClassDeclarations.push_back(cd);
								break;
							}
						}
					}
					for (auto *child : cd->inputClassId) {
						self(self, child);
					}
				};
				addClassDeclToGen(addClassDeclToGen, param->classDeclaration);
			}

			newClassInfo->genericFunctionMap[createFuncNode->nameId].push_back(
			    newCreateFuncNode);
			newClassInfo->createFunctionNodes.push_back(newCreateFuncNode);
			context.currentClassId = savedClassId;
			continue;
		}
		auto newCreateFuncNode = context.newFunctions.push(
		    createFuncNode->line, createFuncNode->tokenIndex, newClassId,
		    createFuncNode->nameId == lexerId__CLASS__ ? newNameId
		                                               : createFuncNode->nameId,
		    nullptr, paramCopy,
		    createFuncNode->functionFlags & ~FunctionFlags::FUNC_SKIP_LOAD);
		if (createFuncNode->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			newCreateFuncNode->pushNativeFunction(
			    in_data, compile.functions[createFuncNode->id]->native);
		} else {
			newCreateFuncNode->pushFunction(in_data);
		}
		auto newFunc = compile.functions[newCreateFuncNode->id];
		auto newFuncInfo = context.functionInfo[newCreateFuncNode->id];
		newFunc->returnId = compile.functions[createFuncNode->id]->returnId;
		if (createFuncNode->classDeclaration) {
			// Vấn đề 3: Reset cây classDeclaration trước khi load để tránh state cũ từ lần instantiate trước
			resetClassDeclTree(createFuncNode->classDeclaration);
			if (!createFuncNode->classDeclaration->classId) {
				createFuncNode->classDeclaration->template load<true>(in_data);
				if (!createFuncNode->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot resolve return type of generic function\nHint: "
					    "Ensure generic function return type is valid and resolved");
				}
				newFunc->returnId = *createFuncNode->classDeclaration->classId;
			} else {
				newFunc->returnId = *createFuncNode->classDeclaration->classId;
			}

			if (newFunc->returnId == DefaultClass::functionClassId) {
				newFuncInfo->returnClass =
				    createFuncNode->classDeclaration->copy(in_data);
			}
			resetClassDeclTree(createFuncNode->classDeclaration);
		}
		context.gotoFunction(newCreateFuncNode->id);
		// ParserContext::mode = createFuncNode->mode; //Loaded in new class
		newFuncInfo->body.nodes.reserve(funcInfo->body.nodes.size());
		for (auto &[declarationNode, value] : funcInfo->reflectDeclarationMap) {
			value =
			    static_cast<DeclarationNode *>(declarationNode->copy(in_data));
		}
		for (auto *node : funcInfo->body.nodes) {
			newFuncInfo->body.nodes.push_back(node->copy(in_data));
		}
		if (funcInfo->inferenceNode) {
			newFuncInfo->inferenceNode =
			    static_cast<ReturnNode *>(newFuncInfo->body.nodes[0]);
			newFuncInfo->inferenceNode->loaded = false;
			context.mustInferenceFunctionType.push_back(newFunc->id);
		}

		if constexpr (isLazy) {
			newCreateFuncNode->optimize(in_data);
		}
	}

	context.gotoFunction(lastCurrentFunctionId);
	context.currentClassId = lastCurrentClassId;

	// Restore snapshot của GenericDeclarationNode để sẵn sàng cho instantiation tiếp theo
	for (size_t i = 0; i < classInfo->genericData->genericDeclarations.size(); ++i) {
		auto &genericDeclaration = classInfo->genericData->genericDeclarations[i];
		genericDeclaration->classId = genericDeclSnapshots[i].classId;
		genericDeclaration->nullable = genericDeclSnapshots[i].nullable;
	}

	if constexpr (isLazy) {
		newCreateClassNode->optimize(in_data);
	}

	// std::cerr << "Created " << newClass->getName(compile) << "\n";
	return newClassId;
}

template ClassId loadClassGenerics<false>(in_func, std::string &name,
                                         ClassDeclaration *classDeclaration);
template ClassId loadClassGenerics<true>(in_func, std::string &name,
                                        ClassDeclaration *classDeclaration);


} // namespace Autolang
#endif
