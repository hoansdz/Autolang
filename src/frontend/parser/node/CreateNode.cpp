#ifndef CREATE_NODE_CPP
#define CREATE_NODE_CPP

#include "frontend/parser/node/CreateNode.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/Type.hpp"
#include <functional>

namespace AutoLang {

void DeclarationNode::optimize(in_func) {
	if (loaded) {
		return;
	}
	{
		auto it = context.globalFunction.find(baseName);
		if (it != context.globalFunction.end()) {
			throwError("Cannot declare variable with the same name as function "
			           "name: " +
			           name);
		}
	}
	if (contextCallClassId) {
		auto classInfo = context.classInfo[*contextCallClassId];
		auto it = classInfo->allFunction.find(baseName);
		if (it != classInfo->allFunction.end()) {
			throwError("Cannot declare variable with the same name as function "
			           "name: " +
			           name);
		}
	}
	{
		auto it = context.defaultClassMap.find(baseName);
		if (it != context.defaultClassMap.end()) {
			throwError("Cannot declare variable with the same name as "
			           "class name: " +
			           name);
		}
	}
	if (classDeclaration) {
		// Doesn't changed
		if (classDeclaration->isGenerics(in_data)) {
			return;
		}
		loaded = true;
		auto &baseClassName =
		    context.lexerString[classDeclaration->baseClassLexerStringId];
		auto it = compile.classMap.find(baseClassName);
		if (it == compile.classMap.end()) {
			throwError(
			    std::string("DeclarationNode: Cannot find class name : ") +
			    baseClassName);
		}
		auto classInfo = context.classInfo[it->second];
		if (!classInfo->genericData) {
			classId = it->second;
			return;
		}
		if (classInfo->genericData->genericDeclarations.size() !=
		        classDeclaration->inputClassId.size() &&
		    !classDeclaration->isGenericDeclaration) {
			throwError("'" + baseClassName + "' expects " +
			           std::to_string(
			               classInfo->genericData->genericDeclarations.size()) +
			           " type argument but " +
			           std::to_string(classDeclaration->inputClassId.size()) +
			           " were given");
		}
		// Generics
		if (!classDeclaration->classId) {
			throwError("Unresolved class id");
		}
		classId = *classDeclaration->classId;
		if (classId == DefaultClass::functionClassId) {
			// std::cerr<<classDeclaration->getName(in_data)<<"\n";
			// std::cerr<<nullable<<"\n";
			nullable = classDeclaration->nullable;
		}
		return;
	}
	// printDebug("DeclarationNode: " + name + " is " +
	//            compile.classes[classId]->name);
}

ExprNode *DeclarationNode::copy(in_func) {
	if (context.currentClassId) {
		if (baseName == lexerIdthis) {
			return context.classInfo[*context.currentClassId]->declarationThis;
		}
	}
	auto newNode = context.declarationNodePool.push(
	    line, context.currentClassId, baseName, name, classDeclaration, isVal,
	    isGlobal, nullable);
	newNode->mustInferenceNullable = mustInferenceNullable;
	if (isGlobal && context.newPositionOfStaticDeclaration) {
		auto it = context.newPositionOfStaticDeclaration->find(id);
		if (it != context.newPositionOfStaticDeclaration->end()) {
			newNode->id = it->second;
		} else {
			newNode->id = id;
		}
	} else {
		newNode->id = id;
	}

	if (classDeclaration) {
		if (!classDeclaration->classId) {
			classDeclaration->load<true>(in_data);
			if (!classDeclaration->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           classDeclaration->getName(in_data));
			}
		}
		newNode->classId = *classDeclaration->classId;
		// newNode->mustInferenceNullable = classDeclaration->mustInference;
		newNode->nullable = classDeclaration->nullable;
	} else {
		newNode->classId = classId;
	}
	return newNode;
}

