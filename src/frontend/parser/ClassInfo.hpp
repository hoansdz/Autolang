#ifndef CLASS_INFO_HPP
#define CLASS_INFO_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include <vector>

namespace AutoLang {

struct ClassInfo {
	std::vector<DeclarationNode *> member;
	HashMap<std::string, DeclarationNode *> staticMember;
	HashMap<std::string, HashMap<HashValue, Offset>> func;
	HashMap<std::string, HashMap<HashValue, Offset>> staticFunc;
	HashMap<std::string, GenericDeclarationNode*> genericDeclarations;
	CreateConstructorNode *primaryConstructor = nullptr;
	std::vector<CreateConstructorNode *> secondaryConstructor;
	DeclarationNode *declarationThis;
	ClassId parent;
	AccessNode *findDeclaration(in_func, uint32_t line, std::string &name,
	                            bool isStatic = false);
	~ClassInfo();
};

} // namespace AutoLang

#endif