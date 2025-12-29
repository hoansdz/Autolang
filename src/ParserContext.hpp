#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include <vector>
#include "Lexer.hpp"
#include "CreateNode.hpp"
#include "Interpreter.hpp"

namespace AutoLang {

struct ClassDeclaration {
	std::string className;
	bool nullable = false;
};

struct ClassInfo {
	std::vector<DeclarationNode*> member;
	std::unordered_map<std::string, DeclarationNode*> staticMember;
	CreateConstructorNode* primaryConstructor = nullptr;
	std::vector<CreateConstructorNode*> secondaryConstructor;
	DeclarationNode* declarationThis;
	AccessNode* findDeclaration(in_func, std::string& name, bool isStatic = false);
	~ClassInfo();
};

struct FunctionInfo {
	AClass* clazz; //Context class
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
	//Optimize ram because reuse std::string instead of new std::string in lexer
	std::vector<std::string> lexerString;
	std::unordered_map<std::string, uint32_t> lexerStringMap;
	//Parse file to tokens
	std::vector<Lexer::Token> tokens;
	//Keywords , example public, private, static
	std::vector<Lexer::TokenType> keywords;
	//Declaration new functions by user
	std::vector<CreateFuncNode*> newFunctions;
	//Declaration new classes by user
	std::vector<CreateClassNode*> newClasses;
	uint32_t line;
	bool canBreakContinue = false;
	//Be used when it is static keywords, example static val a = ...
	bool justFindStatic = false;
	//Be used when put bytecodes with break or continue
	size_t continuePos = 0;
	size_t breakPos = 0;
	//Function information in compiler time
	std::unordered_map<Function*, FunctionInfo> functionInfo;
	//Class information in compiler time
	std::unordered_map<uint32_t, ClassInfo> classInfo;
	//All static variable will be here and put bytecodes to ".main" function
	std::vector<ExprNode*> staticNode;

	AClass* currentClass = nullptr;
	ClassInfo* currentClassInfo;
	Function *mainFunction;
	FunctionInfo *mainFuncInfo;
	Function *currentFunction;
	FunctionInfo *currentFuncInfo;

	//Constant value, example "null", "true", "false"
	std::unordered_map<std::string, std::pair<AObject*, uint32_t>> constValue;
	inline void gotoFunction(Function* func) {
		currentFunction = func;
		currentFuncInfo = &functionInfo[func];
	}
	inline void gotoClass(AClass* clazz) {
		currentClass = clazz;
		currentClassInfo = clazz ? &classInfo[clazz->id] : nullptr;
	}
	inline std::optional<uint32_t> getCurrentContextClassId() {
		if (!currentClass) return std::nullopt;
		return currentClass->id;
	}
	static inline std::optional<uint32_t> getClassId(AClass* clazz) {
		if (!clazz) return std::nullopt;
		return clazz->id;
	}
	HasClassIdNode* findDeclaration(in_func, std::string& name, bool inGlobal);
	DeclarationNode* makeDeclarationNode(bool isTemp, std::string name, std::string className, bool isVal, bool isGlobal, bool nullable, bool pushToScope = true);
	~ParserContext();
};

}

#endif