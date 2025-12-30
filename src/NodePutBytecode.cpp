#ifndef NODE_PUT_BYTECODE_CPP
#define NODE_PUT_BYTECODE_CPP

#include "NodePutBytecode.hpp"
#include "CreateNode.hpp"
#include "Debugger.hpp"

namespace AutoLang
{

	void BinaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		left->putBytecodes(in_data, bytecodes);
		right->putBytecodes(in_data, bytecodes);
		switch (op)
		{
			using namespace AutoLang;
		case Lexer::TokenType::PLUS:
			bytecodes.emplace_back(AutoLang::Opcode::PLUS);
			return;
		case Lexer::TokenType::MINUS:
			bytecodes.emplace_back(AutoLang::Opcode::MINUS);
			return;
		case Lexer::TokenType::STAR:
			bytecodes.emplace_back(AutoLang::Opcode::MUL);
			return;
		case Lexer::TokenType::SLASH:
			bytecodes.emplace_back(AutoLang::Opcode::DIVIDE);
			return;
		case Lexer::TokenType::PERCENT:
			bytecodes.emplace_back(AutoLang::Opcode::MOD);
			return;
		case Lexer::TokenType::AND_AND:
			bytecodes.emplace_back(AutoLang::Opcode::AND_AND);
			return;
		case Lexer::TokenType::OR_OR:
			bytecodes.emplace_back(AutoLang::Opcode::OR_OR);
			return;
		case Lexer::TokenType::NOTEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_POINTER);
			return;
		case Lexer::TokenType::EQEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::EQUAL_POINTER);
			return;
		case Lexer::TokenType::NOTEQ:
			bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_VALUE);
			return;
		case Lexer::TokenType::EQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::EQUAL_VALUE);
			return;
		case Lexer::TokenType::LT:
			bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN);
			return;
		case Lexer::TokenType::GT:
			bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN);
			return;
		case Lexer::TokenType::GTE:
			bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN_EQ);
			return;
		case Lexer::TokenType::LTE:
			bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN_EQ);
			return;
		default:
			// std::cout<<this<<'\n';
			throw std::runtime_error(std::string("Cannot find operator '") + Lexer::Token(0, op).toString(context) + "'");
		}
	}

	void UnaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		value->putBytecodes(in_data, bytecodes);
		switch (op)
		{
		case Lexer::TokenType::MINUS:
		{
			bytecodes.emplace_back(Opcode::NEGATIVE);
			break;
		}
		case Lexer::TokenType::NOT:
		{
			bytecodes.emplace_back(Opcode::NOT);
			break;
		}
		default:
			break;
		}
	}

	void SkipNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		bytecodes.emplace_back(Opcode::JUMP);
		jumpBytePos = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
	}

	void SkipNode::rewrite(in_func, std::vector<uint8_t> &bytecodes)
	{
		switch (type)
		{
		case Lexer::TokenType::CONTINUE:
			put_opcode_u32(bytecodes, jumpBytePos, context.continuePos);
			break;
		case Lexer::TokenType::BREAK:
			put_opcode_u32(bytecodes, jumpBytePos, context.breakPos);
			break;
		default:
			break;
		}
	}

	void ConstValueNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		bytecodes.emplace_back((isLoadPrimary ||
								classId == AutoLang::DefaultClass::nullClassId ||
								classId == AutoLang::DefaultClass::boolClassId)
								   ? Opcode::LOAD_CONST_PRIMARY
								   : Opcode::LOAD_CONST);
		put_opcode_u32(bytecodes, id);
	}

	void CastNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		value->putBytecodes(in_data, bytecodes);
		switch (classId)
		{
		case AutoLang::DefaultClass::INTCLASSID:
			bytecodes.emplace_back(Opcode::TO_INT);
			return;
		case AutoLang::DefaultClass::FLOATCLASSID:
			bytecodes.emplace_back(Opcode::TO_FLOAT);
			return;
		default:
			bytecodes.emplace_back(Opcode::TO_STRING);
			return;
		}
	}

	void BlockNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		for (auto *node : nodes)
		{
			node->putBytecodes(in_data, bytecodes);
		}
	}

	void GetPropNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		if (!isStatic)
		{
			caller->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(isStore ? Opcode::STORE_MEMBER : Opcode::LOAD_MEMBER);
			put_opcode_u32(bytecodes, id);
			return;
		}
		switch (caller->kind)
		{
		case NodeType::UNKNOW:
		case NodeType::VAR:
			bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL : Opcode::LOAD_GLOBAL);
			put_opcode_u32(bytecodes, id);
			break;
		default:
			break;
		}
	}

	void IfNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		condition->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
		size_t jumpIfFalseByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		ifTrue.putBytecodes(in_data, bytecodes);
		if (ifFalse)
		{
			bytecodes.emplace_back(Opcode::JUMP);
			size_t jumpIfTrueByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			put_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
			ifFalse->putBytecodes(in_data, bytecodes);
			put_opcode_u32(bytecodes, jumpIfTrueByte, bytecodes.size());
		}
		else
		{
			put_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		}
	}

	void IfNode::rewrite(in_func, std::vector<uint8_t> &bytecodes)
	{
		ifTrue.rewrite(in_data, bytecodes);
		if (ifFalse)
			ifFalse->rewrite(in_data, bytecodes);
	}

	void CanBreakContinueNode::rewrite(in_func, std::vector<uint8_t> &bytecodes)
	{
		size_t lastContinuePos = context.continuePos;
		size_t lastBreakPos = context.breakPos;
		context.continuePos = continuePos;
		context.breakPos = breakPos;
		body.rewrite(in_data, bytecodes);
		context.continuePos = lastContinuePos;
		context.breakPos = lastBreakPos;
	}

	void WhileNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		continuePos = bytecodes.size();
		condition->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
		size_t jumpIfFalseByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		put_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		breakPos = bytecodes.size();
	}

	void ForRangeNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		// for (detach in from..to) { body }
		// detach = from
		from->putBytecodes(in_data, bytecodes);
		detach->isStore = true;
		detach->putBytecodes(in_data, bytecodes);

		// Skip
		detach->isStore = false;
		detach->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		size_t firstSkipByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		// detach++ => skip first
		continuePos = bytecodes.size();
		detach->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::PLUS_PLUS);
		put_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
		// compare
		to->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(isLessThanEq ? Opcode::LESS_THAN_EQ : Opcode::LESS_THAN);
		bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
		size_t jumpIfFalseByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		// body
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		put_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		breakPos = bytecodes.size();
	}

