#ifndef FUNCTION_INFO_HPP
#define FUNCTION_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include <vector>

namespace AutoLang {

struct FunctionInfo {
	AClass *clazz; // Context class
	GenericData *genericData = nullptr;
	std::vector<HashMap<std::string, DeclarationNode *>> scopes;
	uint32_t declaration; // Count declaration
	BlockNode block;
	bool *nullableArgs = nullptr;
	ReturnNode *inferenceNode = nullptr;
	std::vector<ClassDeclaration *> genericTypeId;
	std::vector<std::pair<DeclarationNode *, HasClassIdNode *>>
	    staticDeclaration;
	Offset virtualPosition;
	int64_t hash;
	FunctionInfo() : declaration(0), block(0) { scopes.emplace_back(); }
	void loadHash();
	inline void popBackScope() {
		declaration -= scopes.back().size();
		scopes.pop_back();
	}
	inline GenericDeclarationNode *
	findGenericDeclaration(LexerStringId nameId) {
		if (!genericData)
			return nullptr;
		return genericData->findDeclaration(nameId);
	}
	AccessNode *findDeclaration(in_func, uint32_t line, const std::string &name,
	                            bool isStatic = false);
	~FunctionInfo();
};

} // namespace AutoLang

#endif