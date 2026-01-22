#ifndef NODE_HPP
#define NODE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <optional>
#include "./vm/Interpreter.hpp"
#include "DefaultClass.hpp"
#include "Lexer.hpp"
#include "OptimizeNode.hpp"

namespace AutoLang
{

	struct ParserContext;
	struct DeclarationNode;
	struct ParserError;

	void put_opcode_u32(std::vector<uint8_t> &code, uint32_t value);
	void rewrite_opcode_u32(std::vector<uint8_t> &code, size_t pos, uint32_t value);
	AClass *findClass(in_func, std::string name);

	enum NodeType : uint8_t
	{
		UNKNOW,
		VAR,
		BINARY,
		CONST,
		GET_PROP,
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

	struct ExprNode
	{
		uint32_t line;
		NodeType kind;
		ExprNode(NodeType kind, uint32_t line = 0) : line(line), kind(kind) {}
		[[noreturn]] inline void throwError(std::string message);
		static void deleteNode(ExprNode *node);
		virtual void optimize(in_func) {}
		virtual void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {};
		virtual void rewrite(in_func, std::vector<uint8_t> &bytecodes) {}
		virtual ~ExprNode() {}
	};

	struct SkipNode : ExprNode
	{
		Lexer::TokenType type;
		size_t jumpBytePos;
		SkipNode(Lexer::TokenType type, uint32_t line) : ExprNode(NodeType::SKIP, line), type(type) {}
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		void rewrite(in_func, std::vector<uint8_t> &bytecodes);
	};

	struct BlockNode : ExprNode
	{
		std::vector<ExprNode *> nodes;
		BlockNode(uint32_t line) : ExprNode(NodeType::BLOCK, line) {}
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		void rewrite(in_func, std::vector<uint8_t> &bytecodes)
		{
			for (auto *node : nodes)
				node->rewrite(in_data, bytecodes);
		}
		void refresh();
		~BlockNode();
	};

	struct CanBreakContinueNode : ExprNode
	{
		size_t continuePos = 0; // Support "continue" command
		size_t breakPos = 0;	// Support "break" command
		BlockNode body;
		CanBreakContinueNode(NodeType kind, uint32_t line) : ExprNode(kind, line), body(line) {}
		void rewrite(in_func, std::vector<uint8_t> &bytecodes);
	};

	struct HasClassIdNode : ExprNode
	{
		uint32_t classId;
		inline std::string &getClassName(in_func) { return compile.classes[classId].name; }
		HasClassIdNode(NodeType kind, uint32_t classId, uint32_t line) : ExprNode(kind, line), classId(classId) {}
		virtual bool isNullable() { return false; }
		virtual ~HasClassIdNode() {}
	};

	struct UnknowNode : HasClassIdNode
	{
		std::string name;
		std::optional<uint32_t> contextCallClassId;
		HasClassIdNode *correctNode;
		bool nullable;
		UnknowNode(uint32_t line, std::optional<uint32_t> contextCallClassId, std::string name, bool nullable) : 
				HasClassIdNode(NodeType::UNKNOW, 0, line), name(std::move(name)), contextCallClassId(contextCallClassId), correctNode(nullptr), nullable(nullable) {}
		bool isNullable() override { return correctNode->isNullable(); }
		void optimize(in_func) override;
		inline void putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
		{
			correctNode->putBytecodes(in_data, bytecodes);
		}
		inline void rewrite(in_func, std::vector<uint8_t> &bytecodes)
		{
			correctNode->rewrite(in_data, bytecodes);
		}
		~UnknowNode();
	};

	struct AccessNode : HasClassIdNode
	{
		DeclarationNode *declaration;
		bool nullable;
		bool isStore;
		bool isVal;
		AccessNode(NodeType kind, uint32_t line, DeclarationNode *declaration, bool nullable, uint32_t classId = 0, bool isVal = false, bool isStore = false) : 
				HasClassIdNode(kind, classId, line), declaration(declaration), nullable(nullable), isStore(isStore), isVal(isVal) {}
		bool isNullable() override { return nullable; }
	};