#define operator_plus_case(type, op)                                 \
	case Lexer::TokenType::type:                                     \
	{                                                                \
		auto detach = this->detach;                                  \
		if (detach->kind == NodeType::UNKNOW)                        \
		{                                                            \
			detach = static_cast<UnknowNode *>(detach)->correctNode; \
		}                                                            \
		auto _node = static_cast<AccessNode *>(detach);              \
		_node->isStore = false;                                      \
		_node->putBytecodes(in_data, bytecodes);                     \
		value->putBytecodes(in_data, bytecodes);                     \
		bytecodes.emplace_back(Opcode::op);                          \
		return;                                                      \
	}

	void SetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		switch (op)
		{
			operator_plus_case(PLUS_EQUAL, PLUS_EQUAL);
			operator_plus_case(MINUS_EQUAL, MINUS_EQUAL);
			operator_plus_case(STAR_EQUAL, MUL_EQUAL);
			operator_plus_case(SLASH_EQUAL, DIVIDE_EQUAL);
		default:
		{
			break;
			// throw std::runtime_error("Unexpected op "+ Lexer::Token(0, op).toString(context));
		}
		}
		value->putBytecodes(in_data, bytecodes);
		detach->putBytecodes(in_data, bytecodes);
	}

	void CallNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		// Function* func = &compile.functions[funcId];
		if (caller)
		{
			caller->putBytecodes(in_data, bytecodes);
		}
		if (addPopBytecode)
			bytecodes.emplace_back(Opcode::POP);
		if (isConstructor)
		{
			bytecodes.emplace_back(Opcode::CREATE_OBJECT);
			put_opcode_u32(bytecodes, classId);
			put_opcode_u32(bytecodes, compile.classes[classId].memberMap.size());
		}
		for (auto &argument : arguments)
		{
			argument->putBytecodes(in_data, bytecodes);
		}
		bytecodes.emplace_back(Opcode::CALL_FUNCTION);
		put_opcode_u32(bytecodes, funcId);
		// std::cout<<funcId<<'\n';
	}

	void ReturnNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		if (value)
		{
			if (value->kind == NodeType::VAR)
			{
				auto node = static_cast<VarNode *>(value);
				if (node->declaration->isGlobal)
					goto return_global;
				bytecodes.emplace_back(RETURN_LOCAL);
				put_opcode_u32(bytecodes, node->declaration->id);
				return;
			}
		return_global:;
			value->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::RETURN_VALUE);
			return;
		}
		bytecodes.emplace_back(Opcode::RETURN);
	}

	void VarNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes)
	{
		if (declaration->isGlobal)
		{
			bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL : Opcode::LOAD_GLOBAL);
		}
		else
		{
			bytecodes.emplace_back(isStore ? Opcode::STORE_LOCAL : Opcode::LOAD_LOCAL);
		}
		put_opcode_u32(bytecodes, declaration->id);
	}

}

#endif