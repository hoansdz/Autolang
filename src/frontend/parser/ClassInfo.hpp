#ifndef CLASS_INFO_HPP
#define CLASS_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include <vector>

namespace AutoLang {

struct GenericData {
	HashMap<DeclarationOffset, DeclarationOffset>
	    newPositionOfStaticDeclaration;
	HashMap<ClassDeclaration *, ExprNode *> mustRenameNodes;
	std::vector<GenericDeclarationNode *> genericDeclarations;
	std::vector<std::pair<DeclarationNode *, HasClassIdNode *>>
	    staticDeclaration;
	HashMap<LexerStringId, Offset> genericDeclarationMap;
	inline GenericDeclarationNode *findDeclaration(LexerStringId nameId) {
		auto it = genericDeclarationMap.find(nameId);
		if (it == genericDeclarationMap.end()) {
			return nullptr;
		}
		return genericDeclarations[it->second];
	}
	~GenericData() {
		for (auto declaration : genericDeclarations) {
			delete declaration;
		}
	}
};

struct ClassInfo {
	ClassId baseClassId;
	GenericData *genericData = nullptr;
	std::vector<DeclarationNode *> allDeclarationNode;
	std::vector<DeclarationNode *> member;
	HashMap<LexerStringId, MemberOffset> memberMap;
	HashMap<LexerStringId, DeclarationNode *> staticMember;
	HashMap<LexerStringId, std::vector<FunctionId>> allFunction;
	HashMap<LexerStringId, HashMap<HashValue, FunctionId>> func;
	HashMap<LexerStringId, HashMap<HashValue, FunctionId>> staticFunc;
	HashMap<LexerStringId, ConstValueNode *> constValue;
	std::vector<ClassDeclaration *> genericTypeId;
	CreateConstructorNode *primaryConstructor = nullptr;
	std::vector<CreateConstructorNode *> secondaryConstructor;
	std::vector<CreateFuncNode *> createFunctionNodes;
	DeclarationNode *declarationThis;
	ClassId parent;

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

} // namespace AutoLang

#endif