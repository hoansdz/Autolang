#ifndef DEBUGGER_GENERIC_FUNCTION_CPP
#define DEBUGGER_GENERIC_FUNCTION_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

struct ClassDeclSnapshot {
	ClassDeclaration *cd;
	std::optional<ClassId> classId;
	bool nullable;
	LexerStringId baseClassLexerStringId;
	std::vector<ClassDeclaration *> inputClassId;
};

struct GenDeclSnapshot {
	GenericDeclarationNode *decl;
	ClassId classId;
	bool nullable;
	std::vector<ClassDeclSnapshot> cdSnapshots;
};

void loadFunctionGenerics(in_func, std::string &name,
                          ClassDeclaration *classDeclaration) {
	auto it = context.genericFunctionMap.find(
	    classDeclaration->baseClassLexerStringId);
	if (it == context.genericFunctionMap.end()) {
		classDeclaration->throwError(
		    "Bug: Cannot find function " +
		    context.lexerString[classDeclaration->baseClassLexerStringId] +
		    "\nHint: Ensure generic function is defined before instantiation");
	}
	{
		auto it = compile.funcMap.find(name);
		if (it != compile.funcMap.end()) {
			return;
		}
	}

	auto &allCreateFuncNode = it->second;

	for (auto createFuncNode : allCreateFuncNode) {
		FunctionId funcId = createFuncNode->id;
		auto func = compile.functions[funcId];
		auto funcInfo = context.functionInfo[funcId];
		if (!funcInfo->genericData) {
			classDeclaration->throwError(
			    func->getName(compile) +
			    " is not generic\nHint: Do not pass type arguments '<...>' to a non-generic "
			    "function");
		}
		if (classDeclaration->inputClassId.size() !=
		    funcInfo->genericData->genericDeclarations.size()) {
			continue;
		}
		auto allCreateFuncNode =
		    context
		        .genericFunctionMap[classDeclaration->baseClassLexerStringId];

		ParserContext::mode = createFuncNode->mode;

		std::vector<ClassDeclaration *> genericTypeId;
		genericTypeId.reserve(
		    funcInfo->genericData->genericDeclarations.size());

		// Snapshot state of GenericDeclarationNode (loadFunctionGenerics)
		std::vector<GenDeclSnapshot> funcGenDeclSnapshots;
		funcGenDeclSnapshots.reserve(
		    funcInfo->genericData->genericDeclarations.size());

		for (size_t i = 0;
		     i < funcInfo->genericData->genericDeclarations.size(); ++i) {
			auto &genericDeclaration =
			    funcInfo->genericData->genericDeclarations[i];
			auto &inputClass = classDeclaration->inputClassId[i];

			ClassId inputClassId = *inputClass->classId;

			// Save snapshot before mutating
			GenDeclSnapshot snap;
			snap.decl = genericDeclaration;
			snap.classId = genericDeclaration->classId;
			snap.nullable = genericDeclaration->nullable;
			snap.cdSnapshots.reserve(genericDeclaration->allClassDeclarations.size());
			for (auto *cd : genericDeclaration->allClassDeclarations) {
				if (cd) {
					snap.cdSnapshots.push_back({cd, cd->classId, cd->nullable, cd->baseClassLexerStringId, cd->inputClassId});
				}
			}
			funcGenDeclSnapshots.push_back(std::move(snap));

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
						    "\nHint: Ensure generic constraint class is defined or imported");
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
				if (classDeclaration->mustInference) {
					classDeclaration->nullable = inputClass->nullable;
					// classDeclaration->mustInference = false;
				}
				classDeclaration->inputClassId =
				    newClassDeclaration->inputClassId;
			}
		}

		auto functionFlags =
		    createFuncNode->functionFlags & ~(FunctionFlags::FUNC_SKIP_LOAD);
		auto lastCurrentFunctionId = context.currentFunctionId;
		auto newCreateFuncNode = context.newFunctions.push(
		    createFuncNode->line, createFuncNode->tokenIndex,
		    createFuncNode->contextCallClassId,
		    context.createLexerStringIfNotExists(name), nullptr,
		    funcInfo->parameter->copy(in_data), functionFlags);

		if (createFuncNode->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			newCreateFuncNode->pushNativeFunction(
			    in_data, compile.functions[createFuncNode->id]->native);
		} else {
			newCreateFuncNode->pushFunction(in_data);
		}
		auto newFunc = compile.functions[newCreateFuncNode->id];
		auto newFuncInfo = context.functionInfo[newCreateFuncNode->id];
		newFunc->maxDeclaration = func->maxDeclaration;
		newFuncInfo->declaration = funcInfo->declaration;
		newFunc->returnId = compile.functions[funcId]->returnId;
		context.gotoFunction(newCreateFuncNode->id);

		for (auto &[classDeclaration, node] :
		     funcInfo->genericData->mustRenameNodes) {
			if (!classDeclaration->classId) {
				if (node->kind == NodeType::CALL &&
				    static_cast<CallNode *>(node)->caller == nullptr &&
				    context.defaultClassMap.find(
				        classDeclaration->baseClassLexerStringId) ==
				        context.defaultClassMap.end()) {
					classDeclaration->template load<false, true, false, true>(in_data);
					if (!classDeclaration->classId) {
						classDeclaration->template load<true, true, false, true>(in_data);
					}
				} else {
					classDeclaration->template load<false, true, false, false>(in_data);
					if (!classDeclaration->classId) {
						classDeclaration->template load<true, true, false, false>(in_data);
					}
				}
				if (!classDeclaration->classId && !classDeclaration->isFunction) {
					classDeclaration->throwError(
					    "Unsolved " + classDeclaration->getName(in_data) +
					    "\nHint: Ensure type parameter or class is defined");
				}
			}
			auto targetNodeName = classDeclaration->getName(in_data);
			// std::cerr << targetNodeName << "\n";
			switch (node->kind) {
				case NodeType::UNKNOW: {
					auto unknowNode = static_cast<UnknowNode *>(node);
					auto it = context.lexerStringMap.find(targetNodeName);
					if (it == context.lexerStringMap.end()) {
						classDeclaration->throwError(
						    "Unsolved " + targetNodeName +
						    "\nHint: Symbol or type name cannot be resolved");
					}
					unknowNode->nameId = it->second;
					break;
				}
				case NodeType::CALL: {
					auto callNode = static_cast<CallNode *>(node);
					auto it = context.lexerStringMap.find(targetNodeName);
					if (it == context.lexerStringMap.end()) {
						classDeclaration->throwError(
						    "Unsolved " + targetNodeName +
						    "\nHint: Called symbol or type cannot be resolved");
					}
					callNode->nameId = it->second;
					break;
				}
				default:
					break;
			}
			// Reset classId and isFunction to prevent state leakage
			classDeclaration->classId = std::nullopt;
			classDeclaration->isFunction = false;
		}
		// std::cerr << "FUNC " << newFunc->getName(compile) << "\n";
		if (createFuncNode->classDeclaration) {
			// Issue 3: Reset tree before loading return type (loadFunctionGenerics)
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

		newFuncInfo->reflectDeclarationMap.reserve(
		    funcInfo->reflectDeclarationMap.size() +
		    newCreateFuncNode->parameter->parameters.size());

		for (size_t p = 0; p < newCreateFuncNode->parameter->parameters.size() &&
		                   p < funcInfo->parameter->parameters.size();
		     ++p) {
			newFuncInfo->reflectDeclarationMap[funcInfo->parameter->parameters[p]] =
			    newCreateFuncNode->parameter->parameters[p];
		}

		for (auto &[declarationNode, value] : funcInfo->reflectDeclarationMap) {
			newFuncInfo->reflectDeclarationMap[declarationNode] =
			    static_cast<DeclarationNode *>(declarationNode->copy(in_data));
		}

		for (auto &[declarationNode, value] :
		     funcInfo->genericData->staticDeclaration) {
			auto node = context.makeDeclarationNode(
			    in_data, declarationNode->line, declarationNode->baseName,
			    newFunc->getName(compile) +
			        context.lexerString[declarationNode->baseName],
			    declarationNode->classDeclaration, declarationNode->isVal, true,
			    declarationNode->nullable, false, true);
			funcInfo->genericData
			    ->newPositionOfStaticDeclaration[declarationNode->id] =
			    node->id;
			if (node->classDeclaration) {
				if (!node->classDeclaration->classId) {
					node->classDeclaration->template load<false>(in_data);
					if (!node->classDeclaration->classId) {
						classDeclaration->throwError(
						    "Bug: Cannot find class name " +
						    node->classDeclaration->getName(in_data) +
						    "\nHint: Check static variable type in generic function");
					}
					// std::cerr << "loaded "
					//           <<
					//           declarationNode->classDeclaration->getName(in_data)
					//           << "\n";
					node->optimize(in_data);
					node->classDeclaration->classId = std::nullopt;
				} else {
					node->classId = *node->classDeclaration->classId;
				}
			}
			node->optimize(in_data);
			if (value) {
				auto varNode = context.varPool.push(declarationNode->line, node,
				                                    false, true);
				auto setNode = context.setValuePool.push(declarationNode->line,
				                                         varNode, value, true);
				context.staticNode.push_back(setNode);
			}
			// declarationNode->classDeclaration = nullptr;
		}

		// ParserContext::mode = createFuncNode->mode; //Loaded in new class
		auto lastNewPositionOfStaticDeclaration =
		    context.newPositionOfStaticDeclaration;
		context.newPositionOfStaticDeclaration =
		    &funcInfo->genericData->newPositionOfStaticDeclaration;
		newFuncInfo->body.nodes.reserve(funcInfo->body.nodes.size());
		for (auto *node : funcInfo->body.nodes) {
			newFuncInfo->body.nodes.push_back(node->copy(in_data));
		}
		if (funcInfo->inferenceNode) {
			newFuncInfo->inferenceNode =
			    static_cast<ReturnNode *>(newFuncInfo->body.nodes[0]);
			context.mustInferenceFunctionType.push_back(newFunc->id);
		}
		context.newPositionOfStaticDeclaration =
		    lastNewPositionOfStaticDeclaration;
		newCreateFuncNode->optimize(in_data);
		context.gotoFunction(lastCurrentFunctionId);

		// Restore snapshot of GenericDeclarationNode (loadFunctionGenerics)
		for (auto &snap : funcGenDeclSnapshots) {
			snap.decl->classId = snap.classId;
			snap.decl->nullable = snap.nullable;
			for (auto &cdSnap : snap.cdSnapshots) {
				cdSnap.cd->classId = cdSnap.classId;
				cdSnap.cd->nullable = cdSnap.nullable;
				cdSnap.cd->baseClassLexerStringId = cdSnap.baseClassLexerStringId;
				cdSnap.cd->inputClassId = cdSnap.inputClassId;
			}
		}

		// for (auto &[classDeclaration, node] :
		//      funcInfo->genericData->mustRenameNodes) {
		// 	if (!classDeclaration) {
		// 		classDeclaration->template load<false>(in_data);
		// 		if (!classDeclaration->classId) {
		// 			classDeclaration->throwError(
		// 			    "Unsolved " + classDeclaration->getName(in_data));
		// 		}
		// 	}
		// 	switch (node->kind) {
		// 		case NodeType::UNKNOW: {
		// 			auto unknowNode = static_cast<UnknowNode *>(node);
		// 			auto it = context.lexerStringMap.find(
		// 			    classDeclaration->getName(in_data));
		// 			if (it == context.lexerStringMap.end()) {
		// 				classDeclaration->throwError(
		// 				    "Unsolved " + classDeclaration->getName(in_data));
		// 			}
		// 			unknowNode->nameId = it->second;
		// 			break;
		// 		}
		// 		case NodeType::CALL: {
		// 			auto callNode = static_cast<CallNode *>(node);
		// 			auto it = context.lexerStringMap.find(
		// 			    classDeclaration->getName(in_data));
		// 			if (it == context.lexerStringMap.end()) {
		// 				classDeclaration->throwError(
		// 				    "Unsolved " + classDeclaration->getName(in_data));
		// 			}
		// 			callNode->nameId = it->second;
		// 			break;
		// 		}
		// 	}
		// 	classDeclaration->classId = std::nullopt;
		// }
	}
}