void CreateConstructorNode::pushFunction(in_func) {
	auto *clazz = compile.classes[classId];
	auto *classInfo = context.classInfo[classId];
	funcId = compile.registerFunction<true>(
	    clazz, context.lexerString[nameId], new ClassId[parameters.size()]{},
	    parameters.size(), classId,
	    functionFlags | FunctionFlags::FUNC_IS_CONSTRUCTOR);
	// Function can be overrided, it will be recreated in override phase
	if (!(clazz->classFlags & ClassFlags::CLASS_HAS_PARENT)) {
		auto classInfo = context.classInfo[classId];
		classInfo->allFunction[nameId].push_back(funcId);
	}
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	new (&func->bytecodes) std::vector<uint8_t>();

	funcInfo->clazz = clazz;
	funcInfo->nullableArgs = new bool[parameters.size()]{};
	func->maxDeclaration = parameters.size();
	funcInfo->declaration = parameters.size();
	funcInfo->parameters = parameters;

	if (isPrimary) {
		auto classInfo = context.classInfo[clazz->id];
		func->functionFlags |= FunctionFlags::FUNC_IS_DATA_CONSTRUCTOR;
		// printDebug(clazz->name);
		// printDebug(arguments.size());
		for (size_t i = 1; i < parameters.size(); ++i) {
			auto param = parameters[i];
			classInfo->memberMap[param->baseName] = i - 1;
			clazz->memberMap[param->name] = i - 1;
			clazz->memberId.push_back(0);
			classInfo->member.push_back(param);
		}
	}
}

void CreateConstructorNode::optimize(in_func) {
	const auto &name = context.lexerString[nameId];
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	AClass *clazz = compile.classes[classId];
	// Add argument class id
	auto classInfo = context.classInfo[classId];
	for (size_t i = 0; i < parameters.size(); ++i) {
		auto &param = parameters[i];
		func->args[i] = param->classId;
		funcInfo->nullableArgs[i] = param->nullable;
		if (isPrimary && i != 0) {

			// printDebug((uintptr_t)clazz);
			// printDebug(classInfo->declarationThis->className);
			// printDebug(std::to_string(i) + " ... " +
			// std::to_string(clazz->memberId.size()));
			// auto setNode = context.setValuePool.push(
			//     line,
			//     context.getPropPool.push(
			//         line, nullptr, classId,
			//         context.varPool.push(line, classInfo->declarationThis,
			//         false, false), argument->name, true, true, false),
			//     context.varPool.push(line, argument, false, true));
			// body.nodes.push_back(setNode);
			clazz->memberId[i - 1] = param->classId;
			// printDebug("Member: " + std::to_string(i) + " " + argument->name
			// + " is " + compile.classes[argument->classId]->name);
		}
	}

	// Check redefine
	funcInfo->hash = func->loadHash();
	auto &hash = classInfo->func[nameId];
	auto it = hash.find(funcInfo->hash);
	if (it != hash.end() && compile.functions[it->second]->name == func->name) {
		throwError("Redefined function : " + func->toString(compile));
	}
	hash[funcInfo->hash] = func->id;

	// Check super
	if (clazz->classFlags & ClassFlags::CLASS_HAS_PARENT) {
		if (body.nodes.empty()) {
			throwError(
			    "super() must be called first in a derived class constructor.");
		}
		auto *n = body.nodes[0];
		switch (n->kind) {
			case NodeType::CALL: {
				auto node = static_cast<CallNode *>(n);
				if (node->caller || node->nameId != lexerIdsuper) {
					throwError(
					    "super() must be called first in a derived class "
					    "constructor.");
				}
				node->isSuper = true;
				node->nameId = context.createLexerStringIfNotExists(
				    compile.classes[classInfo->parent]->name);
				break;
			}
			default:
				throwError("super() must be called first in a derived class "
				           "constructor.");
		}
	}

	// Add return bytecodes
	auto thisNode =
	    context.varPool.push(line, classInfo->declarationThis, false, false);
	body.nodes.push_back(context.returnPool.push(line, funcId, thisNode));
}

