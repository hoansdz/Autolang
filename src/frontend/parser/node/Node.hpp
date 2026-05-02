#ifndef NODE_HPP
#define NODE_HPP

#include "backend/vm/Opcode.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/Parameter.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/Type.hpp"
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace AutoLang {

struct ParserContext;
struct DeclarationNode;
struct ParserError;

inline void put_opcode_u32(std::vector<uint8_t> &code, uint32_t value) {
	code.push_back((value >> 0) & 0xFF);
	code.push_back((value >> 8) & 0xFF);
	code.push_back((value >> 16) & 0xFF);
	code.push_back((value >> 24) & 0xFF);
}

inline void rewrite_opcode_u32(uint8_t *code, uint32_t pos, uint32_t value) {
	code[pos] = (value >> 0) & 0xFF;
	code[pos + 1] = (value >> 8) & 0xFF;
	code[pos + 2] = (value >> 16) & 0xFF;
	code[pos + 3] = (value >> 24) & 0xFF;
}

enum NodeType : uint8_t {
	UNKNOW,
	VAR,
	BINARY,
	CONST,
	GET_PROP,
	DECLARATION,
	CALL,
	CAST,
	SET,
	UNARY,
	IF,
	BLOCK,
	WHILE,
	FOR,
	CREATE_FUNC,
	CREATE_CLASS,
	CREATE_CONSTRUCTOR,
	RET,
	SKIP,
	CLASS_ACCESS,
	FUNCTION_ACCESS,
	OPTIONAL_ACCESS,
	NULL_COALESCING,
	TRY_CATCH,
	THROW,
	RUNTIME_CAST,
	GENERIC_DECLARATION,
	RANGE,
	CREATE_ARRAY,
	CREATE_SET,
	CREATE_MAP,
	CREATE_CLOSURE,
	CREATE_ENUM_VALUE,
	WHEN,
};

struct DeclarationNode;
struct AccessNode;

struct Scopes {
	std::vector<HashMap<LexerStringId, DeclarationNode *>> scopes;
	inline void pop() { scopes.pop_back(); }
	inline HashMap<LexerStringId, DeclarationNode *> &back() {
		return scopes.back();
	}
	inline HashMap<LexerStringId, DeclarationNode *> &operator[](size_t index) {
		return scopes[index];
	}
	inline void emplace_back() { scopes.emplace_back(); }
	AccessNode *findDeclaration(in_func, uint32_t line, LexerStringId nameId,
	                            bool isStatic = false);
	Scopes() { scopes.emplace_back(); }
};

struct ExprNode {
	LibraryData *mode;
	uint32_t line;
	NodeType kind;
	ExprNode(NodeType kind, uint32_t line = 0);
	[[noreturn]] inline void throwError(std::string message);
	void warning(in_func, std::string message);
	static inline void deleteNode(ExprNode *node) {};
	std::string getNodeType();
	bool canCast(in_func, ClassDeclaration *from, ClassDeclaration *to);
	bool canCast(in_func, ClassId from, ClassId to);
	virtual ExprNode *resolve(in_func) { return this; }
	virtual void optimize(in_func) {}
	virtual void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {};
	virtual void rewrite(in_func, uint8_t *bytecodes) {}
	virtual ExprNode *copy(in_func) { return nullptr; }
	virtual ~ExprNode() {}
};

struct SkipNode : ExprNode {
	Lexer::TokenType type;
	BytecodePos jumpBytePos;
	SkipNode(Lexer::TokenType type, uint32_t line)
	    : ExprNode(NodeType::SKIP, line), type(type) {}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
};

struct BlockNode : ExprNode {
	std::vector<ExprNode *> nodes;
	BlockNode(uint32_t line) : ExprNode(NodeType::BLOCK, line) {}
	static void loadReturnValueClassId(in_func, uint32_t line,
	                                   std::optional<ClassId> &currentClassId,
	                                   ClassId newClassId);
	template <bool optimize = true>
	inline void loadClassNode(in_func, ExprNode *node,
	                          std::optional<ClassId> &currentClassId,
	                          bool &nullable, bool &isStatic,
	                          ClassDeclaration *&newClassDeclaration);
	inline void loadClassAndOptimize(in_func);
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *resolve(in_func) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	void refresh();
	inline void addJumpPosition(in_func, BytecodePos pos);
	ExprNode *copy(in_func) override;
	~BlockNode();
};

struct CanBreakContinueNode : ExprNode {
	BytecodePos continuePos = 0; // Support "continue" command
	BytecodePos breakPos = 0;    // Support "break" command
	BlockNode body;
	CanBreakContinueNode(NodeType kind, uint32_t line)
	    : ExprNode(kind, line), body(line) {}
	ExprNode *resolve(in_func) override { return body.resolve(in_data); };
	void rewrite(in_func, uint8_t *bytecodes) override;
};

struct HasClassIdNode : ExprNode {
	ClassDeclaration *classDeclaration;
	ClassId classId;
	inline std::string getClassName(in_func) {
		if (classId == DefaultClass::functionClassId) {
			return classDeclaration->getName(in_data) +
			       (isNullable() ? "?" : "");
		}
		return compile.classes[classId]->name + +(isNullable() ? "?" : "");
	}
	HasClassIdNode(NodeType kind, ClassId classId, uint32_t line,
	               ClassDeclaration *classDeclaration = nullptr)
	    : ExprNode(kind, line), classDeclaration(classDeclaration),
	      classId(classId) {}
	virtual void putBytecodesIfMustBeCalled(in_func,
	                                        std::vector<uint8_t> &bytecodes) {};
	virtual bool isNullable() { return false; }
	virtual bool isStaticValue() { return false; }
	virtual void setNullable(bool nullable) {}
	virtual void setIsStatic(bool isStatic) {}
	virtual ~HasClassIdNode() {}
};

struct NullableNode : HasClassIdNode {
	bool nullable;
	NullableNode(NodeType kind, ClassId classId, bool nullable, uint32_t line)
	    : HasClassIdNode(kind, classId, line), nullable(nullable) {}
	bool isNullable() override { return nullable; }
	void setNullable(bool nullable) override { this->nullable = nullable; }
};

struct JumpIfNullNode : NullableNode {
	BytecodePos jumpIfNullPos;
	bool returnNullIfNull = true;
	JumpIfNullNode(NodeType kind, uint32_t line)
	    : NullableNode(kind, 0, true, line) {}
};

struct OptionalAccessNode : JumpIfNullNode {
	HasClassIdNode *value;
	OptionalAccessNode(uint32_t line, HasClassIdNode *value)
	    : JumpIfNullNode(NodeType::OPTIONAL_ACCESS, line), value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isNullable() override { return true; }
	bool isStaticValue() override { return value->isStaticValue(); }
	~OptionalAccessNode();
};

struct NullCoalescingNode : JumpIfNullNode {
	HasClassIdNode *left;
	HasClassIdNode *right;
	NullCoalescingNode(uint32_t line, HasClassIdNode *left,
	                   HasClassIdNode *right)
	    : JumpIfNullNode(NodeType::NULL_COALESCING, line), left(left),
	      right(right) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isStaticValue() override {
		return left->isStaticValue() && right->isStaticValue();
	}
	~NullCoalescingNode();
};

struct UnknowNode : NullableNode {
	LexerStringId nameId;
	std::optional<ClassId> contextCallClassId;
	FunctionId contextCallFuncId;
	bool justFindStaticMember;
	UnknowNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	           FunctionId contextCallFuncId, LexerStringId nameId,
	           bool nullable, bool justFindStaticMember)
	    : NullableNode(NodeType::UNKNOW, 0, nullable, line), nameId(nameId),
	      contextCallClassId(contextCallClassId),
	      contextCallFuncId(contextCallFuncId),
	      justFindStaticMember(justFindStaticMember) {}
	ExprNode *resolve(in_func) override;
	ExprNode *copy(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
};

struct AccessNode : NullableNode {
	DeclarationNode *declaration;
	bool isStore;
	bool isVal;
	AccessNode(NodeType kind, uint32_t line, DeclarationNode *declaration,
	           bool nullable, ClassId classId = 0, bool isVal = false,
	           bool isStore = false)
	    : NullableNode(kind, classId, nullable, line), declaration(declaration),
	      isStore(isStore), isVal(isVal) {}
};

//"7e5", 72, 1.6, 1e5, ...
struct ConstValueNode : HasClassIdNode {
	union {
		std::string *str;
		int64_t i;
		double f;
		AObject *obj;
	};
	bool isLoadPrimary = false;
	uint32_t id = UINT32_MAX;
	ConstValueNode()
	    : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::intClassId,
	                     line) {}
	ConstValueNode(uint32_t line, int64_t i)
	    : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::intClassId,
	                     line),
	      i(i) {}
	ConstValueNode(uint32_t line, double f)
	    : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::floatClassId,
	                     line),
	      f(f) {}
	ConstValueNode(uint32_t line, std::string str)
	    : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::stringClassId,
	                     line),
	      str(new std::string(std::move(str))) {}
	ConstValueNode(uint32_t line, bool b)
	    : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::boolClassId,
	                     line),
	      obj(ObjectManager::createBoolObject(b)), id(b ? 1 : 2) {}
	ConstValueNode(uint32_t line, AObject *obj, uint32_t id)
	    : HasClassIdNode(NodeType::CONST, obj->type, line), obj(obj), id(id) {}
	static inline uint32_t getBoolId(bool b) { return b ? 1 : 2; }
	bool isNullable() override {
		return classId == AutoLang::DefaultClass::nullClassId;
	}
	bool isStaticValue() override { return true; }
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	~ConstValueNode();
};

