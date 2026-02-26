#ifndef NODE_HPP
#define NODE_HPP

#include "frontend/lexer/Lexer.hpp"
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

inline void rewrite_opcode_u32(std::vector<uint8_t> &code, uint32_t pos,
                               uint32_t value) {
	code[pos] = (value >> 0) & 0xFF;
	code[pos + 1] = (value >> 8) & 0xFF;
	code[pos + 2] = (value >> 16) & 0xFF;
	code[pos + 3] = (value >> 24) & 0xFF;
}

AClass *findClass(in_func, std::string name);

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
	OPTIONAL_ACCESS,
	NULL_COALESCING,
	TRY_CATCH,
	THROW,
	RUNTIME_CAST,
	GENERIC_DECLARATION,
	RANGE
};

struct ExprNode {
	LibraryData *mode;
	uint32_t line;
	NodeType kind;
	ExprNode(NodeType kind, uint32_t line = 0);
	[[noreturn]] inline void throwError(std::string message);
	void warning(in_func, std::string message);
	static void deleteNode(ExprNode *node);
	std::string getNodeType();
	virtual ExprNode *resolve(in_func) { return this; }
	virtual void optimize(in_func) {}
	virtual void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {};
	virtual void rewrite(in_func, std::vector<uint8_t> &bytecodes) {}
	virtual ExprNode *copy(in_func) { return nullptr; }
	virtual ~ExprNode() {}
};

struct SkipNode : ExprNode {
	Lexer::TokenType type;
	BytecodePos jumpBytePos;
	SkipNode(Lexer::TokenType type, uint32_t line)
	    : ExprNode(NodeType::SKIP, line), type(type) {}
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
};

struct BlockNode : ExprNode {
	std::vector<ExprNode *> nodes;
	BlockNode(uint32_t line) : ExprNode(NodeType::BLOCK, line) {}
	static void loadReturnValueClassId(in_func, uint32_t line,
	                                   std::optional<ClassId> &currentClassId,
	                                   ClassId newClassId);
	void loadClassAndOptimize(in_func);
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *resolve(in_func) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	void refresh();
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
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
};

struct HasClassIdNode : ExprNode {
	ClassId classId;
	inline std::string &getClassName(in_func) {
		return compile.classes[classId]->name;
	}
	HasClassIdNode(NodeType kind, ClassId classId, uint32_t line)
	    : ExprNode(kind, line), classId(classId) {}
	virtual bool isNullable() { return false; }
	virtual ~HasClassIdNode() {}
};

struct NullableNode : HasClassIdNode {
	bool nullable;
	NullableNode(NodeType kind, ClassId classId, bool nullable, uint32_t line)
	    : HasClassIdNode(kind, classId, line), nullable(nullable) {}
	bool isNullable() override { return nullable; }
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
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
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
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	~NullCoalescingNode();
};

struct UnknowNode : NullableNode {
	LexerStringId nameId;
	std::optional<ClassId> contextCallClassId;
	UnknowNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	           LexerStringId nameId, bool nullable)
	    : NullableNode(NodeType::UNKNOW, 0, nullable, line), nameId(nameId),
	      contextCallClassId(contextCallClassId) {}
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
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
	ExprNode *copy(in_func) override;
	~ConstValueNode();
};

struct ReturnNode : ExprNode {
	// Current function
	FunctionId funcId;
	HasClassIdNode *value;
	bool loaded = false;
	ReturnNode(uint32_t line, FunctionId funcId, HasClassIdNode *value)
	    : ExprNode(NodeType::RET, line), funcId(funcId), value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
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
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~UnaryNode();
};

// 1 + 2 * 3 ...
struct BinaryNode : HasClassIdNode {
	Lexer::TokenType op;
	HasClassIdNode *left;
	HasClassIdNode *right;
	BinaryNode(uint32_t line, Lexer::TokenType op, HasClassIdNode *left,
	           HasClassIdNode *right)
	    : HasClassIdNode(NodeType::BINARY, 0, line), op(op), left(left),
	      right(right) {}
	ExprNode *leftOpRight(in_func, ConstValueNode *l, ConstValueNode *r);
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
	bool putOptimizedBytecode(in_func, std::vector<uint8_t> &bytecodes);
	ExprNode *copy(in_func) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		left->rewrite(in_data, bytecodes);
		right->rewrite(in_data, bytecodes);
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
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
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
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~RuntimeCastNode();
};

// caller.name
struct GetPropNode : AccessNode {
	std::optional<ClassId> contextCallClassId;
	HasClassIdNode *caller;
	std::string name;
	MemberOffset id;
	BytecodePos jumpIfNullPos;
	bool isInitial;
	bool isStatic;
	bool accessNullable;
	GetPropNode(uint32_t line, DeclarationNode *declaration,
	            std::optional<ClassId> contextCallClassId,
	            HasClassIdNode *caller, std::string name, bool isInitial,
	            bool nullable, bool accessNullable)
	    : AccessNode(NodeType::GET_PROP, line, declaration, nullable,
	                 AutoLang::DefaultClass::nullClassId),
	      contextCallClassId(contextCallClassId), caller(caller),
	      name(std::move(name)), isInitial(isInitial), isStatic(false),
	      accessNullable(accessNullable) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	~GetPropNode();
};