void CreateClassNode::pushClass(in_func) {
	classId = compile.registerClass(context.lexerString[nameId], classFlags);
	context.defaultClassMap[nameId] = classId;
	auto classInfo = context.classInfoAllocator.push();
	context.classInfo.push_back(classInfo);
	// if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
	// 	if (!classInfo->genericData) {
	// 		return;
	// 	}
	// 	context.allClassDeclarations.push_back();
	// 	if (!superDeclaration->classId) {
	// 		throwError("Unresolved class name " +
	// 		           superDeclaration->getName(in_data));
	// 	}
	// 	auto classInfo = context.classInfo[classId];
	// 	classInfo->parent = *superDeclaration->classId;
	// }
	// auto clazz = compile.classes[classId];
	// std::cerr << "Created class name: " << clazz->name << " id " << classId
	//           << "\n";
}

void CreateClassNode::optimize(in_func) {
	const auto &name = context.lexerString[nameId];
	{
		auto it = context.globalFunction.find(nameId);
		if (it != context.globalFunction.end()) {
			throwError(
			    "Cannot declare class with the same name as function name " +
			    name);
		}
	}
	auto classInfo = context.classInfo[classId];
	if (classInfo->genericData) {
		for (auto genericDeclaration :
		     classInfo->genericData->genericDeclarations) {
			if (context.defaultClassMap.find(genericDeclaration->nameId) !=
			    context.defaultClassMap.end()) {
				throwError("Cannot declare generics type with the same name as "
				           "class name " +
				           context.lexerString[genericDeclaration->nameId]);
			}
		}
	}
	if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
		if (classInfo->genericData) {
			// std::cerr<<"A "<<name<<"\n";
			return;
		}
		// std::cerr<<"B "<<name<<"\n";
		if (!superDeclaration->classId) {
			throwError("Unresolved class name " +
			           superDeclaration->getName(in_data));
		}
		auto classInfo = context.classInfo[classId];
		classInfo->parent = *superDeclaration->classId;
	}
}

