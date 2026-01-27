#ifndef CREATE_NODE_CPP
#define CREATE_NODE_CPP

#include "frontend/parser/node/CreateNode.hpp"
#include "shared/DefaultFunction.hpp"

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
	// compile.classes[classId].name);
}

void CreateFuncNode::pushFunction(in_func) {
	AClass *clazz =
	    contextCallClassId ? &compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(clazz, isStatic, name + "()",
	                              std::vector<uint32_t>(arguments.size(), 0),
	                              std::vector<bool>(arguments.size(), false),
	                              clazz ? clazz->id
	                                    : AutoLang::DefaultClass::nullClassId,
	                              returnNullable, nullptr);
	auto func = &compile.functions[id];
	auto funcInfo = &context.functionInfo[id];
	funcInfo->clazz = clazz;
	funcInfo->accessModifier = accessModifier;
}

void CreateFuncNode::optimize(in_func) {
	if (!contextCallClassId && isDeclarationExist(in_data, name))
		throwError(
		    "Cannot declare function with the same name as declaration name " +
		    name);
	if (isClassExist(in_data, name))
		throwError("Cannot declare function with the same name as class name " +
		           name);
	auto func = &compile.functions[id];
	if (!returnClass.empty()) {
		auto it = compile.classMap.find(returnClass);
		if (it == compile.classMap.end())
			throwError("CreateFuncNode: Cannot find class name: " +
			           returnClass);
		func->returnId = it->second;
	} else
		func->returnId = AutoLang::DefaultClass::nullClassId;
	for (size_t i = 0; i < arguments.size(); ++i) {
		auto &argument = arguments[i];
		func->args[i] = argument->classId;
		func->nullableArgs[i] = argument->nullable;
	}
	auto &listFunc = compile.funcMap[name + "()"];
	for (auto id : listFunc) {
		auto f = &compile.functions[id];
		// id >= func->id because functions[id] will be optimized in the future
		if (id >= func->id || f->args.size != func->args.size)
			continue;
		for (size_t i = 0; i < f->args.size; ++i) {
			if (f->args[i] != func->args[i])
				continue;
		}
		throwError("Redefined function : " + func->toString(compile));
	}
}

void CreateConstructorNode::pushFunction(in_func) {
	AClass *clazz = &compile.classes[classId];
	funcId = compile.registerFunction<true>(
	    clazz, false, name, std::vector<uint32_t>(arguments.size(), 0),
	    std::vector<bool>(arguments.size(), false), classId, false,
	    isPrimary ? AutoLang::DefaultFunction::data_constructor : nullptr);
	auto func = &compile.functions[funcId];
	auto funcInfo = &context.functionInfo[funcId];

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
	auto func = &compile.functions[funcId];
	AClass *clazz = &compile.classes[classId];
	// Add argument class id
	auto classInfo = &context.classInfo[classId];
	for (size_t i = 0; i < arguments.size(); ++i) {
		auto &argument = arguments[i];
		func->args[i + 1] = argument->classId;
		func->nullableArgs[i + 1] = argument->nullable;
		if (isPrimary) {
			// printDebug((uintptr_t)clazz);
			// printDebug(classInfo->declarationThis->className);
			// printDebug(std::to_string(i) + " ... " +
			// std::to_string(clazz->memberId.size()));
			clazz->memberId[i] = argument->classId;
			// printDebug("Member: " + std::to_string(i) + " " + argument->name
			// + " is " + compile.classes[argument->classId].name);
		}
	}

	// Check redefine

	auto &listFunc = compile.funcMap[func->name];
	for (auto id : listFunc) {
		auto f = &compile.functions[id];
		// id >= funcId because functions[id] will be optimized in the future
		if (id >= funcId || f->args.size != func->args.size)
			continue;
		for (size_t i = 1; i < f->args.size; ++i) {
			if (f->args[i] != func->args[i])
				goto next;
		}
		throwError("Redefined constructor : " + func->toString(compile));
	next:;
	}

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
				node->name = compile.classes[*classInfo->parent].name + "()";
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
	if (optimized) {
		return;
	}
	auto &name = context.lexerString[nameId];
	if (isFunctionExist(in_data, name))
		throwError("Cannot declare class with the same name as function name " +
		           name);
	if (isDeclarationExist(in_data, name))
		throwError("Cannot declare class with the same name as variable name " +
		           name);
	optimized = true;
	if (superId) {
		auto &superClassName = context.lexerString[*superId];
		auto it = compile.classMap.find(superClassName);
		if (it == compile.classMap.end()) {
			throwError("Cannot find class " + superClassName);
		}
		auto superClassId = it->second;
		auto clazz = &compile.classes[classId];
		auto classInfo = &context.classInfo[classId];
		auto superClass = &compile.classes[superClassId];
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

		context.newClassesMap[superClassId]->optimize(in_data);
		if (superClass->inheritance.get(classId)) {
			throwError("Cyclic inheritance is not allowed.");
		}
		classInfo->parent = superClassId;

		auto memberToFind =
		    ankerl::unordered_dense::map<std::string_view, DeclarationNode *>();
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
		clazz->memberId.insert(clazz->memberId.begin(),
		                       superClass->memberId.begin(),
		                       superClass->memberId.end());
		uint32_t maxSize = std::max<ClassId>(
		    superClassId, superClass->inheritance.getSize() * 64);
		clazz->inheritance.from(superClass->inheritance, maxSize);
		clazz->inheritance.set(superClassId);
		
		for (auto& [key, value] : superClass->funcMap) {
			auto& vecs = clazz->funcMap[key];
			vecs.insert(vecs.end(), value.begin(), value.end());
		}
	}
	// auto clazz = &compile.classes[classId];
	// body.optimize(in_data);
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