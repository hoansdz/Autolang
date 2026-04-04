#ifndef DEBUGGER_GENERIC_CPP
#define DEBUGGER_GENERIC_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "shared/ClassFlags.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <memory>

namespace AutoLang {

ClassId loadClassGenerics(in_func, std::string &name,
                          ClassDeclaration *classDeclaration) {
	auto it =
	    context.defaultClassMap.find(classDeclaration->baseClassLexerStringId);
	if (it == context.defaultClassMap.end()) {
		classDeclaration->throwError(
		    "Bug: Cannot find class " +
		    context.lexerString[classDeclaration->baseClassLexerStringId]);
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
		classDeclaration->throwError(clazz->name + " is not generics");
	}
	auto baseCreateClassNode = context.findCreateClassNode(clazz->id);
	LexerStringId newNameId = context.createLexerStringIfNotExists(name);

	auto newCreateClassNode = context.newClasses.push(
	    baseCreateClassNode->line, newNameId, baseCreateClassNode->classFlags);
	newCreateClassNode->pushClass(in_data);
	// std::cerr<<"Created create class node
	// "<<context.lexerString[newNameId]<<"\n";
	newCreateClassNode->superDeclaration =
	    baseCreateClassNode->superDeclaration;
	auto newClassId = newCreateClassNode->classId;
	context.defaultClassMap[newNameId] = newClassId;
	context.newDefaultClassesMap[newClassId] = newCreateClassNode;

	std::vector<ClassDeclaration *> genericTypeId;

	for (size_t i = 0; i < classInfo->genericData->genericDeclarations.size();
	     ++i) {
		auto &genericDeclaration =
		    classInfo->genericData->genericDeclarations[i];
		auto &inputClass = classDeclaration->inputClassId[i];

		ClassId inputClassId = *inputClass->classId;

		if (genericDeclaration->condition) {
			auto &condition = *genericDeclaration->condition;
			// if (condition.condition ==
			// GenericDeclarationCondition::MUST_EXTENDS) {
			if (!condition.classDeclaration->classId) {
				condition.classDeclaration->load<true>(in_data);
				if (!condition.classDeclaration->classId) {
					condition.classDeclaration->throwError(
					    "Unresolved " +
					    condition.classDeclaration->getName(in_data));
				}
			}
			context.checkValidateExtends[genericDeclaration].push_back(
			    inputClass);
			// }
		}

		// Change callnode name
		if (!genericDeclaration->allCallNodes.empty()) {
			const std::string &name = compile.classes[inputClassId]->name;
			for (auto *callNode : genericDeclaration->allCallNodes) {
				callNode->nameId = context.createLexerStringIfNotExists(name);
			}
		}
		// Change generics type
		genericDeclaration->classId = inputClassId;
		genericDeclaration->nullable = inputClass->nullable;
		auto newClassDeclaration = context.classDeclarationAllocator.push();
		newClassDeclaration->classId = inputClassId;
		newClassDeclaration->nullable = inputClass->nullable;
		newClassDeclaration->line = genericDeclaration->line;
		genericTypeId.push_back(newClassDeclaration);

		for (auto *classDeclaration :
		     genericDeclaration->allClassDeclarations) {
			classDeclaration->classId = inputClassId;
			if (classDeclaration->mustInference) {
				classDeclaration->nullable = inputClass->nullable;
				classDeclaration->mustInference = false;
			}
			classDeclaration->baseClassLexerStringId =
			    inputClass->baseClassLexerStringId;
		}
	}

	auto newClass = compile.classes[newClassId];
	newClass->memberMap = clazz->memberMap;
	newClass->memberId = clazz->memberId;
	auto newClassInfo = context.classInfo[newClassId];
	newClassInfo->baseClassId = classId;
	newClassInfo->memberMap = classInfo->memberMap;
	newClassInfo->genericTypeId = std::move(genericTypeId);
	newClassInfo->declarationThis = context.declarationNodePool.push(
	    classInfo->declarationThis->line, newClassId, lexerIdthis, "this",
	    nullptr, true, false, false);
	newClassInfo->declarationThis->id = 0;
	newClassInfo->declarationThis->classId = newClassId;
	auto lastCurrentClassId = context.currentClassId;
	context.currentClassId = newClassId;

	// std::cerr << "Created " << newClass->name << "\n ";

	if (newCreateClassNode->superDeclaration &&
	    !newCreateClassNode->superDeclaration->classId) {
		newCreateClassNode->superDeclaration->load<true>(in_data);
		// std::cerr << "Created "
		//           << newCreateClassNode->superDeclaration->getName(in_data)
		//           << "\n ";
	}

	for (auto *member : classInfo->member) {
		if (member->classDeclaration) {
			if (!member->classDeclaration->classId) {
				member->classDeclaration->load<true>(in_data);
				if (!member->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Unsolved " +
					    member->classDeclaration->getName(in_data));
				}
			}
			member->classId = *member->classDeclaration->classId;
			newClass->memberId[member->id] = *member->classDeclaration->classId;
			member->nullable = member->classDeclaration->nullable;
			// std::cerr
			//     << "Member declaration: "
			//     << compile.classes[*member->classDeclaration->classId]->name
			//     << "\n";
		} else {
			// std::cerr << "No" << "\n";
		}
		newClassInfo->member.push_back(
		    static_cast<DeclarationNode *>(member->copy(in_data)));
	}