void CreateClassNode::loadSuper(in_func) {
	if ((classFlags & ClassFlags::CLASS_HAS_PARENT) && !loadedSuper) {
		auto clazz = compile.classes[classId];
		auto classInfo = context.classInfo[classId];
		auto superClassId = classInfo->parent;
		auto superClass = compile.classes[superClassId];
		auto superClassInfo = context.classInfo[superClassId];

		if (classInfo->genericData) {
			return;
		}

		if (superClass->classFlags & ClassFlags::CLASS_NO_EXTENDS) {
			throwError("@no_extends is already applied to " + superClass->name);
		}

		loadedSuper = true;

		{
			auto node = context.findCreateClassNode(superClassId);
			if (!node) {
				throwError("Bug: Cannot find create class node " +
				           compile.classes[superClassId]->name + " " +
				           std::to_string(superClassId));
			}
			node->loadSuper(in_data);
		}

		if (superClass->inheritance.get(classId)) {
			throwError("Cyclic inheritance is not allowed.");
		}

		auto memberToFind = HashMap<std::string_view, DeclarationNode *>();
		memberToFind.reserve(classInfo->member.size());
		for (auto *declaration : classInfo->member) {
			memberToFind[declaration->name] = declaration;
			declaration->id += superClassInfo->member.size();
			classInfo->memberMap[declaration->baseName] = declaration->id;
			clazz->memberMap[declaration->name] = declaration->id;
		}
		uint32_t newPosition = superClassInfo->member.size();
		for (auto &declaration : superClassInfo->member) {
			if (declaration->accessModifier == Lexer::TokenType::PRIVATE) {
				continue;
			}
			auto it = memberToFind.find(declaration->name);
			if (it != memberToFind.end()) {
				throwError("Member '" + declaration->name +
				           "' is already declared in superclass '" +
				           superClass->name +
				           "'. Overriding is not supported yet.");
			}
			classInfo->memberMap[declaration->baseName] = declaration->id;
			clazz->memberMap[declaration->name] = declaration->id;
		}
		classInfo->member.reserve(classInfo->member.size() +
		                          superClassInfo->member.size());
		classInfo->member.insert(classInfo->member.begin(),
		                         superClassInfo->member.begin(),
		                         superClassInfo->member.end());
		auto newStaticMember = superClassInfo->staticMember;
		for (auto &[key, declaration] : classInfo->staticMember) {
			if (declaration->accessModifier == Lexer::TokenType::PRIVATE) {
				continue;
			}
			newStaticMember[key] = declaration;
		}
		classInfo->staticMember = std::move(newStaticMember);
		clazz->memberId.insert(clazz->memberId.begin(),
		                       superClass->memberId.begin(),
		                       superClass->memberId.end());
		uint32_t maxSize = std::max<ClassId>(
		    superClassId, superClass->inheritance.getSize() * 64);
		clazz->inheritance.from(superClass->inheritance, maxSize);
		clazz->parentId = superClassId;
		clazz->inheritance.set(superClassId);

		// Static can be override without error
		for (auto &[key, superStaticFuncHash] : superClassInfo->staticFunc) {
			auto &hash = classInfo->staticFunc[key];
			for (auto &[hashValue, offset] : superStaticFuncHash) {
				hash[hashValue] = offset;
			}
		}

		clazz->vtable = superClass->vtable;
		InheritanceBitset funcOverride;

		for (auto &[funcNameId, superFuncHash] : superClassInfo->func) {
			auto &hash = classInfo->func[funcNameId];
			for (auto &[hashValue, offset] : superFuncHash) {
				auto it = hash.find(hashValue);
				if (it != hash.end()) {
					auto func = compile.functions[it->second];
					auto funcInfo = context.functionInfo[it->second];
					auto superFunc = compile.functions[offset];
					auto superFuncInfo = context.functionInfo[offset];

					// Index virtual position : Three times override -> Twice
					// override -> First override  -> parent
					if (!(superFunc->functionFlags &
					      FunctionFlags::FUNC_IS_VIRTUAL)) {
						if (superFunc->functionFlags &
						    FunctionFlags::FUNC_NO_OVERRIDE) {
							throwError("Function " +
							           superFunc->toString(compile) +
							           " is marked @no_override");
						}
						ClassId parentId = *clazz->parentId;
						while (true) {
							auto parentClassInfo = context.classInfo[parentId];
							auto it1 = parentClassInfo->func.find(funcNameId);
							if (it1 == parentClassInfo->func.end())
								break;
							auto &hashMap = it1->second;
							auto it2 = hashMap.find(hashValue);
							if (it2 == hashMap.end())
								break;
							auto parentClass = compile.classes[parentId];
							parentClass->vtable.push_back(offset);
							if (!parentClass->parentId)
								break;
							parentId = *parentClass->parentId;
						}
						superFuncInfo->virtualPosition = clazz->vtable.size();
						superFunc->functionFlags |=
						    FunctionFlags::FUNC_IS_VIRTUAL;

						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable.push_back(it->second);
						func->functionFlags |= FunctionFlags::FUNC_IS_VIRTUAL;
					} else {
						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable[funcInfo->virtualPosition] = it->second;
						func->functionFlags |= FunctionFlags::FUNC_IS_VIRTUAL;
					}
					funcOverride.resize(superFunc->id + 1);
					// std::cerr << superFunc->name << " & " << func->name <<
					// "\n"; std::cerr << superFuncInfo->virtualPosition << " &
					// "
					//           << funcInfo->virtualPosition << "\n"
					//           << func->id << " & " << superFunc->id << "\n"
					//           << funcOverride.getSize() << "\n";
					funcOverride.set(superFunc->id);
				}
				hash[hashValue] = offset;
			}
		}

		for (auto &[nameId, vecs] : classInfo->func) {
			auto &classVecs = classInfo->allFunction[nameId];
			for (auto &[_, funcId] : vecs) {
				classVecs.push_back(funcId);
			}
		}

		for (auto &[nameId, vecs] : classInfo->staticFunc) {
			auto &classVecs = classInfo->allFunction[nameId];
			for (auto &[_, funcId] : vecs) {
				classVecs.push_back(funcId);
			}
		}

		for (auto &[name, allFuncVecs] : superClass->funcMap) {
			auto &vecs = clazz->funcMap[name];
			vecs.reserve(vecs.size() + allFuncVecs.size());
			for (auto val : allFuncVecs) {
				auto func = compile.functions[val];
				if (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR) {
					continue;
				}
				if (funcOverride.get(val)) {
					continue;
				}
				vecs.push_back(val);
			}
		}
	}
}

} // namespace AutoLang

#endif