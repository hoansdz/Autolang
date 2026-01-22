#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include <vector>
#include "Lexer.hpp"
#include "CreateNode.hpp"
#include "./vm/Interpreter.hpp"
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
		AccessNode *findDeclaration(in_func, uint32_t line, std::string &name, bool isStatic = false);
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
		FunctionInfo() : declaration(0), block(0) { scopes.emplace_back(); }
		inline void popBackScope()
		{
			declaration -= scopes.back().size();
			scopes.pop_back();
		}
		AccessNode *findDeclaration(in_func, uint32_t line, std::string &name, bool isStatic = false);
		~FunctionInfo();
	};

	enum ModifierFlags : uint32_t {
		PUBLIC = 1u << 0,
		PRIVATE = 1u << 1,
		PROTECTED = 1u << 2,
		STATIC = 1u << 3,
	};

	struct ParserContext
	{
		// Optimize ram because reuse std::string instead of new std::string in lexer
		std::vector<std::string> lexerString;
		ankerl::unordered_dense::map<std::string, uint32_t> lexerStringMap;
		
		ankerl::unordered_dense::map<std::tuple<uint32_t, uint32_t, uint8_t>, uint32_t, PairHash> binaryOpResultType;
		// Parse file to tokens
		std::vector<Lexer::Token> tokens;
		// Keywords , example public, private, static
		uint32_t modifierflags = 0;
		// Declaration new functions by user
		NonReallocatePool<CreateFuncNode> newFunctions;
		// Declaration new classes by user
		NonReallocatePool<CreateClassNode> newClasses;
		uint32_t line;
		bool hasError = false;
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
		FixedPool<CreateConstructorNode> createConstructorPool;
		// NonReallocatePool<BinaryNode> binaryNodePool;
		FixedPool<IfNode> ifPool;
		FixedPool<WhileNode> whilePool;

		std::optional<uint32_t> currentClassId = std::nullopt;
		uint32_t mainFunctionId;
		uint32_t currentFunctionId;

		AVMReadFileMode* mode;

		// Constant value, example "null", "true", "false"
		ankerl::unordered_dense::map<std::string, std::pair<AObject *, uint32_t>> constValue;
		void init(CompiledProgram& compiler, AVMReadFileMode& mode);
		void logMessage(uint32_t line, const std::string& message);
		inline static std::tuple<uint32_t, uint32_t, uint8_t> makeTuple(uint32_t first, uint32_t second, uint8_t op)
		{
			return std::make_tuple(
				std::min(first, second),
				std::max(first, second),
				op);
		}
		inline void addTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t classId)
		{
			binaryOpResultType[{
				std::min(first, second),
				std::max(first, second),
				op}] = classId;
		}

		inline bool getTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t &result)
		{
			auto it = binaryOpResultType.find({std::min(first, second),
									std::max(first, second),
									op});
			if (it == binaryOpResultType.end())
				return false;
			result = it->second;
			return true;
		}

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
		HasClassIdNode *findDeclaration(in_func, uint32_t line, std::string &name, bool inGlobal);
		DeclarationNode *makeDeclarationNode(in_func, uint32_t line, bool isTemp, std::string name, std::string className, bool isVal, bool isGlobal, bool nullable, bool pushToScope = true);
		inline bool getBoolConstValuePosition(bool b) { return b ? 1 : 2; }
		~ParserContext();
	};

}

#endif