	newClassInfo->declarationThis->classId = newClassId;
	// newClassInfo->func = classInfo->func;
	newClassInfo->parent = classInfo->parent;
	newClassInfo->staticFunc = classInfo->staticFunc;

	// newClass->funcMap = clazz->funcMap;

	for (auto &[classDeclaration, node] :
	     classInfo->genericData->mustRenameNodes) {
		if (!classDeclaration->classId) {
			classDeclaration->load<false, true>(in_data);
			if (!classDeclaration->classId) {
				classDeclaration->throwError(
				    "Unsolved " + classDeclaration->getName(in_data));
			}
		}
		auto name = classDeclaration->getName(in_data);
		switch (node->kind) {
			case NodeType::UNKNOW: {
				auto unknowNode = static_cast<UnknowNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError("CUnsolved " + name);
				}
				unknowNode->nameId = it->second;
				break;
			}
			case NodeType::CALL: {
				auto callNode = static_cast<CallNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError("DUnsolved " + name);
				}
				callNode->nameId = it->second;
				break;
			}
		}
		classDeclaration->classId = std::nullopt;
	}

	ParserContext::mode = baseCreateClassNode->mode;

	for (auto *declarationNode : classInfo->allDeclarationNode) {
		if (declarationNode->classDeclaration) {
			if (!declarationNode->classDeclaration->classId) {
				declarationNode->classDeclaration->load<false>(in_data);
				if (!declarationNode->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot find class name " +
					    declarationNode->classDeclaration->getName(in_data));
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
		    newClass->name + "." +
		        context.lexerString[declarationNode->baseName],
		    declarationNode->classDeclaration, declarationNode->isVal, true,
		    declarationNode->nullable, false, true);
		newClassInfo->staticMember[declarationNode->baseName] = node;
		if (node->classDeclaration) {
			if (!node->classDeclaration->classId) {
				node->classDeclaration->load<false>(in_data);
				if (!node->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot find class name " +
					    node->classDeclaration->getName(in_data));
				}
				node->optimize(in_data);
				node->classDeclaration->classId = std::nullopt;
				// declarationNode->classDeclaration = nullptr;
			} else {
				node->classId = *node->classDeclaration->classId;
			}
			// std::cerr << "loaded " << (compile.classes[node->classId]->name)
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
			// std::cerr << "Added " << node->name << "\n";
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
		}
	}

	auto lastCurrentFunctionId = context.currentFunctionId;

	for (auto *createFuncNode : classInfo->createFunctionNodes) {
		auto funcInfo = context.functionInfo[createFuncNode->id];
		auto newCreateFuncNode = context.newFunctions.push(
		    createFuncNode->line, newClassId,
		    createFuncNode->nameId == lexerId__CLASS__ ? newNameId
		                                               : createFuncNode->nameId,
		    nullptr, createFuncNode->parameter->copy(in_data),
		    createFuncNode->functionFlags);
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
			if (!createFuncNode->classDeclaration->classId) {
				createFuncNode->classDeclaration->load<false>(in_data);
				if (!createFuncNode->classDeclaration->classId) {
					classDeclaration->throwError("wtf");
				}
				createFuncNode->classDeclaration->classId = std::nullopt;
			}
			newFunc->returnId = *createFuncNode->classDeclaration->classId;
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
			context.mustInferenceFunctionType.push_back(newFunc->id);
		}
	}

	context.gotoFunction(lastCurrentFunctionId);
	context.currentClassId = lastCurrentClassId;

	// std::cerr << "Created " << newClass->name << "\n";
	return newClassId;
}

