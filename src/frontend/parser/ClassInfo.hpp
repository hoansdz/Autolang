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
	inline GenericDeclarationNode *findDeclaration(LexerStringId nameId) {
		auto it = genericDeclarationMap.find(nameId);
		if (it == genericDeclarationMap.end()) {
			return nullptr;
		}
		return genericDeclarations[it->second];
	}
};

struct ClassInfo {
	ClassId baseClassId;
	GenericData *genericData = nullptr;
	std::vector<DeclarationNode *> allDeclarationNode;
	std::vector<DeclarationNode *> member;
	HashMap<std::string, DeclarationNode *> staticMember;
	HashMap<std::string, HashMap<HashValue, FunctionId>> func;
	HashMap<std::string, HashMap<HashValue, FunctionId>> staticFunc;
	HashMap<LexerStringId, ConstValueNode *> constValue;
	std::vector<ClassDeclaration *> genericTypeId;
	CreateConstructorNode *primaryConstructor = nullptr;
	std::vector<CreateConstructorNode *> secondaryConstructor;
	std::vector<CreateFuncNode *> createFunctionNodes;
	DeclarationNode *declarationThis;
	ClassId parent;

	AccessNode *findDeclaration(in_func, uint32_t line, const std::string &name,
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