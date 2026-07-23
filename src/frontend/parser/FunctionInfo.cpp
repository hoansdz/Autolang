#ifndef FUNCTION_INFO_CPP
#define FUNCTION_INFO_CPP

#include "FunctionInfo.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

AccessNode *Scopes::findDeclaration(in_func, uint32_t line,
                                    LexerStringId nameId, bool isStatic) {
	for (size_t i = scopes.size(); i-- > 0;) {
		auto scope = scopes[i];
		auto it = scope.find(nameId);
		if (it == scope.end())
			continue;
		if (isStatic && i != 0 && !it->second->isGlobal)
			throw ParserError(line, it->second->name + " is not static");
		return context.varPool.push(line, it->second, false,
		                            it->second->nullable);
	}
	return nullptr;
}

int64_t FunctionInfo::loadHash(Function *func) {
	int64_t hash = 1469598103934665603ull; // FNV offset
	bool isStatic = func->functionFlags & FunctionFlags::FUNC_IS_STATIC;
	if (isStatic) {
		hash ^= 488;
		hash *= 1099511628211ull;
	}
	auto &param = parameter->parameters;
	for (size_t i = !isStatic; i < param.size(); ++i) {
		if (param[i]->classId == DefaultClass::functionClassId) {
			hash ^= param[i]->classDeclaration->loadHash();
		} else {
			hash ^= param[i]->classId;
		}
		hash *= 1099511628211ull;
	}
	return hash;
}

std::string FunctionInfo::toString(in_func) {
	auto func = compile.functions[id];
	bool isFirst = true;
	std::string result =
	    "[" + std::to_string(id) + "] " + func->getName(compile) + ": (";
	for (auto declaration : parameter->parameters) {
		if (isFirst) {
			isFirst = false;
		} else {
			result += ", ";
		}
		if (declaration->classDeclaration) {
			result += declaration->name + " : " +
			          declaration->classDeclaration->getName<true>(in_data);
		} else {
			result += compile.classes[declaration->classId]->getName(compile);
		}
	}
	result += ")->";
	if (returnClass) {
		result += returnClass->getName<true>(in_data);
	} else {
		result += "Void";
	}
	return result;
}

FunctionInfo::~FunctionInfo() {}

} // namespace Autolang

#endif