FunctionId loadFunctionGenerics(in_func, std::string &name,
                                ClassDeclaration *classDeclaration) {
	auto it = compile.funcMap.find(
	    context.lexerString[classDeclaration->baseClassLexerStringId]);
	if (it == compile.funcMap.end()) {
		classDeclaration->throwError(
		    "Bug: Cannot find function " +
		    context.lexerString[classDeclaration->baseClassLexerStringId]);
	}
	{
		auto it = compile.funcMap.find(name);
		if (it != compile.funcMap.end()) {
			return it->second[0];
		}
	}
	FunctionId funcId = it->second[0];
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	if (!funcInfo->genericData) {
		classDeclaration->throwError(func->name + " is not generics");
	}
	auto createFuncNode = context.genericFunctionMap[func->name];
	ParserContext::mode = createFuncNode->mode;

	std::vector<ClassDeclaration *> genericTypeId;

	for (size_t i = 0; i < funcInfo->genericData->genericDeclarations.size();
	     ++i) {
		auto &genericDeclaration =
		    funcInfo->genericData->genericDeclarations[i];
		auto &inputClass = classDeclaration->inputClassId[i];

		ClassId inputClassId = *inputClass->classId;

		if (genericDeclaration->condition) {
			auto &condition = *genericDeclaration->condition;
			// if (condition.condition ==
			// GenericDeclarationCondition::MUST_EXTENDS) {
			if (!condition.classDeclaration->classId) {
				condition.classDeclaration->load<true>(in_data);
				if (!condition.classDeclaration->classId) {
					condition.classDeclaration->throwError(
					    "Unresolved " +
					    condition.classDeclaration->getName(in_data));
				}
			}
			context.checkValidateExtends[genericDeclaration].push_back(
			    inputClass);
			// }
		}

		// Change callnode name
		if (!genericDeclaration->allCallNodes.empty()) {
			const std::string &name = compile.classes[inputClassId]->name;
			for (auto *callNode : genericDeclaration->allCallNodes) {
				callNode->nameId = context.createLexerStringIfNotExists(name);
			}
		}
		// Change generics type
		genericDeclaration->classId = inputClassId;
		genericDeclaration->nullable = inputClass->nullable;
		auto newClassDeclaration = context.classDeclarationAllocator.push();
		newClassDeclaration->classId = inputClassId;
		newClassDeclaration->nullable = inputClass->nullable;
		newClassDeclaration->line = genericDeclaration->line;
		genericTypeId.push_back(newClassDeclaration);

		for (auto *classDeclaration :
		     genericDeclaration->allClassDeclarations) {
			classDeclaration->classId = inputClassId;
			if (classDeclaration->mustInference) {
				classDeclaration->nullable = inputClass->nullable;
				classDeclaration->mustInference = false;
			}
			classDeclaration->baseClassLexerStringId =
			    inputClass->baseClassLexerStringId;
		}
	}
	auto lastCurrentFunctionId = context.currentFunctionId;
	auto newCreateFuncNode = context.newFunctions.push(
	    createFuncNode->line, createFuncNode->contextCallClassId,
	    context.createLexerStringIfNotExists(name), nullptr,
	    funcInfo->parameter->copy(in_data), createFuncNode->functionFlags);

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
	newFunc->returnId = compile.functions[createFuncNode->id]->returnId;
	context.gotoFunction(newCreateFuncNode->id);
	// std::cerr << "THIS " << newFunc->name << "\n";
	if (createFuncNode->classDeclaration) {
		if (!createFuncNode->classDeclaration->classId) {
			createFuncNode->classDeclaration->load<false>(in_data);
			if (!createFuncNode->classDeclaration->classId) {
				classDeclaration->throwError("wtf");
			}
			createFuncNode->classDeclaration->classId = std::nullopt;
		}
		newFunc->returnId = *createFuncNode->classDeclaration->classId;
	}

	for (auto &[classDeclaration, node] :
	     funcInfo->genericData->mustRenameNodes) {
		if (!classDeclaration->classId) {
			classDeclaration->load<false, true>(in_data);
			if (!classDeclaration->classId) {
				classDeclaration->throwError(
				    "Unsolved " + classDeclaration->getName(in_data));
			}
		}
		auto name = classDeclaration->getName(in_data);
		switch (node->kind) {
			case NodeType::UNKNOW: {
				auto unknowNode = static_cast<UnknowNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError("AUnsolved " + name);
				}
				unknowNode->nameId = it->second;
				break;
			}
			case NodeType::CALL: {
				auto callNode = static_cast<CallNode *>(node);
				auto it = context.lexerStringMap.find(name);
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError("BUnsolved " + name);
				}
				callNode->nameId = it->second;
				break;
			}
		}
		classDeclaration->classId = std::nullopt;
	}

	newFuncInfo->reflectDeclarationMap.reserve(
	    funcInfo->reflectDeclarationMap.size());

	for (auto &[declarationNode, value] : funcInfo->reflectDeclarationMap) {
		newFuncInfo->reflectDeclarationMap[declarationNode] =
		    static_cast<DeclarationNode *>(declarationNode->copy(in_data));
		// std::cerr << "Created declaration: " << declarationNode << " "
		//           << newFuncInfo->reflectDeclarationMap[declarationNode]
		//           << "\n";
	}

	for (auto &[declarationNode, value] :
	     funcInfo->genericData->staticDeclaration) {
		auto node = context.makeDeclarationNode(
		    in_data, declarationNode->line, declarationNode->baseName,
		    newFunc->name + context.lexerString[declarationNode->baseName],
		    declarationNode->classDeclaration, declarationNode->isVal, true,
		    declarationNode->nullable, false, true);
		funcInfo->genericData
		    ->newPositionOfStaticDeclaration[declarationNode->id] = node->id;
		if (node->classDeclaration) {
			if (!node->classDeclaration->classId) {
				node->classDeclaration->load<false>(in_data);
				if (!node->classDeclaration->classId) {
					classDeclaration->throwError(
					    "Bug: Cannot find class name " +
					    node->classDeclaration->getName(in_data));
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
			auto varNode =
			    context.varPool.push(declarationNode->line, node, false, true);
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
	context.newPositionOfStaticDeclaration = lastNewPositionOfStaticDeclaration;
	context.gotoFunction(lastCurrentFunctionId);

	for (auto &[classDeclaration, node] :
	     funcInfo->genericData->mustRenameNodes) {
		if (!classDeclaration) {
			classDeclaration->load<false>(in_data);
			if (!classDeclaration->classId) {
				classDeclaration->throwError(
				    "Unsolved " + classDeclaration->getName(in_data));
			}
		}
		switch (node->kind) {
			case NodeType::UNKNOW: {
				auto unknowNode = static_cast<UnknowNode *>(node);
				auto it = context.lexerStringMap.find(
				    classDeclaration->getName(in_data));
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError(
					    "Unsolved " + classDeclaration->getName(in_data));
				}
				unknowNode->nameId = it->second;
				break;
			}
			case NodeType::CALL: {
				auto callNode = static_cast<CallNode *>(node);
				auto it = context.lexerStringMap.find(
				    classDeclaration->getName(in_data));
				if (it == context.lexerStringMap.end()) {
					classDeclaration->throwError(
					    "Unsolved " + classDeclaration->getName(in_data));
				}
				callNode->nameId = it->second;
				break;
			}
		}
		classDeclaration->classId = std::nullopt;
	}

	return newFunc->id;
}

} // namespace AutoLang
#endif