struct ReturnNode : ExprNode {
	// Current function
	FunctionId funcId;
	HasClassIdNode *value;
	bool loaded = false;
	bool throwErrIfVoid = false;
	ReturnNode(uint32_t line, FunctionId funcId, HasClassIdNode *value)
	    : ExprNode(NodeType::RET, line), funcId(funcId), value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	static void putOptimizedBytecodes(in_func, HasClassIdNode *value,
	                                  std::vector<uint8_t> &bytecodes);
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		if (value)
			value->rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~ReturnNode();
};

struct UnaryNode : HasClassIdNode {
	HasClassIdNode *value;
	Lexer::TokenType op;
	UnaryNode(uint32_t line, Lexer::TokenType op, HasClassIdNode *value)
	    : HasClassIdNode(NodeType::UNARY, 0, line), value(value), op(op) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	template <Opcode normal, Opcode local, Opcode global, Opcode local_member,
	          Opcode global_member>
	void putOptimizedBytecodes(in_func, std::vector<uint8_t> &bytecodes);
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		value->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	bool isNullable() override { return false; }
	bool isStaticValue() override { return value->isStaticValue(); }
	ExprNode *copy(in_func) override;
	~UnaryNode();
};

// 1 + 2 * 3 ...
struct BinaryNode : HasClassIdNode {
	Lexer::TokenType op;
	std::optional<ClassId> contextCallClassId;
	HasClassIdNode *left;
	HasClassIdNode *right;
	BinaryNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	           Lexer::TokenType op, HasClassIdNode *left, HasClassIdNode *right)
	    : HasClassIdNode(NodeType::BINARY, 0, line), op(op),
	      contextCallClassId(contextCallClassId), left(left), right(right) {}
	ExprNode *leftOpRight(in_func, ConstValueNode *l, ConstValueNode *r);
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	static bool putOptimizedBytecode(in_func, std::vector<uint8_t> &bytecodes,
	                                 Lexer::TokenType op, HasClassIdNode *left,
	                                 HasClassIdNode *right);
	ExprNode *copy(in_func) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		left->rewrite(in_data, bytecodes);
		right->rewrite(in_data, bytecodes);
	}
	bool isStaticValue() override {
		return left->isStaticValue() && right->isStaticValue();
	}
	~BinaryNode();
};