void loadMemberFunctionGenerics(in_func, ClassId callerClassId,
                                const std::string &name,
                                ClassDeclaration *classDeclaration,
                                LexerStringId baseNameId) {
	auto callerClassInfo = context.classInfo[callerClassId];
	auto nameId = context.createLexerStringIfNotExists(name);
	{
		auto it = callerClassInfo->allFunction.find(nameId);
		if (it != callerClassInfo->allFunction.end()) {
			return;
		}
	}

	auto it = callerClassInfo->genericFunctionMap.find(baseNameId);
	if (it == callerClassInfo->genericFunctionMap.end()) {
		auto callerClass = compile.classes[callerClassId];
		if (callerClass && callerClass->genericBaseClassId != 0) {
			auto baseClassInfo =
			    context.classInfo[callerClass->genericBaseClassId];
			it = baseClassInfo->genericFunctionMap.find(baseNameId);
		}
	}

	if (it == callerClassInfo->genericFunctionMap.end()) {
		classDeclaration->throwError(
		    "Bug: Cannot find member function " +
		    context.lexerString[baseNameId] +
		    "\nHint: Ensure generic member function is defined before instantiation");
	}

	auto &allCreateFuncNode = it->second;

	for (auto createFuncNode : allCreateFuncNode) {
		auto resolvedCreateFuncNode = createFuncNode;
		FunctionId resolvedFuncId = createFuncNode->id;
		auto resolvedFunc = compile.functions[resolvedFuncId];
		auto resolvedFuncInfo = context.functionInfo[resolvedFuncId];

		if ((createFuncNode->functionFlags & FunctionFlags::FUNC_SKIP_LOAD) &&
		    resolvedFuncInfo->body.nodes.empty()) {
			auto callerClass = compile.classes[callerClassId];
			if (callerClass && callerClass->genericBaseClassId != 0) {
				auto baseClassInfo =
				    context.classInfo[callerClass->genericBaseClassId];
				auto baseIt = baseClassInfo->genericFunctionMap.find(baseNameId);
				if (baseIt != baseClassInfo->genericFunctionMap.end()) {
					for (auto baseFuncNode : baseIt->second) {
						auto baseFuncInfo = context.functionInfo[baseFuncNode->id];
						if (baseFuncInfo->genericData &&
						    baseFuncInfo->genericData->genericDeclarations.size() ==
						        classDeclaration->inputClassId.size()) {
							resolvedCreateFuncNode = baseFuncNode;
							resolvedFuncId = baseFuncNode->id;
							resolvedFunc = compile.functions[resolvedFuncId];
							resolvedFuncInfo = context.functionInfo[resolvedFuncId];
							break;
						}
					}
				}
			}
		}

		FunctionId funcId = resolvedFuncId;
		auto func = resolvedFunc;
		auto funcInfo = resolvedFuncInfo;
		if (!funcInfo->genericData) {
			classDeclaration->throwError(
			    func->getName(compile) +
			    " is not generic\nHint: Do not pass type arguments '<...>' to a non-generic "
			    "function");
		}
		if (classDeclaration->inputClassId.size() !=
		    funcInfo->genericData->genericDeclarations.size()) {
			continue;
		}

		ParserContext::mode = resolvedCreateFuncNode->mode;

		std::vector<ClassDeclaration *> genericTypeId;
		genericTypeId.reserve(
		    funcInfo->genericData->genericDeclarations.size());

		// Snapshot state of GenericDeclarationNode (loadMemberFunctionGenerics)
		std::vector<GenDeclSnapshot> memberGenDeclSnapshots;
		memberGenDeclSnapshots.reserve(
		    funcInfo->genericData->genericDeclarations.size());

		std::vector<GenDeclSnapshot> classGenDeclSnapshots;
		auto callerClass = compile.classes[callerClassId];
		if (callerClass && callerClass->genericBaseClassId != 0) {
			auto baseClassInfo = context.classInfo[callerClass->genericBaseClassId];
			if (baseClassInfo && baseClassInfo->genericData) {
				for (size_t ci = 0; ci < baseClassInfo->genericData->genericDeclarations.size() &&
				                    ci < callerClassInfo->genericTypeId.size(); ++ci) {
					auto *classGenDecl = baseClassInfo->genericData->genericDeclarations[ci];
					auto *typeDecl = callerClassInfo->genericTypeId[ci];
					if (typeDecl && typeDecl->classId) {
						GenDeclSnapshot snap;
						snap.decl = classGenDecl;
						snap.classId = classGenDecl->classId;
						snap.nullable = classGenDecl->nullable;
						snap.cdSnapshots.reserve(classGenDecl->allClassDeclarations.size());
						for (auto *cd : classGenDecl->allClassDeclarations) {
							if (cd) {
								snap.cdSnapshots.push_back({cd, cd->classId, cd->nullable, cd->baseClassLexerStringId, cd->inputClassId});
							}
						}
						classGenDeclSnapshots.push_back(std::move(snap));
						classGenDecl->classId = *typeDecl->classId;
						classGenDecl->nullable = typeDecl->nullable;
						for (auto *cd : classGenDecl->allClassDeclarations) {
							cd->classId = *typeDecl->classId;
							cd->inputClassId = typeDecl->inputClassId;
							cd->baseClassLexerStringId = typeDecl->baseClassLexerStringId;
						}
					}
				}
			}
		}

		for (size_t i = 0;
		     i < funcInfo->genericData->genericDeclarations.size(); ++i) {
			auto &genericDeclaration =
			    funcInfo->genericData->genericDeclarations[i];
			auto &inputClass = classDeclaration->inputClassId[i];

			if (!inputClass->classId) {
				inputClass->template load<true>(in_data);
			}

			ClassId inputClassId = *inputClass->classId;

			// Save snapshot before mutating
			GenDeclSnapshot snap;
			snap.decl = genericDeclaration;
			snap.classId = genericDeclaration->classId;
			snap.nullable = genericDeclaration->nullable;
			snap.cdSnapshots.reserve(genericDeclaration->allClassDeclarations.size());
			for (auto *cd : genericDeclaration->allClassDeclarations) {
				if (cd) {
					snap.cdSnapshots.push_back({cd, cd->classId, cd->nullable, cd->baseClassLexerStringId, cd->inputClassId});
				}
			}
			memberGenDeclSnapshots.push_back(std::move(snap));

			// Change callnode name
			if (!genericDeclaration->allCallNodes.empty()) {
				const std::string &inputName =
				    compile.classes[inputClassId]->getName(compile);
				auto inputNameId = context.createLexerStringIfNotExists(inputName);
				for (auto *callNode : genericDeclaration->allCallNodes) {
					callNode->nameId = inputNameId;
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
					for (auto inputChildDecl : inputClass->inputClassId) {
						newClassDeclaration->inputClassId.push_back(
						    inputChildDecl->copy(in_data));
					}
				}
			} else {
				newClassDeclaration = inputClass;
			}

			genericTypeId.push_back(newClassDeclaration);

			if (genericDeclaration->condition) {
				auto &condition = *genericDeclaration->condition;
				if (!condition.classDeclaration->classId) {
					condition.classDeclaration->template load<true>(in_data);
					if (!condition.classDeclaration->classId) {
						condition.classDeclaration->throwError(
						    "Unresolved " +
						    condition.classDeclaration->getName(in_data) +
						    "\nHint: Ensure generic constraint class is defined or imported");
					}
				} else if (condition.classDeclaration->classId ==
				           DefaultClass::functionClassId) {
					condition.classDeclaration->template load<true>(in_data);
				}
				context.checkValidateExtends[genericDeclaration].push_back(
				    newClassDeclaration);
			}

			// Fix Issue 2: Patch allCallNodes of class-level generic declarations as well
			if (!genericDeclaration->allCallNodes.empty()) {
				const std::string &inputName =
				    compile.classes[inputClassId]->getName(compile);
				auto inputNameId = context.createLexerStringIfNotExists(inputName);
				for (auto *callNode : genericDeclaration->allCallNodes) {
					callNode->nameId = inputNameId;
				}
			}
			for (auto *cd : genericDeclaration->allClassDeclarations) {
				cd->classId = inputClassId;
				if (cd->mustInference) {
					cd->nullable = inputClass->nullable;
				}
				cd->inputClassId = newClassDeclaration->inputClassId;
			}
		}

		auto functionFlags =
		    createFuncNode->functionFlags & ~(FunctionFlags::FUNC_SKIP_LOAD);
		auto lastCurrentFunctionId = context.currentFunctionId;
		auto lastCurrentClassId = context.currentClassId;
		context.currentClassId = callerClassId;

		auto paramCopy = funcInfo->parameter->copy(in_data);
		if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
		    !paramCopy->parameters.empty() && callerClassInfo->declarationThis) {
			paramCopy->parameters[0] = callerClassInfo->declarationThis;
		}

		auto substituteGenerics = [&](auto &self, ClassDeclaration *cd) -> void {
			if (!cd) return;
			for (size_t gi = 0; gi < funcInfo->genericData->genericDeclarations.size(); ++gi) {
				auto &genDecl = funcInfo->genericData->genericDeclarations[gi];
				if (cd->isGenericDeclaration && cd->baseClassLexerStringId == genDecl->nameId) {
					auto &inputClass = classDeclaration->inputClassId[gi];
					cd->classId = *inputClass->classId;
					if (cd->mustInference) cd->nullable = inputClass->nullable;
					cd->inputClassId = genericTypeId[gi]->inputClassId;
					return;
				}
			}
			if (callerClass && callerClass->genericBaseClassId != 0) {
				auto baseClassInfo = context.classInfo[callerClass->genericBaseClassId];
				if (baseClassInfo && baseClassInfo->genericData) {
					for (size_t ci = 0; ci < baseClassInfo->genericData->genericDeclarations.size() &&
					                    ci < callerClassInfo->genericTypeId.size(); ++ci) {
						auto *classGenDecl = baseClassInfo->genericData->genericDeclarations[ci];
						if (cd->isGenericDeclaration && cd->baseClassLexerStringId == classGenDecl->nameId) {
							auto *typeDecl = callerClassInfo->genericTypeId[ci];
							if (typeDecl && typeDecl->classId) {
								cd->classId = *typeDecl->classId;
								if (cd->mustInference) cd->nullable = typeDecl->nullable;
								cd->inputClassId = typeDecl->inputClassId;
								return;
							}
						}
					}
				}
			}
			for (auto *child : cd->inputClassId) {
				self(self, child);
			}
		};
		for (auto *param : paramCopy->parameters) {
			if (param && param->classDeclaration) {
				substituteGenerics(substituteGenerics, param->classDeclaration);
			}
		}

		auto newCreateFuncNode = context.newFunctions.push(
		    resolvedCreateFuncNode->line, resolvedCreateFuncNode->tokenIndex,
		    callerClassId, nameId, nullptr,
		    paramCopy, functionFlags);

		if (resolvedCreateFuncNode->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			newCreateFuncNode->pushNativeFunction(
			    in_data, compile.functions[resolvedFuncId]->native);
		} else {
			newCreateFuncNode->pushFunction(in_data);
		}
		auto newFunc = compile.functions[newCreateFuncNode->id];
		auto newFuncInfo = context.functionInfo[newCreateFuncNode->id];
		newFunc->maxDeclaration = func->maxDeclaration;
		newFuncInfo->declaration = funcInfo->declaration;
		newFunc->returnId = compile.functions[resolvedFuncId]->returnId;
		context.gotoFunction(newCreateFuncNode->id);

		for (auto &[classDeclarationNode, node] :
		     funcInfo->genericData->mustRenameNodes) {
			if (!classDeclarationNode->classId) {
				if (node->kind == NodeType::CALL &&
				    static_cast<CallNode *>(node)->caller == nullptr &&
				    context.defaultClassMap.find(
				        classDeclarationNode->baseClassLexerStringId) ==
				        context.defaultClassMap.end()) {
					classDeclarationNode->template load<false, true, false, true>(in_data);
					if (!classDeclarationNode->classId) {
						classDeclarationNode->template load<true, true, false, true>(in_data);
					}
				} else {
					classDeclarationNode->template load<false, true, false, false>(in_data);
					if (!classDeclarationNode->classId) {
						classDeclarationNode->template load<true, true, false, false>(in_data);
					}
				}
				if (!classDeclarationNode->classId && !classDeclarationNode->isFunction) {
					classDeclarationNode->throwError(
					    "Unsolved " + classDeclarationNode->getName(in_data) +
					    "\nHint: Ensure type parameter or class is defined");
				}
			}
			auto targetNodeName = classDeclarationNode->getName(in_data);
			switch (node->kind) {
				case NodeType::UNKNOW: {
					auto unknowNode = static_cast<UnknowNode *>(node);
					auto itName = context.lexerStringMap.find(targetNodeName);
					if (itName == context.lexerStringMap.end()) {
						classDeclarationNode->throwError(
						    "Unsolved " + targetNodeName +
						    "\nHint: Symbol or type name cannot be resolved");
					}
					unknowNode->nameId = itName->second;
					break;
				}
				case NodeType::CALL: {
					auto callNode = static_cast<CallNode *>(node);
					auto itName = context.lexerStringMap.find(targetNodeName);
					if (itName == context.lexerStringMap.end()) {
						classDeclarationNode->throwError(
						    "Unsolved " + targetNodeName +
						    "\nHint: Called symbol or type cannot be resolved");
					}
					callNode->nameId = itName->second;
					break;
				}
				default:
					break;
			}
			// Reset classId and isFunction
			classDeclarationNode->classId = std::nullopt;
			classDeclarationNode->isFunction = false;
		}

		if (resolvedCreateFuncNode->classDeclaration) {
			resetClassDeclTree(resolvedCreateFuncNode->classDeclaration);

			if (!resolvedCreateFuncNode->classDeclaration->classId) {
				resolvedCreateFuncNode->classDeclaration->template load<true>(in_data);
				if (!resolvedCreateFuncNode->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot resolve return type of generic function\nHint: "
					    "Ensure generic function return type is valid and resolved");
				}
				newFunc->returnId = *resolvedCreateFuncNode->classDeclaration->classId;
			} else {
				newFunc->returnId = *resolvedCreateFuncNode->classDeclaration->classId;
			}

			if (newFunc->returnId == DefaultClass::functionClassId) {
				newFuncInfo->returnClass =
				    resolvedCreateFuncNode->classDeclaration->copy(in_data);
			}
			resetClassDeclTree(resolvedCreateFuncNode->classDeclaration);
		}

		newFuncInfo->reflectDeclarationMap.reserve(
		    funcInfo->reflectDeclarationMap.size() +
		    paramCopy->parameters.size() + 4);

		for (size_t p = 0; p < paramCopy->parameters.size() &&
		                   p < funcInfo->parameter->parameters.size();
		     ++p) {
			newFuncInfo->reflectDeclarationMap[funcInfo->parameter->parameters[p]] =
			    paramCopy->parameters[p];
		}

		if (!(functionFlags & FunctionFlags::FUNC_IS_STATIC) && callerClassInfo->declarationThis) {
			newFuncInfo->reflectDeclarationMap[callerClassInfo->declarationThis] =
			    callerClassInfo->declarationThis;
			auto callerClass = compile.classes[callerClassId];
			if (callerClass && callerClass->genericBaseClassId != 0) {
				auto baseClassInfo = context.classInfo[callerClass->genericBaseClassId];
				if (baseClassInfo && baseClassInfo->declarationThis) {
					newFuncInfo->reflectDeclarationMap[baseClassInfo->declarationThis] =
					    callerClassInfo->declarationThis;
				}
			}
		}

		// Issue 6: Add mapping for caller class members so body copy can resolve correctly
		for (auto *memberDecl : callerClassInfo->member) {
			if (memberDecl &&
			    newFuncInfo->reflectDeclarationMap.find(memberDecl) ==
			        newFuncInfo->reflectDeclarationMap.end()) {
				newFuncInfo->reflectDeclarationMap[memberDecl] = memberDecl;
			}
		}

		for (auto &[declarationNode, value] : funcInfo->reflectDeclarationMap) {
			newFuncInfo->reflectDeclarationMap[declarationNode] =
			    static_cast<DeclarationNode *>(declarationNode->copy(in_data));
		}
		for (auto &[declarationNode, value] : newFuncInfo->reflectDeclarationMap) {
			if (value && value->id >= newFunc->maxDeclaration) {
				newFunc->maxDeclaration = value->id + 1;
				newFuncInfo->declaration = newFunc->maxDeclaration;
			}
		}

		for (auto &[declarationNode, value] :
		     funcInfo->genericData->staticDeclaration) {
			auto node = context.makeDeclarationNode(
			    in_data, declarationNode->line, declarationNode->baseName,
			    newFunc->getName(compile) +
			        context.lexerString[declarationNode->baseName],
			    declarationNode->classDeclaration, declarationNode->isVal, true,
			    declarationNode->nullable, false, true);
			funcInfo->genericData
			    ->newPositionOfStaticDeclaration[declarationNode->id] =
			    node->id;
			if (node->classDeclaration) {
				if (!node->classDeclaration->classId) {
					node->classDeclaration->template load<false>(in_data);
					if (!node->classDeclaration->classId) {
						classDeclaration->throwError(
						    "Bug: Cannot find class name " +
						    node->classDeclaration->getName(in_data) +
						    "\nHint: Check static variable type in generic function");
					}
					node->optimize(in_data);
					node->classDeclaration->classId = std::nullopt;
				} else {
					node->classId = *node->classDeclaration->classId;
				}
			}
			node->optimize(in_data);
			if (value) {
				auto varNode = context.varPool.push(declarationNode->line, node,
				                                    false, true);
				auto setNode = context.setValuePool.push(declarationNode->line,
				                                         varNode, value, true);
				context.staticNode.push_back(setNode);
			}
		}

		auto lastNewPositionOfStaticDeclaration =
		    context.newPositionOfStaticDeclaration;
		context.newPositionOfStaticDeclaration =
		    &funcInfo->genericData->newPositionOfStaticDeclaration;
		newFuncInfo->body.nodes.reserve(funcInfo->body.nodes.size());
		for (auto *node : funcInfo->body.nodes) {
			newFuncInfo->body.nodes.push_back(node->copy(in_data));
		}
		if (funcInfo->inferenceNode) {
			newFuncInfo->inferenceNode =
			    static_cast<ReturnNode *>(newFuncInfo->body.nodes[0]);
			context.mustInferenceFunctionType.push_back(newFunc->id);
		}
		context.newPositionOfStaticDeclaration =
		    lastNewPositionOfStaticDeclaration;

		newFuncInfo->body.resolve(in_data);
		newFuncInfo->body.optimize(in_data);
		newCreateFuncNode->optimize(in_data);

		auto &funcList = callerClassInfo->allFunction[nameId];
		bool foundFunc = false;
		for (auto fid : funcList) {
			if (fid == newFunc->id) {
				foundFunc = true;
				break;
			}
		}
		if (!foundFunc) {
			funcList.push_back(newFunc->id);
		}
		callerClassInfo->createFunctionNodes.push_back(newCreateFuncNode);

		context.currentClassId = lastCurrentClassId;
		context.gotoFunction(lastCurrentFunctionId);

		// Restore snapshot of GenericDeclarationNode (loadMemberFunctionGenerics)
		for (auto &snap : memberGenDeclSnapshots) {
			snap.decl->classId = snap.classId;
			snap.decl->nullable = snap.nullable;
			for (auto &cdSnap : snap.cdSnapshots) {
				cdSnap.cd->classId = cdSnap.classId;
				cdSnap.cd->nullable = cdSnap.nullable;
				cdSnap.cd->baseClassLexerStringId = cdSnap.baseClassLexerStringId;
				cdSnap.cd->inputClassId = cdSnap.inputClassId;
			}
		}

		for (auto &snap : classGenDeclSnapshots) {
			snap.decl->classId = snap.classId;
			snap.decl->nullable = snap.nullable;
			for (auto &cdSnap : snap.cdSnapshots) {
				cdSnap.cd->classId = cdSnap.classId;
				cdSnap.cd->nullable = cdSnap.nullable;
				cdSnap.cd->baseClassLexerStringId = cdSnap.baseClassLexerStringId;
				cdSnap.cd->inputClassId = cdSnap.inputClassId;
			}
		}
	}
}

} // namespace Autolang
#endif