	//"7e5", 72, 1.6, 1e5, ...
	struct ConstValueNode : HasClassIdNode
	{
		union
		{
			std::string *str;
			int64_t i;
			double f;
			AObject *obj;
		};
		bool isLoadPrimary = false;
		uint32_t id = UINT32_MAX;
		ConstValueNode(uint32_t line, int64_t i) : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::intClassId, line), i(i) {}
		ConstValueNode(uint32_t line, double f) : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::floatClassId, line), f(f) {}
		ConstValueNode(uint32_t line, std::string str) : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::stringClassId, line), str(new std::string(std::move(str))) {}
		ConstValueNode(uint32_t line, bool b) : HasClassIdNode(NodeType::CONST, AutoLang::DefaultClass::boolClassId, line), obj(ObjectManager::createBoolObject(b)), id(b ? 1 : 2) {}
		ConstValueNode(uint32_t line, AObject *obj, uint32_t id) : HasClassIdNode(NodeType::CONST, obj->type, line), obj(obj), id(id) {}
		bool isNullable() override { return id == AutoLang::DefaultClass::nullClassId; }
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		~ConstValueNode();
	};

	struct ReturnNode : ExprNode
	{
		// Current function
		uint32_t funcId;
		HasClassIdNode *value;
		ReturnNode(uint32_t line, uint32_t funcId, HasClassIdNode *value) : ExprNode(NodeType::RET, line), funcId(funcId), value(value) {}
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
		~ReturnNode();
	};

	struct UnaryNode : HasClassIdNode
	{
		HasClassIdNode *value;
		Lexer::TokenType op;
		UnaryNode(uint32_t line, Lexer::TokenType op, HasClassIdNode *value) : HasClassIdNode(NodeType::UNARY, 0, line), value(value), op(op) {}
		ConstValueNode *calculate(in_func);
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
		~UnaryNode();
	};

	// 1 + 2 * 3 ...
	struct BinaryNode : HasClassIdNode
	{
		Lexer::TokenType op;
		HasClassIdNode *left;
		HasClassIdNode *right;
		BinaryNode(uint32_t line, Lexer::TokenType op, HasClassIdNode *left, HasClassIdNode *right) : 
					HasClassIdNode(NodeType::BINARY, 0, line), op(op), left(left), right(right) {}
		// Calculate
		ConstValueNode *calculate(in_func);
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		~BinaryNode();
	};

	struct CastNode : HasClassIdNode
	{
		HasClassIdNode *value;
		CastNode(HasClassIdNode *value, uint32_t classId) : HasClassIdNode(NodeType::CAST, classId, value->line), value(value) {}
		static HasClassIdNode *createAndOptimize(in_func, HasClassIdNode *value, uint32_t classId);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		~CastNode();
	};

	// caller.name
	struct GetPropNode : AccessNode
	{
		std::optional<uint32_t> contextCallClassId;
		HasClassIdNode *caller;
		std::string name;
		uint32_t id;
		bool isInitial;
		bool isStatic;
		GetPropNode(uint32_t line, DeclarationNode *declaration, std::optional<uint32_t> contextCallClassId, HasClassIdNode *caller, std::string name, bool isInitial, bool nullable) : 
					AccessNode(NodeType::GET_PROP, line, declaration, nullable, AutoLang::DefaultClass::nullClassId), contextCallClassId(contextCallClassId), caller(caller), name(std::move(name)), isInitial(isInitial), isStatic(false) {}
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
		~GetPropNode();
	};

	struct IfNode : ExprNode
	{
		HasClassIdNode *condition;
		BlockNode ifTrue;
		BlockNode *ifFalse = nullptr;
		IfNode(uint32_t line) : ExprNode(NodeType::IF, line), ifTrue(line) {}
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		void rewrite(in_func, std::vector<uint8_t> &bytecodes);
		~IfNode();
	};

	struct WhileNode : CanBreakContinueNode
	{
		HasClassIdNode *condition;
		WhileNode(uint32_t line) : CanBreakContinueNode(NodeType::WHILE, line) {}
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		~WhileNode();
	};

	// detach = value
	struct SetNode : HasClassIdNode
	{
		Lexer::TokenType op;
		HasClassIdNode *detach;
		HasClassIdNode *value;
		bool isGetPointer = false;
		SetNode(uint32_t line, HasClassIdNode *detach, HasClassIdNode *value, Lexer::TokenType op = Lexer::TokenType::EQUAL) : 
				HasClassIdNode(NodeType::SET, 0, line), op(op), detach(detach), value(value) {}
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
		~SetNode();
	};

	// for (detach in from..to) body
	struct ForRangeNode : CanBreakContinueNode
	{
		AccessNode *detach;
		HasClassIdNode *from;
		HasClassIdNode *to;
		bool isLessThanEq;
		// ForRangeNode(uint32_t line) : CanBreakContinueNode(NodeType::FOR_RANGE, line) {}
		ForRangeNode(uint32_t line, AccessNode *detach, HasClassIdNode *from, HasClassIdNode *to, bool isLessThanEq) : CanBreakContinueNode(NodeType::FOR_RANGE, line), detach(detach), from(from), to(to), isLessThanEq(isLessThanEq) {}
		void optimize(in_func);
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes);
		~ForRangeNode();
	};

	// getName(id) => variable
	struct VarNode : AccessNode
	{
		VarNode(uint32_t line, DeclarationNode *declaration, bool isStore, bool nullable) : 
				AccessNode(NodeType::VAR, line, declaration, nullable, AutoLang::DefaultClass::nullClassId, true, isStore) {}
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
	};

	struct ClassAccessNode : HasClassIdNode
	{
		ClassAccessNode(uint32_t line, uint32_t type) : HasClassIdNode(NodeType::CLASS_ACCESS, type, line) {}
	};

	struct MatchOverload
	{
		Function *func;
		uint32_t id;
		uint8_t score;
		bool errorNonNullIfMatch = false;
	};

	// caller.name(arguments)
	struct CallNode : HasClassIdNode
	{
		std::optional<uint32_t> contextCallClassId;
		HasClassIdNode *caller;
		std::string name;
		std::vector<HasClassIdNode *> arguments;
		uint32_t funcId;
		bool isConstructor = false;
		bool justFindStatic;
		bool addPopBytecode = false;
		bool nullable;
		CallNode(uint32_t line, std::optional<uint32_t> contextCallClassId, HasClassIdNode *caller, std::string name, std::vector<HasClassIdNode *> arguments, bool justFindStatic, bool nullable) : 
				HasClassIdNode(NodeType::CALL, 0, line), contextCallClassId(contextCallClassId), caller(caller), name(name), arguments(std::move(arguments)), justFindStatic(justFindStatic), nullable(nullable) {}
		bool isNullable() override { return nullable; }
		void optimize(in_func) override;
		void putBytecodes(in_func, std::vector<uint8_t> &bytecodes) override;
		bool match(in_func, MatchOverload &match, std::vector<uint32_t> &functions, int &i);
		~CallNode();
	};

}

#endif