struct CastNode : NullableNode { // #
	HasClassIdNode *value;
	CastNode(HasClassIdNode *value, ClassId classId)
	    : NullableNode(NodeType::CAST, classId, false, value->line),
	      value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		value->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	bool isStaticValue() override { return value->isStaticValue(); }
	ExprNode *copy(in_func) override;
	~CastNode();
};

struct RuntimeCastNode : NullableNode { // #
	HasClassIdNode *value;
	RuntimeCastNode(HasClassIdNode *value, ClassId classId, bool isSafeCast)
	    : NullableNode(NodeType::RUNTIME_CAST, classId, isSafeCast,
	                   value->line),
	      value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	bool isStaticValue() override { return value->isStaticValue(); }
	ExprNode *copy(in_func) override;
	~RuntimeCastNode();
};

// caller.name
struct GetPropNode : AccessNode {
	std::optional<ClassId> contextCallClassId;
	HasClassIdNode *caller;
	LexerStringId nameId;
	MemberOffset id;
	BytecodePos jumpIfNullPos;
	bool isInitial;
	bool isStatic;
	bool accessNullable;
	GetPropNode(uint32_t line, DeclarationNode *declaration,
	            std::optional<ClassId> contextCallClassId,
	            HasClassIdNode *caller, LexerStringId nameId, bool isInitial,
	            bool nullable, bool accessNullable)
	    : AccessNode(NodeType::GET_PROP, line, declaration, nullable,
	                 AutoLang::DefaultClass::nullClassId),
	      contextCallClassId(contextCallClassId), caller(caller),
	      nameId(nameId), isInitial(isInitial), isStatic(false),
	      accessNullable(accessNullable) {}
	ExprNode *resolve(in_func) override;
	bool optimizeSkipIfNotFoundMember(in_func);
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		caller->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isStaticValue() override { return isStatic; }
	void setIsStatic(bool isStatic) override { this->isStatic = isStatic; }
	~GetPropNode();
};

