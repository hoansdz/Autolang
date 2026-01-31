#ifndef CREATE_NODE_CPP
#define CREATE_NODE_CPP

#include <functional>
#include "frontend/parser/node/CreateNode.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/Type.hpp"

namespace AutoLang {

void DeclarationNode::optimize(in_func) {
	if (isFunctionExist(in_data, name))
		throwError(
		    "Cannot declare variable with the same name as function name " +
		    name);
	if (isClassExist(in_data, name))
		throwError("Cannot declare variable with the same name as class name " +
		           name);
	if (!className.empty()) {
		auto clazz = findClass(in_data, className);
		if (!clazz)
			throwError(std::string("Cannot find class name : ") + className);
		classId = clazz->id;
	}
	// printDebug("DeclarationNode: " + name + " is " +
	// compile.classes[classId]->name);
}

void CreateConstructorNode::pushFunction(in_func) {
	AClass *clazz = compile.classes[classId];
	funcId = compile.registerFunction<true>(
	    clazz, false, name, new ClassId[arguments.size() + 1]{},
	    std::vector<bool>(arguments.size() + 1, false), classId, false,
	    isPrimary && !arguments.empty() ? AutoLang::DefaultFunction::data_constructor : nullptr );
	auto func = compile.functions[funcId];
	auto funcInfo = &context.functionInfo[funcId];

	func->args[0] = classId;

	funcInfo->clazz = clazz;
	funcInfo->accessModifier = accessModifier;
	funcInfo->isConstructor = true;
	func->maxDeclaration = 1;
	funcInfo->declaration = 1;
	if (isPrimary) {
		auto classInfo = &context.classInfo[clazz->id];
		// printDebug(clazz->name);
		// printDebug(arguments.size());
		for (size_t i = 0; i < arguments.size(); ++i) {
			auto argument = arguments[i];
			clazz->memberMap[argument->name] = i;
			clazz->memberId.push_back(0);
			classInfo->member.push_back(argument);
		}
	}
}

void CreateConstructorNode::optimize(in_func) {
	auto func = compile.functions[funcId];
	auto funcInfo = &context.functionInfo[funcId];
	AClass *clazz = compile.classes[classId];
	// Add argument class id
	auto classInfo = &context.classInfo[classId];
	auto *newClassNode = context.newClassesMap[classId];
	for (size_t i = 0; i < arguments.size(); ++i) {
		auto &argument = arguments[i];
		func->args[i + 1] = argument->classId;
		func->nullableArgs[i + 1] = argument->nullable;
		if (isPrimary) {

			// printDebug((uintptr_t)clazz);
			// printDebug(classInfo->declarationThis->className);
			// printDebug(std::to_string(i) + " ... " +
			// std::to_string(clazz->memberId.size()));
			// auto setNode = context.setValuePool.push(
			//     line,
			//     new GetPropNode(
			//         line, nullptr, classId,
			//         new VarNode(line, classInfo->declarationThis, false, false),
			//         argument->name, true, true, false),
			//     new VarNode(line, argument, false, true));
			// body.nodes.push_back(setNode);
			clazz->memberId[i] = argument->classId;
			// printDebug("Member: " + std::to_string(i) + " " + argument->name
			// + " is " + compile.classes[argument->classId]->name);
		}
	}

	// Check redefine
	funcInfo->hash = func->loadHash();
	auto &hash = classInfo->func[name];
	auto it = hash.find(funcInfo->hash);
	if (it != hash.end() && compile.functions[it->second]->name == func->name) {
		throwError("Redefined function : " + func->toString(compile));
	}
	hash[funcInfo->hash] = func->id;

	// Check super
	if (classInfo->parent) {
		if (body.nodes.empty()) {
			throwError(
			    "super() must be called first in a derived class constructor.");
		}
		auto *n = body.nodes[0];
		switch (n->kind) {
			case NodeType::CALL: {
				auto node = static_cast<CallNode *>(n);
				if (node->caller || node->name != "super()") {
					throwError(
					    "super() must be called first in a derived class "
					    "constructor.");
				}
				node->isSuper = true;
				node->name = compile.classes[*classInfo->parent]->name + "()";
				break;
			}
			default:
				throwError("super() must be called first in a derived class "
				           "constructor.");
		}
	}

	// Add return bytecodes
	if (!isPrimary) {
		body.nodes.push_back(context.returnPool.push(
		    line, funcId,
		    new VarNode(line, classInfo->declarationThis, false, false)));
	}
}

void CreateClassNode::pushClass(in_func) {
	classId = compile.registerClass(context.lexerString[nameId]);
}

void CreateClassNode::optimize(in_func) {
	auto &name = context.lexerString[nameId];
	if (isFunctionExist(in_data, name))
		throwError("Cannot declare class with the same name as function name " +
		           name);
	if (isDeclarationExist(in_data, name))
		throwError("Cannot declare class with the same name as variable name " +
		           name);
	if (superId) {
		auto &superClassName = context.lexerString[*superId];
		auto it = compile.classMap.find(superClassName);
		if (it == compile.classMap.end()) {
			throwError("Cannot find class " + superClassName);
		}
		auto classInfo = &context.classInfo[classId];
		classInfo->parent = it->second;
	}
}

void CreateClassNode::loadSuper(in_func) {
	if (!loadedSuper && superId) {
		auto clazz = compile.classes[classId];
		auto classInfo = &context.classInfo[classId];
		auto superClassId = *classInfo->parent;
		auto superClass = compile.classes[superClassId];
		auto superClassInfo = &context.classInfo[superClassId];

		switch (superClassId) {
			case DefaultClass::intClassId:
			case DefaultClass::floatClassId:
			case DefaultClass::nullClassId:
			case DefaultClass::stringClassId:
			case DefaultClass::boolClassId:
			case DefaultClass::anyClassId:
				throwError(superClass->name + " cannot be extends");
			default:
				break;
		}

		loadedSuper = true;

		{
			auto it = context.newClassesMap.find(superClassId);
			it->second->loadSuper(in_data);
		}
		

		if (superClass->inheritance.get(classId)) {
			throwError("Cyclic inheritance is not allowed.");
		}

		auto memberToFind = HashMap<std::string_view, DeclarationNode *>();
		memberToFind.reserve(classInfo->member.size());
		for (auto *declaration : classInfo->member) {
			memberToFind[declaration->name] = declaration;
			declaration->id += superClassInfo->member.size();
			clazz->memberMap[declaration->name] = declaration->id;
		}
		uint32_t newPosition = superClassInfo->member.size();
		for (auto &declaration : superClassInfo->member) {
			auto it = memberToFind.find(declaration->name);
			if (it != memberToFind.end()) {
				throwError("Member '" + declaration->name +
				           "' is already declared in superclass '" +
				           superClass->name +
				           "'. Overriding is not supported yet.");
			}
			clazz->memberMap[declaration->name] = declaration->id;
		}
		classInfo->member.reserve(classInfo->member.size() +
		                          superClassInfo->member.size());
		classInfo->member.insert(classInfo->member.begin(),
		                         superClassInfo->member.begin(),
		                         superClassInfo->member.end());
		auto newStaticMember = superClassInfo->staticMember;
		for (auto &[key, value] : classInfo->staticMember) {
			newStaticMember[key] = value;
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

		for (auto &[funcName, superFuncHash] : superClassInfo->func) {
			auto &hash = classInfo->func[funcName];
			for (auto &[hashValue, offset] : superFuncHash) {
				auto it = hash.find(hashValue);
				if (it != hash.end()) {
					auto funcInfo = &context.functionInfo[it->second];
					auto superFunc = compile.functions[offset];
					auto superFuncInfo = &context.functionInfo[offset];
					// Index virtual position : Three times override -> Twice
					// override -> First override  -> parent
					if (!superFuncInfo->isVirtual) {
						ClassId parentId = *clazz->parentId;
						while (true) {
							auto parentClassInfo = &context.classInfo[parentId];
							auto it1 = parentClassInfo->func.find(funcName);
							if (it1 == parentClassInfo->func.end()) break;
							auto& hashMap = it1->second;
							auto it2 = hashMap.find(hashValue);
							if (it2 == hashMap.end()) break;
							auto parentClass = compile.classes[parentId];
							parentClass->vtable.push_back(offset);
							if (!parentClass->parentId) break;
							parentId = *parentClass->parentId;
						}
						superFuncInfo->virtualPosition = clazz->vtable.size();
						superFuncInfo->isVirtual = true;

						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable.push_back(it->second);
						funcInfo->isVirtual = true;
					} else {
						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable[funcInfo->virtualPosition] = it->second;
						funcInfo->isVirtual = true;
					}
					std::cerr<<superFunc->name<<" ok\n";
					funcOverride.resize(superFunc->id);
					funcOverride.set(superFunc->id);
				}
				hash[hashValue] = offset;
			}
		}

		for (auto &[key, value] : superClass->funcMap) {
			auto &vecs = clazz->funcMap[key];
			vecs.reserve(vecs.size() + value.size());
			for (auto val : value) {
				if (funcOverride.get(val)) {
					continue;
				}
				vecs.push_back(val);
			}
		}
	}
}

bool isFunctionExist(in_func, std::string &name) {
	std::string newName = name + "()";
	auto it = compile.funcMap.find(newName);
	return it != compile.funcMap.end();
}

bool isClassExist(in_func, std::string &name) {
	auto it = compile.classMap.find(name);
	return it != compile.classMap.end();
}

bool isDeclarationExist(in_func, std::string &name) {
	return context.findDeclaration(in_data, 0, name, true);
}

} // namespace AutoLang

#endif