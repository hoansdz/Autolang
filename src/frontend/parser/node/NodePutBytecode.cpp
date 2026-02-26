#ifndef NODE_PUT_BYTECODE_CPP
#define NODE_PUT_BYTECODE_CPP

#include "frontend/parser/node/NodePutBytecode.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

void UnaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::MINUS: {
			bytecodes.emplace_back(Opcode::NEGATIVE);
			break;
		}
		case Lexer::TokenType::NOT: {
			bytecodes.emplace_back(Opcode::NOT);
			break;
		}
		default:
			break;
	}
}

void SkipNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	bytecodes.emplace_back(Opcode::JUMP);
	jumpBytePos = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
}

void SkipNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	switch (type) {
		case Lexer::TokenType::CONTINUE:
			rewrite_opcode_u32(bytecodes, jumpBytePos, context.continuePos);
			break;
		case Lexer::TokenType::BREAK:
			rewrite_opcode_u32(bytecodes, jumpBytePos, context.breakPos);
			break;
		default:
			throwError("Wrong type");
	}
}

void ConstValueNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (classId == AutoLang::DefaultClass::nullClassId) {
		bytecodes.emplace_back(Opcode::LOAD_NULL);
		return;
	}
	if (classId == AutoLang::DefaultClass::boolClassId) {
		bytecodes.emplace_back(obj->b ? Opcode::LOAD_TRUE : LOAD_FALSE);
		return;
	}
	bytecodes.emplace_back(isLoadPrimary ? Opcode::LOAD_CONST_PRIMARY
	                                     : Opcode::LOAD_CONST);
	put_opcode_u32(bytecodes, id);
}

void GetPropNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (!isStatic) {
		caller->putBytecodes(in_data, bytecodes);
		if (isStore) {
			if (accessNullable) {
				throwError(
				    "Bug: Setnode not ensure store data is non nullable");
			}
			bytecodes.emplace_back(Opcode::STORE_MEMBER);
			put_opcode_u32(bytecodes, id);
			return;
		}
		if (accessNullable) {
			assert(context.jumpIfNullNode != nullptr);
			bytecodes.emplace_back(context.jumpIfNullNode->returnNullIfNull
			                           ? Opcode::LOAD_MEMBER_CAN_RET_NULL
			                           : Opcode::LOAD_MEMBER_IF_NNULL);
		} else {
			bytecodes.emplace_back(Opcode::LOAD_MEMBER);
		}
		put_opcode_u32(bytecodes, id);
		return;
	}
	switch (caller->kind) {
		case NodeType::VAR: {
			break;
		}
		default: {
			caller->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::POP);
			break;
		}
	}
	if (accessNullable) {
		if (isStore) {
			throwError("Bug: Setnode not ensure store data is non nullable");
		}
		warning(in_data, "Access static variables: we recommend call " +
		                     compile.classes[caller->classId]->name + "." +
		                     name);
		accessNullable = false;
	}
	bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL
	                               : Opcode::LOAD_GLOBAL);
	put_opcode_u32(bytecodes, id);
}

void GetPropNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.jumpIfNullNode) {
		caller->rewrite(in_data, bytecodes);
		if (accessNullable && isStore) {
			rewrite_opcode_u32(bytecodes, jumpIfNullPos,
			                   context.jumpIfNullNode->jumpIfNullPos);
		}
	}
}

void CanBreakContinueNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	uint32_t lastContinuePos = context.continuePos;
	uint32_t lastBreakPos = context.breakPos;
	context.continuePos = continuePos;
	context.breakPos = breakPos;
	body.rewrite(in_data, bytecodes);
	context.continuePos = lastContinuePos;
	context.breakPos = lastBreakPos;
}

void WhileNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	continuePos = bytecodes.size();

	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (!static_cast<ConstValueNode *>(condition)->obj->b) {
			warning(in_data, "While command won't never be called here");
			return;
		}
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		breakPos = bytecodes.size();
		return;
	}

	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	breakPos = bytecodes.size();
}

#define operator_plus_case(type, op)                                           \
	case Lexer::TokenType::type: {                                             \
		auto _node = static_cast<AccessNode *>(detach);                        \
		_node->isStore = false;                                                \
		_node->putBytecodes(in_data, bytecodes);                               \
		value->putBytecodes(in_data, bytecodes);                               \
		bytecodes.emplace_back(Opcode::op);                                    \
		return;                                                                \
	}

void SetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	switch (op) {
		operator_plus_case(PLUS_EQUAL, PLUS_EQUAL);
		operator_plus_case(MINUS_EQUAL, MINUS_EQUAL);
		operator_plus_case(STAR_EQUAL, MUL_EQUAL);
		operator_plus_case(SLASH_EQUAL, DIVIDE_EQUAL);
		default: {
			break;
			// throwError("Unexpected op "+ Lexer::Token(0,
			// op).toString(context));
		}
	}
	value->putBytecodes(in_data, bytecodes);
	detach->putBytecodes(in_data, bytecodes);
}

void ReturnNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (value) {
		if (value->kind == NodeType::VAR) {
			auto node = static_cast<VarNode *>(value);
			if (node->declaration->isGlobal)
				goto return_global;
			bytecodes.emplace_back(RETURN_LOCAL);
			put_opcode_u32(bytecodes, node->declaration->id);
			return;
		}
	return_global:;
		size_t f1 = bytecodes.size();
		value->putBytecodes(in_data, bytecodes);
		size_t f2 = bytecodes.size();
		if (f1 == f2) {
			std::cerr<<value->getNodeType();
			std::cerr<<" WTF\n";
		}
		bytecodes.emplace_back(Opcode::RETURN_VALUE);
		return;
	}
	bytecodes.emplace_back(Opcode::RETURN);
}

void VarNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (declaration->isGlobal) {
		bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL
		                               : Opcode::LOAD_GLOBAL);
	} else {
		bytecodes.emplace_back(isStore ? Opcode::STORE_LOCAL
		                               : Opcode::LOAD_LOCAL);
	}
	put_opcode_u32(bytecodes, declaration->id);
}

} // namespace AutoLang

#endif