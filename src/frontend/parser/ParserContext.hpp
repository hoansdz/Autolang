#ifndef PARSER_CONTEXT_HPP
#define PARSER_CONTEXT_HPP

#include "backend/vm/AVM.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ClassInfo.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/structure/NonReallocatePool.hpp"
#include "shared/ChunkArena.hpp"
#include "shared/FixedPool.hpp"
#include "frontend/parser/OperatorId.hpp"
#include <set>
#include <vector>

namespace AutoLang {

enum ModifierFlags : uint32_t {
	MF_PUBLIC = 1u << 0,
	MF_PRIVATE = 1u << 1,
	MF_PROTECTED = 1u << 2,
	MF_STATIC = 1u << 3,
	MF_LATEINIT = 1u << 4,
};

enum AnnotationFlags : uint32_t {
	AN_OVERRIDE = 1u << 0,
	AN_NO_OVERRIDE = 1u << 1,
	AN_NATIVE = 1u << 2,
	AN_NO_CONSTRUCTOR = 1u << 3,
	AN_WAIT_INPUT = 1u << 4,
	AN_NO_EXTENDS = 1u << 5,
	AN_NATIVE_DATA = 1u << 6,
};

struct LibraryData;

constexpr LexerStringId lexerIdsuper = 0;
constexpr LexerStringId lexerIdInt = 1;
constexpr LexerStringId lexerIdFloat = 2;
constexpr LexerStringId lexerIdBool = 3;
constexpr LexerStringId lexerIdNull = 4;
constexpr LexerStringId lexerIdVoid = 5;
constexpr LexerStringId lexerIdnull = 6;
constexpr LexerStringId lexerIdtrue = 7;
constexpr LexerStringId lexerIdfalse = 8;
constexpr LexerStringId lexerId__FILE__ = 9;
constexpr LexerStringId lexerId__LINE__ = 10;
constexpr LexerStringId lexerId__FUNC__ = 11;
constexpr LexerStringId lexerId__CLASS__ = 12;
constexpr LexerStringId lexerIdgetClassId = 13;

struct ParserContext {
	// Optimize ram because reuse std::string instead of new std::string in
	// lexer
	std::vector<std::string> lexerString;
	HashMap<std::string, LexerStringId> lexerStringMap;

	Lexer::Context *mainLexerContext;
	HashMap<std::string, LibraryData *> importMap;
	std::vector<LibraryData *> loadingLibs;

	HashMap<Lexer::TokenType, OperatorId> operatorTable;

	HashMap<std::tuple<ClassId, ClassId, uint8_t>, ClassId, PairHash>
	    binaryOpResultType;
	// Parse file to tokens
	std::vector<Lexer::Token> tokens;
	// Keywords , example public, private, static
	uint32_t modifierflags = 0;
	// Anotations
	uint32_t annotationFlags = 0;
	HashMap<AnnotationFlags, Lexer::Token> annotationMetadata;
	// Declaration new functions by user
	NonReallocatePool<CreateFuncNode> newFunctions;
	std::vector<FunctionId> mustInferenceFunctionType;
	// Declaration new classes by user
	NonReallocatePool<CreateClassNode> newClasses;
	HashMap<LexerStringId, ClassId> defaultClassMap;

	HashMap<uint32_t, CreateClassNode *> newDefaultClassesMap;
	HashMap<uint32_t, CreateClassNode *> newGenericClassesMap;
	ChunkArena<ClassDeclaration, 64> classDeclarationAllocator;
	std::vector<ClassDeclaration *> allClassDeclarations;

	bool hasError = false;
	bool canBreakContinue = false;
	// Be used when it is static keywords, example static val a = ...
	bool justFindStatic = false;
	// Be used when put bytecodes with break or continue
	uint32_t continuePos = 0;
	uint32_t breakPos = 0;
	size_t currentTokenPos = 0;
	JumpIfNullNode *jumpIfNullNode = nullptr;
	IfNode *mustReturnValueNode = nullptr;
	// Function information in compiler time
	ChunkArena<FunctionInfo, 64> functionInfoAllocator;
	std::vector<FunctionInfo *> functionInfo;
	// Class information in compiler time
	ChunkArena<ClassInfo, 64> classInfoAllocator;
	std::vector<ClassInfo *> classInfo;
	// All static variable will be here and put bytecodes to ".main" function
	std::vector<ExprNode *> staticNode;