struct IfNode : HasClassIdNode {
	HasClassIdNode *condition;
	BlockNode ifTrue;
	BlockNode *ifFalse = nullptr;
	bool mustReturnValue = false;
	std::vector<BytecodePos> jumpPosition;
	IfNode(uint32_t line, bool mustReturnValue)
	    : HasClassIdNode(NodeType::IF, DefaultClass::nullClassId, line),
	      ifTrue(line), mustReturnValue(mustReturnValue) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	~IfNode();
};

struct WhileNode : CanBreakContinueNode {
	HasClassIdNode *condition;
	WhileNode(uint32_t line) : CanBreakContinueNode(NodeType::WHILE, line) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
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
	SetNode(uint32_t line, HasClassIdNode *detach, HasClassIdNode *value,
	        Lexer::TokenType op = Lexer::TokenType::EQUAL)
	    : HasClassIdNode(NodeType::SET, 0, line), op(op), detach(detach),
	      value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		value->rewrite(in_data, bytecodes);
	}
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
};

// for (detach in from..to) body
struct ForNode : CanBreakContinueNode {
	VarNode *iteratorNode;
	VarNode *detach;
	HasClassIdNode *data;
	ForNode(uint32_t line, VarNode *detach, HasClassIdNode *data,
	        VarNode *iteratorNode)
	    : CanBreakContinueNode(NodeType::FOR, line), detach(detach), data(data),
	      iteratorNode(iteratorNode) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func);
	bool putOptimizedRangeBytecode(in_func, std::vector<uint8_t> &bytecodes,
	                               BytecodePos &jumpIfFalseByte,
	                               BytecodePos &firstSkipByte);
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	inline void rewrite(in_func, std::vector<uint8_t> &bytecodes) override {
		data->rewrite(in_data, bytecodes);
		CanBreakContinueNode::rewrite(in_data, bytecodes);
	}
	ExprNode *copy(in_func) override;
	~ForNode();
};

struct ClassAccessNode : HasClassIdNode {
	ClassAccessNode(uint32_t line, uint32_t type)
	    : HasClassIdNode(NodeType::CLASS_ACCESS, type, line) {}
	ExprNode *copy(in_func) override { return this; }
};

struct MatchOverload {
	Function *func;
	FunctionId id;
	uint8_t score;
	bool errorNonNullIfMatch = false;
};

// caller.name(arguments)
struct CallNode : NullableNode {
	std::optional<ClassId> contextCallClassId;
	HasClassIdNode *caller;
	std::string name;
	std::vector<HasClassIdNode *> arguments;
	FunctionId funcId;
	BytecodePos jumpIfNullPos;
	bool justFindStatic;
	bool pauseVM = false;
	bool accessNullable;
	bool isSuper = false;
	CallNode(uint32_t line, std::optional<ClassId> contextCallClassId,
	         HasClassIdNode *caller, std::string name,
	         std::vector<HasClassIdNode *> arguments, bool justFindStatic,
	         bool nullable, bool accessNullable)
	    : NullableNode(NodeType::CALL, 0, nullable, line),
	      contextCallClassId(contextCallClassId), caller(caller), name(name),
	      arguments(std::move(arguments)), justFindStatic(justFindStatic),
	      accessNullable(accessNullable) {}
	bool isNullable() override { return nullable; }
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
	bool match(in_func, MatchOverload &match, std::vector<uint32_t> &functions,
	           int &i);
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
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
	ExprNode *copy(in_func) override;
};

struct ThrowNode : ExprNode {
	HasClassIdNode *value;
	ThrowNode(uint32_t line, HasClassIdNode *value)
	    : ExprNode(NodeType::THROW, line), value(value) {}
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
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
	ExprNode *resolve(in_func) override;
	void optimize(in_func) override;
	ExprNode *copy(in_func) override;
	~RangeNode();
};

// struct CreateArrayNode : HasClassIdNode {
// 	std::vector<HasClassIdNode *> values;
// 	CreateArrayNode(uint32_t line, std::vector<HasClassIdNode *>values)
// 	    : HasClassIdNode(NodeType::THROW, DefaultClass::nullClassId, line),
// values(values) {} 	ExprNode *resolve(in_func) override; 	void optimize(in_func)
// override; 	void putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
// override; 	void rewrite(in_func, std::vector<uint8_t> &bytecodes) override;
// 	ExprNode *copy(in_func) override;
// 	~CreateArrayNode();
// };

} // namespace AutoLang

#endif