struct IfNode : NullableNode {
	HasClassIdNode *condition;
	BlockNode ifTrue;
	BlockNode *ifFalse = nullptr;
	bool mustReturnValue = false;
	bool isStatic = false;
	std::vector<BytecodePos> jumpPosition;
	IfNode(uint32_t line, bool mustReturnValue)
	    : NullableNode(NodeType::IF, DefaultClass::nullClassId, false, line),
	      ifTrue(line), mustReturnValue(mustReturnValue) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isStaticValue() override { return isStatic; }
	void setIsStatic(bool isStatic) override { this->isStatic = isStatic; }
	~IfNode();
};

struct WhileNode : CanBreakContinueNode {
	HasClassIdNode *condition;
	WhileNode(uint32_t line) : CanBreakContinueNode(NodeType::WHILE, line) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override {
		if (condition->kind == NodeType::CONST) {
			// Is bool because optimize forbiddened others
			if (!static_cast<ConstValueNode *>(condition)->obj->b) {
				return;
			}
		}
		condition->rewrite(in_data, bytecodes);
		CanBreakContinueNode::rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~WhileNode();
};

// detach = value
struct SetNode : HasClassIdNode {
	Lexer::TokenType op;
	HasClassIdNode *detach;
	HasClassIdNode *value;
	bool isGetPointer = false;
	bool justDetachStatic = false;
	SetNode(uint32_t line, HasClassIdNode *detach, HasClassIdNode *value,
	        bool justDetachStatic,
	        Lexer::TokenType op = Lexer::TokenType::EQUAL)
	    : HasClassIdNode(NodeType::SET, 0, line), op(op), detach(detach),
	      value(value), justDetachStatic(justDetachStatic) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	bool isStaticValue() override { return false; }
	ExprNode *copy(in_func) override;
	~SetNode();
};

// getName(id) => variable
struct VarNode : AccessNode {
	VarNode(uint32_t line, DeclarationNode *declaration, bool isStore,
	        bool nullable)
	    : AccessNode(NodeType::VAR, line, declaration, nullable,
	                 AutoLang::DefaultClass::nullClassId, true, isStore) {}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isStaticValue() override;
};

// for (detach in from..to) body
struct ForNode : CanBreakContinueNode {
	VarNode *iteratorNode;
	VarNode *detach;
	HasClassIdNode *data;
	ForNode(uint32_t line, VarNode *detach, HasClassIdNode *data,
	        VarNode *iteratorNode)
	    : CanBreakContinueNode(NodeType::FOR, line), iteratorNode(iteratorNode),
	      detach(detach), data(data) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	bool putOptimizedRangeBytecode(in_func, std::vector<uint8_t> &bytecodes,
	                               BytecodePos &jumpIfFalseByte,
	                               BytecodePos &firstSkipByte);
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, uint8_t *bytecodes) override {
		data->rewrite(in_data, bytecodes);
		CanBreakContinueNode::rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~ForNode();
};

struct ClassAccessNode : HasClassIdNode {
	ClassAccessNode(uint32_t line, ClassId type)
	    : HasClassIdNode(NodeType::CLASS_ACCESS, type, line) {}
	ExprNode *copy(in_func) override { return this; }
	bool isStaticValue() override { return true; }
};

struct FunctionAccessNode : HasClassIdNode {
	LexerStringId nameId;
	uint32_t count;
	HasClassIdNode *caller;
	HasClassIdNode *object;
	std::vector<FunctionId> *funcs[2];
	std::vector<BytecodePos> jumpPosition;
	FunctionId funcId;
	FunctionAccessNode(uint32_t line, HasClassIdNode *caller,
	                   LexerStringId nameId, uint32_t count,
	                   HasClassIdNode *object, std::vector<FunctionId> **funcs)
	    : HasClassIdNode(NodeType::FUNCTION_ACCESS,
	                     DefaultClass::functionClassId, line),
	      nameId(nameId), count(count), caller(caller), object(object) {
		if (count >= 1)
			this->funcs[0] = funcs[0];
		if (count >= 2)
			this->funcs[1] = funcs[1];
	}
	ExprNode *copy(in_func) override { return this; }
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	bool isStaticValue() override { return true; }
};

struct CreateClosureNode : HasClassIdNode {
	std::vector<HasClassIdNode *> objects;
	std::vector<DeclarationNode *> newDeclaration;
	std::vector<uint8_t> currentBytecodes;
	Parameter *parameter;
	BlockNode body;
	FunctionId funcId;
	uint32_t declarationCount;
	uint32_t maxDeclaration;
	uint32_t parameterCountFirstTime;
	Scopes scopes;
	DeclarationNode *declarationThis = nullptr;
	bool mustInfer = false;
	CreateClosureNode(uint32_t line, Parameter *parameter)
	    : HasClassIdNode(NodeType::CREATE_CLOSURE,
	                     DefaultClass::functionClassId, line),
	      parameter(std::move(parameter)), body(line),
	      declarationCount(parameter->parameters.size()),
	      maxDeclaration(parameter->parameters.size()),
	      parameterCountFirstTime(parameter->parameters.size()) {}
	ExprNode *copy(in_func) override;
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	void inferFrom(in_func, ClassDeclaration *from);
	bool isStaticValue() override { return false; }
};

struct CreateEnumValueNode : HasClassIdNode {
	DeclarationNode *declarationNode;
	CreateEnumValueNode(uint32_t line, uint32_t type,
	                    DeclarationNode *declarationNode)
	    : HasClassIdNode(NodeType::CREATE_ENUM_VALUE, type, line),
	      declarationNode(declarationNode) {}
	ExprNode *copy(in_func) override { return this; }
};

struct MatchOverload {
	Function *func;
	FunctionId id;
	uint8_t score;
	uint8_t errorNonNullIfMatchCount = 0;
};

// caller.name(arguments)
struct CallNode : NullableNode {
	std::optional<ClassId> contextCallClassId;
	HasClassIdNode *caller;
	HasClassIdNode *funcObject;
	LexerStringId nameId;
	std::vector<HasClassIdNode *> arguments;
	ClassDeclaration *inputGenericArguments;
	FunctionId funcId;
	BytecodePos jumpIfNullPos;
	bool justFindStatic;
	bool pauseVM = false;
	bool accessNullable;
	bool isSuper = false;
	CallNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	         HasClassIdNode *caller, LexerStringId nameId,
	         std::vector<HasClassIdNode *> arguments, bool justFindStatic,
	         bool nullable, bool accessNullable)
	    : NullableNode(NodeType::CALL, 0, nullable, line),
	      contextCallClassId(contextCallClassId), caller(caller),
	      funcObject(nullptr), nameId(nameId), arguments(std::move(arguments)),
	      justFindStatic(justFindStatic), accessNullable(accessNullable) {}
	CallNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	         HasClassIdNode *funcObject, LexerStringId nameId,
	         bool justFindStatic, std::vector<HasClassIdNode *> arguments,
	         bool nullable, bool accessNullable)
	    : NullableNode(NodeType::CALL, 0, nullable, line),
	      contextCallClassId(contextCallClassId), caller(nullptr),
	      funcObject(funcObject), nameId(nameId),
	      arguments(std::move(arguments)), accessNullable(accessNullable) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool match(in_func, MatchOverload &match,
	           std::vector<FunctionId> &functions, int &i,
	           bool mustInferenceGenericType);
	void matchFunction(in_func, bool mustInferenceGenericType);
	void matchFunction(in_func, ClassDeclaration *detach,
	                   ClassDeclaration *value);
	FunctionId matchObjectFunction(in_func, std::vector<FunctionId> &funcs);
	bool isStaticValue() override { return false; }
	~CallNode();
};

