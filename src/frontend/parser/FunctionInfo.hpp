#ifndef FUNCTION_INFO_HPP
#define FUNCTION_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include "frontend/parser/node/Node.hpp"
#include <vector>

namespace AutoLang {

struct FunctionInfo {
	AClass *clazz; // Context class
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	std::vector<HashMap<std::string, DeclarationNode *>>
	    scopes;
	uint32_t declaration; // Count declaration
	BlockNode block;
	bool isConstructor = false;
	bool isVirtual = false;
	Offset virtualPosition;
	int64_t hash;
	FunctionInfo() : declaration(0), block(0) { scopes.emplace_back(); }
	void loadHash();
	inline void popBackScope() {
		declaration -= scopes.back().size();
		scopes.pop_back();
	}
	AccessNode *findDeclaration(in_func, uint32_t line, std::string &name,
	                            bool isStatic = false);
	~FunctionInfo();
};

} // namespace AutoLang

#endif