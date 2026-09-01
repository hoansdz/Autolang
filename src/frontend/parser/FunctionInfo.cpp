#ifndef FUNCTION_INFO_CPP
#define FUNCTION_INFO_CPP

#include "FunctionInfo.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/FunctionFlags.hpp"

namespace Autolang {

AccessNode *Scopes::findDeclaration(in_func, uint32_t line,
                                    LexerStringId nameId, bool isStatic) {
	for (size_t i = scopes.size(); i-- > 0;) {
		auto scope = scopes[i];
		auto it = scope.find(nameId);
		if (it == scope.end())
			continue;
		if (isStatic && i != 0 && !it->second->isGlobal)
			throw ParserError(line, it->second->name +
			                            " is not static\nHint: Non-static "
			                            "local variables cannot be "
			                            "accessed in a static context");
		return context.varPool.push(line, it->second, false,
		                            it->second->nullable);
	}
	return nullptr;
}

int64_t FunctionInfo::loadHash(Function *func) {
	uint64_t hash = 14695981039346656037ull; // FNV offset
	auto mix = [](uint64_t h, uint64_t val) {
		for (int j = 0; j < 8; ++j) {
			h ^= (val & 0xFF);
			h *= 1099511628211ull;
			val >>= 8;
		}
		return h;
	};
	bool isStatic = func->functionFlags & FunctionFlags::FUNC_IS_STATIC;
	if (isStatic) {
		hash = mix(hash, 488);
	}
	auto &param = parameter->parameters;
	for (size_t i = !isStatic; i < param.size(); ++i) {
		if (param[i]->classId == DefaultClass::functionClassId) {
			hash = mix(hash, param[i]->classDeclaration->loadHash());
		} else {
			hash = mix(hash, param[i]->classId);
		}
	}
	return static_cast<int64_t>(hash);
}

std::string FunctionInfo::toString(in_func) {
	auto func = compile.functions[id];
	bool isFirst = true;
	std::string result = func->getName(compile) + ": (";
	size_t startIndex =
	    (clazz && !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) ? 1
	                                                                      : 0;
	for (size_t i = startIndex; i < parameter->parameters.size(); ++i) {
		auto declaration = parameter->parameters[i];
		if (isFirst) {
			isFirst = false;
		} else {
			result += ", ";
		}
		if (declaration->classDeclaration) {
			result += declaration->name + " : " +
			          declaration->classDeclaration->getName<true>(in_data);
		} else if (declaration->classId < compile.classes.size() &&
		           compile.classes[declaration->classId]) {
			if (!declaration->name.empty()) {
				result += declaration->name + " : ";
			}
			result += compile.classes[declaration->classId]->getName(compile);
		}
	}
	result += ")->";
	if (returnClass) {
		result += returnClass->getName<true>(in_data);
	} else if (func->returnId != DefaultClass::voidClassId &&
	           func->returnId < compile.classes.size() &&
	           compile.classes[func->returnId]) {
		result += compile.classes[func->returnId]->getName(compile);
	} else {
		result += "Void";
	}
	return result;
}

FunctionInfo::~FunctionInfo() {}

} // namespace Autolang

#endif