struct TryCatchNode : ExprNode {
	DeclarationNode *exceptionDeclaration;
	BlockNode body;
	BlockNode catchBody;
	TryCatchNode(uint32_t line)
	    : ExprNode(NodeType::TRY_CATCH, line), body(line), catchBody(line) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
};

struct ThrowNode : ExprNode {
	HasClassIdNode *value;
	ThrowNode(uint32_t line, HasClassIdNode *value)
	    : ExprNode(NodeType::THROW, line), value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	~ThrowNode();
};

struct RangeNode : HasClassIdNode {
	HasClassIdNode *from;
	HasClassIdNode *to;
	bool lessThan;
	RangeNode(uint32_t line, HasClassIdNode *from, HasClassIdNode *to,
	          bool lessThan)
	    : HasClassIdNode(NodeType::RANGE, DefaultClass::intClassId, line),
	      from(from), to(to), lessThan(lessThan) {}
	bool isNullable() override {
		return from->isNullable() || to->isNullable();
	}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isStaticValue() override {
		return from->isStaticValue() && to->isStaticValue();
	}
	~RangeNode();
};

struct CreateArrayNode : HasClassIdNode {
	ClassDeclaration *classDeclaration;
	std::vector<HasClassIdNode *> values;
	CreateArrayNode(uint32_t line, ClassDeclaration *classDeclaration,
	                std::vector<HasClassIdNode *> values)
	    : HasClassIdNode(NodeType::CREATE_ARRAY, DefaultClass::nullClassId,
	                     line),
	      classDeclaration(classDeclaration), values(std::move(values)) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	~CreateArrayNode();
};

