#ifndef NODE_PUT_BYTECODE_CPP
#define NODE_PUT_BYTECODE_CPP

#include "frontend/parser/node/NodePutBytecode.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

void SkipNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	bytecodes.emplace_back(Opcode::JUMP);
	jumpBytePos = bytecodes.size() - context.currentBytecodePos;
	put_opcode_u32(bytecodes, 0);
}

void SkipNode::rewrite(in_func, uint8_t *bytecodes) {
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

void CanBreakContinueNode::rewrite(in_func, uint8_t *bytecodes) {
	uint32_t lastContinuePos = context.continuePos;
	uint32_t lastBreakPos = context.breakPos;
	context.continuePos = continuePos;
	context.breakPos = breakPos;
	body.rewrite(in_data, bytecodes);
	context.continuePos = lastContinuePos;
	context.breakPos = lastBreakPos;
}

void WhileNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	continuePos = bytecodes.size() - context.currentBytecodePos;

	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (!static_cast<ConstValueNode *>(condition)->obj->b) {
			warning(in_data, "While command won't never be called here");
			return;
		}
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		breakPos = bytecodes.size() - context.currentBytecodePos;
		return;
	}

	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
	put_opcode_u32(bytecodes, 0);
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
	                   jumpIfFalseByte,
	                   bytecodes.size() - context.currentBytecodePos);
	breakPos = bytecodes.size() - context.currentBytecodePos;
}

void ReturnNode::putOptimizedBytecodes(in_func, HasClassIdNode *value,
                                       std::vector<uint8_t> &bytecodes) {
	if (value) {
		switch (value->kind) {
			case NodeType::VAR: {
				auto node = static_cast<VarNode *>(value);
				bytecodes.emplace_back(
				    node->declaration->isGlobal ? RETURN_GLOBAL : RETURN_LOCAL);
				put_opcode_u32(bytecodes, node->declaration->id);
				return;
			}
			case NodeType::CONST: {
				auto node = static_cast<ConstValueNode *>(value);
				bytecodes.emplace_back(RETURN_CONST);
				put_opcode_u32(bytecodes, node->id);
				return;
			}
			case NodeType::GET_PROP: {
				auto node = static_cast<GetPropNode *>(value);
				if (node->isStatic) {
					node->caller->putBytecodesIfMustBeCalled(in_data,
					                                         bytecodes);
					bytecodes.emplace_back(RETURN_GLOBAL);
					put_opcode_u32(bytecodes, node->declaration->id);
					return;
				}
				if (node->caller->kind != NodeType::VAR) {
					break;
				}
				auto caller = static_cast<VarNode *>(node->caller);
				bytecodes.emplace_back(caller->declaration->isGlobal
				                           ? RETURN_GLOBAL_MEMBER
				                           : RETURN_LOCAL_MEMBER);
				put_opcode_u32(bytecodes, caller->declaration->id);
				put_opcode_u32(bytecodes, node->declaration->id);
				return;
			}
		}
		value->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::RETURN_VALUE);
		return;
	}
	bytecodes.emplace_back(Opcode::RETURN);
}

void ReturnNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	putOptimizedBytecodes(in_data, value, bytecodes);
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