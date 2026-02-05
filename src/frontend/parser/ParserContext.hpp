#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/ClassInfo.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/structure/NonReallocatePool.hpp"
#include "shared/FixedPool.hpp"
#include <vector>

namespace AutoLang {

struct ClassDeclaration {
	std::string className;
	bool nullable = false;
};

enum ModifierFlags : uint32_t {
	MF_PUBLIC = 1u << 0,
	MF_PRIVATE = 1u << 1,
	MF_PROTECTED = 1u << 2,
	MF_STATIC = 1u << 3,
};

enum AnnotationFlags : uint32_t {
	AN_OVERRIDE = 1u << 0,
	AN_NO_OVERRIDE = 1u << 1,
	AN_NATIVE = 1u << 2,
	AN_NO_CONSTRUCTOR = 1u << 3,
};

struct SourceChunk {
	uint32_t end;
	AVMReadFileMode mode;
};

struct ParserContext {
	// Optimize ram because reuse std::string instead of new std::string in
	// lexer
	std::vector<std::string> lexerString;
	HashMap<std::string, LexerStringId> lexerStringMap;

	HashMap<std::tuple<ClassId, ClassId, uint8_t>, ClassId,
	                             PairHash>
	    binaryOpResultType;
	// Parse file to tokens
	std::vector<Lexer::Token> tokens;
	std::vector<SourceChunk> sources;
	uint32_t sourcePos;
	SourceChunk* currentChunk = nullptr;
	// Keywords , example public, private, static
	uint32_t modifierflags = 0;
	// Anotations
	uint32_t annotationFlags = 0;
	HashMap<AnnotationFlags, Lexer::Token> annotationMetadata;
	// Declaration new functions by user
	NonReallocatePool<CreateFuncNode> newFunctions;
	// Declaration new classes by user
	NonReallocatePool<CreateClassNode> newClasses;

	HashMap<uint32_t, CreateClassNode *> newClassesMap;

	bool hasError = false;
	bool canBreakContinue = false;
	// Be used when it is static keywords, example static val a = ...
	bool justFindStatic = false;
	// Be used when put bytecodes with break or continue
	uint32_t continuePos = 0;
	uint32_t breakPos = 0;
	size_t currentTokenPos = 0;
	JumpIfNullNode *jumpIfNullNode = nullptr;
	// Function information in compiler time
	HashMap<uint32_t, FunctionInfo> functionInfo;
	// Class information in compiler time
	HashMap<uint32_t, ClassInfo> classInfo;
	// All static variable will be here and put bytecodes to ".main" function
	std::vector<ExprNode *> staticNode;

	NonReallocatePool<DeclarationNode> declarationNodePool;
	NonReallocatePool<ReturnNode> returnPool;
	NonReallocatePool<SetNode> setValuePool;
	FixedPool<CreateConstructorNode> createConstructorPool;
	// NonReallocatePool<BinaryNode> binaryNodePool;
	FixedPool<IfNode> ifPool;
	FixedPool<WhileNode> whilePool;

	std::optional<ClassId> currentClassId = std::nullopt;
	uint32_t mainFunctionId;
	uint32_t currentFunctionId;

	static AVMReadFileMode *mode;

	HashMap<int64_t, Offset> constIntMap;
	HashMap<double, Offset> constFloatMap;
	HashMap<AString *, Offset, AString::Hash, AString::Equal> constStringMap;

	// Constant value, example "null", "true", "false"
	HashMap<std::string, std::pair<AObject *, uint32_t>>
	    constValue;
	void init(CompiledProgram &compiler);
	void logMessage(uint32_t line, const std::string &message);
	void warning(uint32_t line, const std::string &message);
	void refresh(CompiledProgram &compile);
	inline static std::tuple<ClassId, ClassId, uint8_t>
	makeTuple(ClassId first, ClassId second, uint8_t op) {
		return std::make_tuple(std::min(first, second), std::max(first, second),
		                       op);
	}
	inline void addTypeResult(ClassId first, ClassId second, uint8_t op,
	                          ClassId classId) {
		binaryOpResultType[{std::min(first, second), std::max(first, second),
		                    op}] = classId;
	}

	inline bool getTypeResult(ClassId first, ClassId second, uint8_t op,
	                          ClassId &result) {
		auto it = binaryOpResultType.find(
		    {std::min(first, second), std::max(first, second), op});
		if (it == binaryOpResultType.end())
			return false;
		result = it->second;
		return true;
	}

	inline void gotoFunction(uint32_t currentFunctionId) {
		this->currentFunctionId = currentFunctionId;
	}
	inline void gotoClass(AClass *clazz) {
		if (clazz == nullptr) {
			currentClassId = std::nullopt;
			return;
		}
		currentClassId = clazz->id;
	}
	inline Function *getMainFunction(in_func) {
		return compile.functions[mainFunctionId];
	}
	inline FunctionInfo *getMainFunctionInfo(in_func) {
		return &functionInfo[mainFunctionId];
	}
	inline Function *getCurrentFunction(in_func) {
		return compile.functions[currentFunctionId];
	}
	inline FunctionInfo *getCurrentFunctionInfo(in_func) {
		return &functionInfo[currentFunctionId];
	}
	inline AClass *getCurrentClass(in_func) {
		return currentClassId ? compile.classes[*currentClassId] : nullptr;
	}
	inline ClassInfo *getCurrentClassInfo(in_func) {
		return currentClassId ? &classInfo[*currentClassId] : nullptr;
	}
	static inline std::optional<uint32_t> getClassId(AClass *clazz) {
		if (!clazz)
			return std::nullopt;
		return clazz->id;
	}
	HasClassIdNode *findDeclaration(in_func, uint32_t line, std::string &name,
	                                bool inGlobal);
	DeclarationNode *makeDeclarationNode(in_func, uint32_t line, bool isTemp,
	                                     std::string name,
	                                     std::string className, bool isVal,
	                                     bool isGlobal, bool nullable,
	                                     bool pushToScope = true);
	inline bool getBoolConstValuePosition(bool b) { return b ? 1 : 2; }
	~ParserContext();
};

} // namespace AutoLang

#endif