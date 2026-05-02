#ifndef FUNCTION_INFO_CPP
#define FUNCTION_INFO_CPP

#include "FunctionInfo.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

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

FunctionInfo::~FunctionInfo() {}

} // namespace AutoLang

#endif