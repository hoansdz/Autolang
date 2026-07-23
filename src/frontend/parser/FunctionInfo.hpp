#ifndef FUNCTION_INFO_HPP
#define FUNCTION_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/GenericData.hpp"
#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include <vector>

namespace Autolang {

struct FunctionInfo {
	AClass *clazz; // Context class
	GenericData *genericData = nullptr;
	Scopes scopes;
	BlockNode body;
	ReturnNode *inferenceNode = nullptr;
	Parameter *parameter;
	ClassDeclaration *returnClass;
	std::vector<ClassDeclaration *> genericTypeId;
	HashMap<DeclarationNode *, DeclarationNode *> reflectDeclarationMap;
	Offset virtualPosition;
	uint32_t id;
	uint32_t declaration; // Count declaration
	uint32_t tokenIndex;
	int64_t hash;
	FunctionInfo() : body(0), declaration(0) {}
	int64_t loadHash(Function *func);
	inline void popBackScope() {
		declaration -= scopes.back().size();
		scopes.pop();
	}
	std::string toString(in_func);
	inline GenericDeclarationNode *
	findGenericDeclaration(LexerStringId nameId) {
		if (!genericData)
			return nullptr;
		return genericData->findDeclaration(nameId);
	}
	inline AccessNode *findDeclaration(in_func, uint32_t line,
	                                   LexerStringId nameId,
	                                   bool isStatic = false) {
		return scopes.findDeclaration(in_data, line, nameId, isStatic);
	}
	~FunctionInfo();
};

} // namespace Autolang

#endif