struct CreateSetNode : HasClassIdNode {
	ClassDeclaration *classDeclaration;
	std::vector<HasClassIdNode *> values;
	CreateSetNode(uint32_t line, ClassDeclaration *classDeclaration,
	              std::vector<HasClassIdNode *> values)
	    : HasClassIdNode(NodeType::CREATE_SET, DefaultClass::nullClassId, line),
	      classDeclaration(classDeclaration), values(std::move(values)) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	~CreateSetNode();
};

struct CreateMapNode : HasClassIdNode {
	ClassDeclaration *classDeclaration;
	std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values;
	CreateMapNode(
	    uint32_t line, ClassDeclaration *classDeclaration,
	    std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values)
	    : HasClassIdNode(NodeType::CREATE_MAP, DefaultClass::nullClassId, line),
	      classDeclaration(classDeclaration), values(std::move(values)) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	~CreateMapNode();
};

struct WhenNode : HasClassIdNode {
	HasClassIdNode *value;
	IfNode *ifNode;
	WhenNode(uint32_t line, HasClassIdNode *value, IfNode *ifNode)
	    : HasClassIdNode(NodeType::WHEN, DefaultClass::nullClassId, line),
	      value(value), ifNode(ifNode) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodesIfMustBeCalled(in_func,
	                                std::vector<uint8_t> &bytecodes) override {
		putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::POP);
	}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, uint8_t *bytecodes) override;
	ExprNode *copy(in_func) override;
	bool isNullable() override {
		return !ifNode ? false : ifNode->isNullable();
	}
	bool isStaticValue() override {
		return !ifNode ? true : ifNode->isStaticValue();
	}
	~WhenNode();
};

} // namespace AutoLang

#endif