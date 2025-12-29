#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <optional>
#include "Interpreter.hpp"
#include "DefaultClass.hpp"
#include "Lexer.hpp"
#include "OptimizeNode.hpp"

namespace AutoLang {

struct ParserContext;
struct DeclarationNode;

void put_opcode_u32(std::vector<uint8_t>& code, uint32_t value);
void put_opcode_u32(std::vector<uint8_t>& code, size_t pos, uint32_t value);
AClass* findClass(in_func, std::string name);

enum NodeType : uint8_t {
	UNKNOW,
	VAR,
	BINARY,
	CONST,
	GET_PROP,
	CONST_VALUE,
	DECLARATION,
	CALL,
	CAST,
	SET,
	CLASS,
	UNARY,
	IF,
	BLOCK,
	WHILE,
	FOR_RANGE,
	CREATE_FUNC,
	CREATE_CLASS,
	CREATE_CONSTRUCTOR,
	RET,
	SKIP,
	CLASS_ACCESS,
};

struct ExprNode {
	NodeType kind;
	ExprNode(NodeType kind):kind(kind){}
	virtual void optimize(in_func){}
	virtual void putBytecodes(in_func, std::vector<uint8_t>& bytecodes){};
	virtual void rewrite(in_func, std::vector<uint8_t>& bytecodes){}
	virtual ~ExprNode(){}
};

struct SkipNode : ExprNode {
	Lexer::TokenType type;
	size_t jumpBytePos;
	SkipNode(Lexer::TokenType type):
		ExprNode(NodeType::SKIP), type(type){}
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	void rewrite(in_func, std::vector<uint8_t>& bytecodes);
};

struct BlockNode : ExprNode {
	std::vector<ExprNode*> nodes;
	BlockNode():
		ExprNode(NodeType::BLOCK){}
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	void rewrite(in_func, std::vector<uint8_t>& bytecodes){
		for (auto* node:nodes) node->rewrite(in_data, bytecodes);
	}
	~BlockNode();
};

struct CanBreakContinueNode : ExprNode {
	size_t continuePos = 0;
	size_t breakPos = 0;
	BlockNode body;
	CanBreakContinueNode(NodeType kind):
		ExprNode(kind){}
	void rewrite(in_func, std::vector<uint8_t>& bytecodes);
};

struct HasClassIdNode : ExprNode {
	uint32_t classId;
	inline std::string& getClassName(in_func){ return compile.classes[classId].name; }
	HasClassIdNode(NodeType kind, uint32_t classId = 0):
		ExprNode(kind), classId(classId){}
};

struct UnknowNode : HasClassIdNode {
	std::string name;
	std::optional<uint32_t> contextCallClassId;
	HasClassIdNode* correctNode;
	UnknowNode(std::optional<uint32_t> contextCallClassId, std::string name):
		HasClassIdNode(NodeType::UNKNOW), name(std::move(name)), contextCallClassId(contextCallClassId), correctNode(nullptr){
		}
	void optimize(in_func) override;
	inline void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) {
		correctNode->putBytecodes(in_data, bytecodes);
	}
	inline void rewrite(in_func, std::vector<uint8_t>& bytecodes) {
		correctNode->rewrite(in_data, bytecodes);
	}
	~UnknowNode();
};

struct AccessNode : HasClassIdNode {
	DeclarationNode* declaration;
	bool isStore;
	bool isVal;
	AccessNode(NodeType kind, DeclarationNode* declaration, uint32_t classId = 0, bool isVal = false, bool isStore = false):
		HasClassIdNode(kind, classId), declaration(declaration), isStore(isStore), isVal(isVal){}
};

//"7e5", 72, 1.6, 1e5, ... 
struct ConstValueNode : HasClassIdNode {
	union {
		std::string* str;
		long long i;
		double f;
		AObject* obj;
	};
	bool isLoadPrimary = false;
	uint32_t id = UINT32_MAX;
	ConstValueNode(long long i): 
		HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::INTCLASSID), i(i) {}
	ConstValueNode(double f): 
		HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::FLOATCLASSID), f(f) {}
	ConstValueNode(std::string str): 
		HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::stringClassId), str(new std::string(std::move(str))) {}
	ConstValueNode(bool b): 
		HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::boolClassId), obj(ObjectManager::create(b)), id(b ? 1 : 2) {}
	ConstValueNode(AObject* obj, uint32_t id):
		HasClassIdNode(NodeType::CONST, obj->type), obj(obj), id(id) {}
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	~ConstValueNode();
};

struct ReturnNode : HasClassIdNode {
	//Current function
	Function* func;
	HasClassIdNode* value;
	ReturnNode(Function* func, HasClassIdNode* value):
		HasClassIdNode(NodeType::RET), func(func), value(value){}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
	~ReturnNode();
};