	NonReallocatePool<DeclarationNode> declarationNodePool;
	ChunkArena<ReturnNode, 128> returnPool;
	ChunkArena<SetNode, 128> setValuePool;
	ChunkArena<CreateConstructorNode, 64> createConstructorPool;
	ChunkArena<BinaryNode, 128> binaryNodePool;
	ChunkArena<IfNode, 128> ifPool;
	ChunkArena<WhileNode, 64> whilePool;
	ChunkArena<TryCatchNode, 64> tryCatchPool;
	ChunkArena<ThrowNode, 64> throwPool;
	ChunkArena<CastNode, 64> castPool;
	ChunkArena<RuntimeCastNode, 64> runtimeCastPool;
	ChunkArena<VarNode, 128> varPool;
	ChunkArena<GetPropNode, 128> getPropPool;
	ChunkArena<UnknowNode, 128> unknowNodePool;
	ChunkArena<OptionalAccessNode, 64> optionalAccessNodePool;
	ChunkArena<NullCoalescingNode, 64> nullCoalescingPool;
	ChunkArena<BlockNode, 64> blockNodePool;
	ChunkArena<CallNode, 64> callNodePool;
	ChunkArena<ClassAccessNode, 64> classAccessPool;
	ChunkArena<ConstValueNode, 128> constValuePool;
	ChunkArena<UnaryNode, 64> unaryNodePool;
	ChunkArena<ForNode, 64> forPool;
	ChunkArena<RangeNode, 32> rangeNode;

	std::optional<ClassId> currentClassId = std::nullopt;
	uint32_t mainFunctionId;
	uint32_t currentFunctionId;

	static LibraryData *mode;

	HashMap<int64_t, Offset> constIntMap;
	HashMap<double, Offset> constFloatMap;
	HashMap<AString *, Offset, AString::Hash, AString::Equal> constStringMap;

	// Constant value, example "null", "true", "false"
	HashMap<LexerStringId, ConstValueNode *> constValue;
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
		return functionInfo[mainFunctionId];
	}
	inline Function *getCurrentFunction(in_func) {
		return compile.functions[currentFunctionId];
	}
	inline FunctionInfo *getCurrentFunctionInfo(in_func) {
		return functionInfo[currentFunctionId];
	}
	inline AClass *getCurrentClass(in_func) {
		return currentClassId ? compile.classes[*currentClassId] : nullptr;
	}
	inline ClassInfo *getCurrentClassInfo(in_func) {
		return currentClassId ? classInfo[*currentClassId] : nullptr;
	}
	static inline std::optional<uint32_t> getClassId(AClass *clazz) {
		if (!clazz)
			return std::nullopt;
		return clazz->id;
	}
	CreateClassNode *findCreateClassNode(uint32_t id) {
		{
			auto it = newDefaultClassesMap.find(id);
			if (it != newDefaultClassesMap.end()) {
				return it->second;
			}
		}
		{
			auto it = newGenericClassesMap.find(id);
			if (it != newGenericClassesMap.end()) {
				return it->second;
			}
		}
		return nullptr;
	}
	HasClassIdNode *findDeclaration(in_func, uint32_t line, std::string &name,
	                                bool inGlobal);
	DeclarationNode *
	makeDeclarationNode(in_func, uint32_t line, bool isTemp, std::string name,
	                    ClassDeclaration *classDeclaration, bool isVal,
	                    bool isGlobal, bool nullable, bool pushToScope = true);
	inline size_t getBoolConstValuePosition(bool b) { return b ? 1 : 2; }
	~ParserContext();
};

} // namespace AutoLang

#endif