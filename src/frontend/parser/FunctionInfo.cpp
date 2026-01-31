#ifndef FUNCTION_INFO_CPP
#define FUNCTION_INFO_CPP

#include "FunctionInfo.hpp"

namespace AutoLang {

AccessNode *FunctionInfo::findDeclaration(in_func, uint32_t line,
                                          std::string &name, bool isStatic) {
	for (size_t i = scopes.size(); i-- > 0;) {
		auto scope = scopes[i];
		auto it = scope.find(name);
		if (it == scope.end())
			continue;
		if (isStatic && i != 0 && !it->second->isGlobal)
			throw ParserError(line, it->second->name + " is not static");
		return new VarNode(line, it->second, false, it->second->nullable);
	}
	return nullptr;
}

FunctionInfo::~FunctionInfo() {}

} // namespace AutoLang

#endif