struct UnaryNode : HasClassIdNode {
	HasClassIdNode* value;
	Lexer::TokenType op;
	UnaryNode(Lexer::TokenType op, HasClassIdNode* value):
		HasClassIdNode(NodeType::UNARY), value(value), op(op){}
	ConstValueNode* calculate(in_func);
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
	~UnaryNode();
};

//1 + 2 * 3 ...
struct BinaryNode : HasClassIdNode {
	Lexer::TokenType op;
	HasClassIdNode* left;
	HasClassIdNode* right;
	BinaryNode(Lexer::TokenType op, HasClassIdNode* left, HasClassIdNode* right):
		HasClassIdNode(NodeType::BINARY), op(op), left(left), right(right){}
	//Calculate
	ConstValueNode* calculate(in_func);
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	~BinaryNode();
};

struct CastNode : HasClassIdNode {
	HasClassIdNode* value;
	CastNode(HasClassIdNode* value, uint32_t classId): 
		HasClassIdNode(NodeType::CAST, classId), value(value){}
	static HasClassIdNode* createAndOptimize(in_func, HasClassIdNode* value, uint32_t classId);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	~CastNode();
};

//caller.name
struct GetPropNode : AccessNode {
	std::optional<uint32_t> contextCallClassId;
	HasClassIdNode* caller;
	std::string name;
	uint32_t id;
	bool isInitial;
	bool isStatic;
	GetPropNode(DeclarationNode* declaration, std::optional<uint32_t> contextCallClassId, HasClassIdNode* caller, std::string name, bool isInitial) :
		AccessNode(NodeType::GET_PROP, declaration, AutoLang::DefaultClass::nullClassId), contextCallClassId(contextCallClassId), caller(caller), name(std::move(name)), isInitial(isInitial){
		}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
	~GetPropNode();
};

struct IfNode : ExprNode {
	HasClassIdNode* condition;
	BlockNode ifTrue;
	BlockNode* ifFalse = nullptr;
	IfNode():
		ExprNode(NodeType::IF){}
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	void rewrite(in_func, std::vector<uint8_t>& bytecodes);
	~IfNode();
};

struct WhileNode : CanBreakContinueNode {
	HasClassIdNode* condition;
	WhileNode():
		CanBreakContinueNode(NodeType::WHILE){}
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	~WhileNode();
};

// detach = value
struct SetNode : HasClassIdNode {
	Lexer::TokenType op;
	HasClassIdNode* detach;
	HasClassIdNode* value;
	bool isGetPointer = false;
	SetNode(HasClassIdNode* detach, HasClassIdNode* value, Lexer::TokenType op = Lexer::TokenType::EQUAL):
		HasClassIdNode(NodeType::SET), op(op), detach(detach), value(value){}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
	~SetNode();
};

// for (detach in from..to) body
struct ForRangeNode : CanBreakContinueNode {
	AccessNode* detach;
	HasClassIdNode* from;
	HasClassIdNode* to;
	bool isLessThanEq;
	ForRangeNode():
		CanBreakContinueNode(NodeType::FOR_RANGE){}
	ForRangeNode(AccessNode* detach, HasClassIdNode* from, HasClassIdNode* to, bool isLessThanEq):
		CanBreakContinueNode(NodeType::FOR_RANGE), detach(detach), from(from), to(to), isLessThanEq(isLessThanEq){}
	void optimize(in_func);
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes);
	~ForRangeNode();
};

//getName(id) => variable
struct VarNode : AccessNode {
	VarNode(DeclarationNode* declaration, bool isStore): 
		AccessNode(NodeType::VAR, declaration, AutoLang::DefaultClass::nullClassId, true, isStore) {}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
};

struct ClassAccessNode : HasClassIdNode {
	ClassAccessNode(uint32_t type): 
		HasClassIdNode(NodeType::CLASS_ACCESS, type) {}
};

struct MatchOverload {
	Function* func;
	uint8_t score;
	uint32_t id;
	bool hasNull;
};

//caller.name(arguments)
struct CallNode : HasClassIdNode {
	std::optional<uint32_t> contextCallClassId;
	HasClassIdNode* caller;
	std::string name;
	std::vector<HasClassIdNode*> arguments;
	uint32_t funcId;
	bool isConstructor = false;
	bool justFindStatic;
	bool addPopBytecode = false;
	CallNode(std::optional<uint32_t> contextCallClassId, HasClassIdNode* caller, std::string name, bool justFindStatic) : 
		HasClassIdNode(NodeType::CALL), caller(caller), contextCallClassId(contextCallClassId), name(name), justFindStatic(justFindStatic){}
	void optimize(in_func) override;
	void putBytecodes(in_func, std::vector<uint8_t>& bytecodes) override;
	bool match(CompiledProgram& compile, MatchOverload& match, std::vector<uint32_t>& functions, int& i);
	~CallNode();
};

}

#endif