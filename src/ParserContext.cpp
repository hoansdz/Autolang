#ifndef PARSER_CONTEXT_CPP
#define PARSER_CONTEXT_CPP

#include "ParserContext.hpp"
#include "Debugger.hpp"

namespace AutoLang {
	
HasClassIdNode* ParserContext::findDeclaration(in_func, std::string& name, bool inGlobal) {
	AccessNode* node = currentFuncInfo->findDeclaration(in_data, name, justFindStatic);
	if (node) return node;
	if (currentClass) {
		node = currentClassInfo->findDeclaration(in_data, name, justFindStatic);
		//Static in function is VarNode, NonStatic is GetPropNode
		if (currentFunction->isStatic && node && node->kind == NodeType::GET_PROP)
			goto isNotStatic;
		if (node) return node;
	}
	if (!inGlobal || currentFuncInfo == mainFuncInfo) return node;
	node = mainFuncInfo->findDeclaration(in_data, name);
	if (node == nullptr) return node;
	if (justFindStatic) 
		goto isNotStatic;
	return node;
	isNotStatic:;
	throw std::runtime_error(name + " is not static");
}

DeclarationNode* ParserContext::makeDeclarationNode(bool isTemp, std::string name, std::string className, bool isVal, bool isGlobal, bool nullable, bool pushToScope) {
	auto func = isGlobal ? mainFunction : currentFunction;
	auto funcInfo = isGlobal ? mainFuncInfo : currentFuncInfo;
	if (pushToScope) {
		auto it = funcInfo->scopes.back().find(name);
		if (it != funcInfo->scopes.back().end())
			throw std::runtime_error(name + " has exist");
	}
	DeclarationNode* node = new DeclarationNode(std::move(name), std::move(className), isVal, isGlobal, nullable);
	node->classId = AutoLang::DefaultClass::nullClassId;
	node->id = pushToScope ? funcInfo->declaration++ : 0;
	funcInfo->declarationNodes.push_back(node);
	if (pushToScope) {
		size_t newSize = funcInfo->declaration;
		if (newSize > func->maxDeclaration)
			func->maxDeclaration = newSize;
		funcInfo->scopes.back()[node->name] = node;
	}
	return node;
}

AccessNode* FunctionInfo::findDeclaration(in_func, std::string& name, bool isStatic) {
	for (size_t i = scopes.size(); i-- > 0;) {
		auto scope = scopes[i];
		auto it = scope.find(name);
		if (it == scope.end()) continue;
		if (isStatic && i != 0 && !it->second->isGlobal)
			throw std::runtime_error(it->second->name + " is not static");
		return new VarNode(
			it->second,
			false
		);
	}
	return nullptr;
}

AccessNode* ClassInfo::findDeclaration(in_func, std::string& name, bool isStatic) {
	//Find static member
	{
		auto it = staticMember.find(name);
		if (it != staticMember.end()) {
			return new VarNode(
				it->second,
				false
			);
		}
		//Not found
	}
	if (declarationThis) {
		auto clazz = &compile.classes[declarationThis->classId];
		auto it = clazz->memberMap.find(name);
		if (it != clazz->memberMap.end()) {
			auto node = member[it->second];
			if (isStatic)
				throw std::runtime_error(name + " is not static");
			return new GetPropNode(
				node,
				declarationThis->classId,
				new VarNode(
					declarationThis,
					false
				),
				name,
				false
			);
		}
	}
	return nullptr;
}

FunctionInfo::~FunctionInfo() {
	for (auto node : declarationNodes) {
		delete node;
	}
}

ClassInfo::~ClassInfo() {
	
}

ParserContext::~ParserContext() {
	for (auto* node : newFunctions) {
		delete node;
	}
	for (auto* node : newClasses) {
		delete node;
	}
	for (auto* node : staticNode) {
		delete node;
	}
}

}

#endif