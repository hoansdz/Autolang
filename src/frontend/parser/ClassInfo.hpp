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
	HashMap<ClassDeclaration *, ExprNode *> mustRenameNodes;
	std::vector<GenericDeclarationNode *> genericDeclarations;
	HashMap<LexerStringId, Offset> genericDeclarationMap;
};

struct ClassInfo {
	ClassId baseClassId;
	GenericData *genericData = nullptr;
	std::vector<DeclarationNode *> allDeclarationNode;
	std::vector<DeclarationNode *> member;
	HashMap<std::string, DeclarationNode *> staticMember;
	HashMap<std::string, HashMap<HashValue, Offset>> func;
	HashMap<std::string, HashMap<HashValue, Offset>> staticFunc;
	std::vector<ClassDeclaration *> genericTypeId;
	CreateConstructorNode *primaryConstructor = nullptr;
	std::vector<CreateConstructorNode *> secondaryConstructor;
	std::vector<CreateFuncNode *> createFunctionNodes;
	DeclarationNode *declarationThis;
	ClassId parent;

	AccessNode *findDeclaration(in_func, uint32_t line, std::string &name,
	                            bool isStatic = false);
	GenericDeclarationNode *findGenericDeclaration(LexerStringId nameId) {
		if (!genericData)
			return nullptr;
		auto it = genericData->genericDeclarationMap.find(nameId);
		if (it == genericData->genericDeclarationMap.end()) {
			return nullptr;
		}
		return genericData->genericDeclarations[it->second];
	}
	~ClassInfo();
};

} // namespace AutoLang

#endif