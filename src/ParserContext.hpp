#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include <vector>
#include "Lexer.hpp"
#include "CreateNode.hpp"
#include "Interpreter.hpp"
#include "optimize/NonReallocatePool.hpp"
#include "optimize/FixedPool.hpp"

namespace AutoLang
{

	struct ClassDeclaration
	{
		std::string className;
		bool nullable = false;
	};

	struct ClassInfo
	{
		std::vector<DeclarationNode *> member;
		ankerl::unordered_dense::map<std::string, DeclarationNode *> staticMember;
		CreateConstructorNode *primaryConstructor = nullptr;
		std::vector<CreateConstructorNode *> secondaryConstructor;
		DeclarationNode *declarationThis;
		AccessNode *findDeclaration(in_func, std::string &name, bool isStatic = false);
		~ClassInfo();
	};

	struct FunctionInfo
	{
		AClass *clazz; // Context class
		Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
		std::vector<ankerl::unordered_dense::map<std::string, DeclarationNode *>> scopes;
		uint32_t declaration; //Count declaration
		BlockNode block;
		bool isConstructor = false;
		FunctionInfo() : declaration(0) { scopes.emplace_back(); }
		inline void popBackScope()
		{
			declaration -= scopes.back().size();
			scopes.pop_back();
		}
		AccessNode *findDeclaration(in_func, std::string &name, bool isStatic = false);
		~FunctionInfo();
	};

	struct ParserContext
	{
		// Optimize ram because reuse std::string instead of new std::string in lexer
		std::vector<std::string> lexerString;
		ankerl::unordered_dense::map<std::string, uint32_t> lexerStringMap;
		// Parse file to tokens
		std::vector<Lexer::Token> tokens;
		// Keywords , example public, private, static
		std::vector<Lexer::TokenType> keywords;
		// Declaration new functions by user
		std::vector<CreateFuncNode *> newFunctions;
		// Declaration new classes by user
		std::vector<CreateClassNode *> newClasses;
		uint32_t line;
		bool canBreakContinue = false;
		// Be used when it is static keywords, example static val a = ...
		bool justFindStatic = false;
		// Be used when put bytecodes with break or continue
		size_t continuePos = 0;
		size_t breakPos = 0;
		// Function information in compiler time
		ankerl::unordered_dense::map<uint32_t, FunctionInfo> functionInfo;
		// Class information in compiler time
		ankerl::unordered_dense::map<uint32_t, ClassInfo> classInfo;
		// All static variable will be here and put bytecodes to ".main" function
		std::vector<ExprNode *> staticNode;

		NonReallocatePool<DeclarationNode> declarationNodePool;
		NonReallocatePool<ReturnNode> returnPool;
		NonReallocatePool<SetNode> setValuePool;
		FixedPool<IfNode> ifPool;
		FixedPool<WhileNode> whilePool;

		std::optional<uint32_t> currentClassId = std::nullopt;
		uint32_t mainFunctionId;
		uint32_t currentFunctionId;

		// Constant value, example "null", "true", "false"
		ankerl::unordered_dense::map<std::string, std::pair<AObject *, uint32_t>> constValue;
		inline void gotoFunction(uint32_t currentFunctionId)
		{
			this->currentFunctionId = currentFunctionId;
		}
		inline void gotoClass(AClass *clazz)
		{
			if (clazz == nullptr)
			{
				currentClassId = std::nullopt;
				return;
			}
			currentClassId = clazz->id;
		}
		inline Function *getMainFunction(in_func)
		{
			return &compile.functions[mainFunctionId];
		}
		inline FunctionInfo *getMainFunctionInfo(in_func)
		{
			return &functionInfo[mainFunctionId];
		}
		inline Function *getCurrentFunction(in_func)
		{
			return &compile.functions[currentFunctionId];
		}
		inline FunctionInfo *getCurrentFunctionInfo(in_func)
		{
			return &functionInfo[currentFunctionId];
		}
		inline AClass *getCurrentClass(in_func)
		{
			return currentClassId ? &compile.classes[*currentClassId] : nullptr;
		}
		inline ClassInfo *getCurrentClassInfo(in_func)
		{
			return currentClassId ? &classInfo[*currentClassId] : nullptr;
		}
		static inline std::optional<uint32_t> getClassId(AClass *clazz)
		{
			if (!clazz)
				return std::nullopt;
			return clazz->id;
		}
		HasClassIdNode *findDeclaration(in_func, std::string &name, bool inGlobal);
		DeclarationNode *makeDeclarationNode(in_func, bool isTemp, std::string name, std::string className, bool isVal, bool isGlobal, bool nullable, bool pushToScope = true);
		~ParserContext();
	};

}

#endif