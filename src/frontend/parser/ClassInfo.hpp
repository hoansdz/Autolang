#ifndef CLASS_INFO_HPP
#define CLASS_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/parser/GenericData.hpp"
#include "shared/SmallVector.hpp"
#include <vector>

namespace Autolang {

struct ClassInfo {
	GenericData *genericData = nullptr;
	SmallVector<DeclarationNode *, 8> allDeclarationNode;
	SmallVector<DeclarationNode *, 8> member;
	HashMap<LexerStringId, MemberOffset> memberMap;
	HashMap<LexerStringId, DeclarationNode *> staticMember;
	HashMap<LexerStringId, std::vector<FunctionId>> allFunction;
	HashMap<LexerStringId, HashMap<HashValue, FunctionId>> func;
	HashMap<LexerStringId, HashMap<HashValue, FunctionId>> staticFunc;
	HashMap<LexerStringId, ConstValueNode *> constValue;
	SmallVector<ClassDeclaration *, 2> genericTypeId;
	CreateConstructorNode *primaryConstructor = nullptr;
	SmallVector<CreateConstructorNode *, 2> secondaryConstructor;
	SmallVector<CreateFuncNode *, 8> createFunctionNodes;
	DeclarationNode *declarationThis = nullptr;

	AccessNode *findDeclaration(in_func, uint32_t line, LexerStringId nameId,
	                            bool isStatic = false);
	DeclarationNode *findAllMember(in_func, uint32_t line, LexerStringId nameId,
	                               bool isStatic = false);
	inline GenericDeclarationNode *
	findGenericDeclaration(LexerStringId nameId) {
		if (!genericData)
			return nullptr;
		return genericData->findDeclaration(nameId);
	}
	~ClassInfo();
};

} // namespace Autolang

#endif