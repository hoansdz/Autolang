#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include <vector>
#include "Lexer.hpp"
#include "CreateNode.hpp"
#include "Interpreter.hpp"

namespace AutoLang {

struct ClassInfo {
	std::vector<DeclarationNode*> member;
	std::unordered_map<std::string, DeclarationNode*> staticMember;
	std::unordered_map<std::string, std::vector<uint32_t>> vtable;
	CreateConstructorNode* primaryConstructor = nullptr;
	std::vector<CreateConstructorNode*> secondaryConstructor;
	DeclarationNode* declarationThis;
	AccessNode* findDeclaration(in_func, std::string& name, bool isStatic = false);
	~ClassInfo();
};

struct FunctionInfo {
	AClass* clazz;
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	std::vector<std::unordered_map<std::string, DeclarationNode*>> scopes;
	std::vector<DeclarationNode*> declarationNodes;
	uint32_t declaration;
	BlockNode block;
	bool isConstructor = false;
	FunctionInfo():
		declaration(0){ scopes.emplace_back(); }
	inline void popBackScope(){
		declaration -= scopes.back().size();
		scopes.pop_back();
	}
	AccessNode* findDeclaration(in_func, std::string& name, bool isStatic = false);
	~FunctionInfo();
}; 

struct ParserContext {
	std::vector<std::string> lexerString;
	std::unordered_map<std::string, uint32_t> lexerStringMap;
	std::vector<Lexer::Token> tokens;
	std::vector<Lexer::TokenType> keywords;
	std::vector<CreateFuncNode*> newFunctions;
	std::vector<CreateClassNode*> newClasses;
	int line;
	bool canBreakContinue = false;
	bool justFindStatic = false;
	size_t continuePos = 0;
	size_t breakPos = 0;
	std::unordered_map<Function*, FunctionInfo> functionInfo;
	std::unordered_map<AClass*, ClassInfo> classInfo;
	std::vector<ExprNode*> staticNode;
	AClass* currentClass = nullptr;
	ClassInfo* currentClassInfo;
	Function *mainFunction;
	FunctionInfo *mainFuncInfo;
	Function *currentFunction;
	FunctionInfo *currentFuncInfo;
	std::unordered_map<std::string, std::pair<AObject*, uint32_t>> constValue;
	inline void gotoFunction(Function* func) {
		currentFunction = func;
		currentFuncInfo = &functionInfo[func];
	}
	inline void gotoClass(AClass* clazz) {
		currentClass = clazz;
		currentClassInfo = clazz ? &classInfo[clazz] : nullptr;
	}
	HasClassIdNode* findDeclaration(in_func, std::string& name, bool inGlobal);
	DeclarationNode* makeDeclarationNode(bool isTemp, std::string name, std::string className, bool isVal, bool isGlobal, bool pushToScope = true);
	~ParserContext();
};

}

#endif