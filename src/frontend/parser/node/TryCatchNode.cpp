#ifndef TRY_CATCH_CPP
#define TRY_CATCH_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *TryCatchNode::resolve(in_func) {
	body.resolve(in_data);
	if (hasCatch) {
		catchBody.resolve(in_data);
	}
	if (hasFinally) {
		finallyBody.resolve(in_data);
	}
	return this;
}

ExprNode *TryCatchNode::optimize(in_func) {
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	context.mustReturnValueNode = nullptr;
	body.optimize(in_data);
	if (hasCatch) {
		catchBody.optimize(in_data);
	}
	if (hasFinally) {
		finallyBody.optimize(in_data);
	}
	context.mustReturnValueNode = lastMustReturnValueNode;
	return this;
}

void TryCatchNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (hasFinally && !hasCatch) {
		bytecodes.emplace_back(Opcode::ADD_FINALLY_BLOCK);
		Offset jumpToFinallyPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		body.putBytecodes(in_data, bytecodes);

		bytecodes.emplace_back(Opcode::REMOVE_FINALLY);
		BytecodePos startFinallyPos = bytecodes.size() - context.currentBytecodePos;
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToFinallyPos, startFinallyPos);
		finallyBody.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::END_FINALLY);
	} else if (hasFinally && hasCatch) {
		bytecodes.emplace_back(Opcode::ADD_FINALLY_BLOCK);
		Offset jumpToFinallyPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		bytecodes.emplace_back(Opcode::ADD_TRY_BLOCK);
		Offset jumpToCatchPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		body.putBytecodes(in_data, bytecodes);

		bytecodes.emplace_back(Opcode::REMOVE_TRY_AND_JUMP);
		Offset jumpToAfterCatch = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		BytecodePos startCatchPos = bytecodes.size() - context.currentBytecodePos;
		bytecodes.emplace_back(Opcode::LOAD_EXCEPTION);
		bytecodes.emplace_back(exceptionDeclaration->isGlobal
		                           ? Opcode::STORE_GLOBAL
		                           : Opcode::STORE_LOCAL);
		put_opcode_u32(bytecodes, exceptionDeclaration->id);
		catchBody.putBytecodes(in_data, bytecodes);

		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToCatchPos, startCatchPos);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToAfterCatch,
		                   bytecodes.size() - context.currentBytecodePos);

		bytecodes.emplace_back(Opcode::REMOVE_FINALLY);
		BytecodePos startFinallyPos = bytecodes.size() - context.currentBytecodePos;
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToFinallyPos, startFinallyPos);
		finallyBody.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::END_FINALLY);
	} else {
		bytecodes.emplace_back(Opcode::ADD_TRY_BLOCK);
		Offset jumpToCatchPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::REMOVE_TRY_AND_JUMP);
		Offset jumpToNewCommand = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		BytecodePos startCatchPos = bytecodes.size() - context.currentBytecodePos;
		bytecodes.emplace_back(Opcode::LOAD_EXCEPTION);
		bytecodes.emplace_back(exceptionDeclaration->isGlobal
		                           ? Opcode::STORE_GLOBAL
		                           : Opcode::STORE_LOCAL);
		put_opcode_u32(bytecodes, exceptionDeclaration->id);
		catchBody.putBytecodes(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToCatchPos, startCatchPos);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToNewCommand,
		                   bytecodes.size() - context.currentBytecodePos);
	}
}

void TryCatchNode::rewrite(in_func, uint8_t *bytecodes) {
	body.rewrite(in_data, bytecodes);
	if (hasCatch) {
		catchBody.rewrite(in_data, bytecodes);
	}
	if (hasFinally) {
		finallyBody.rewrite(in_data, bytecodes);
	}
}

ExprNode *TryCatchNode::copy(in_func) {
	auto newNode = context.tryCatchPool.push(line);
	newNode->hasCatch = hasCatch;
	newNode->hasFinally = hasFinally;
	newNode->exceptionDeclaration = exceptionDeclaration; //???
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto node : body.nodes) {
		newNode->body.nodes.push_back(node->copy(in_data));
	}
	if (hasCatch) {
		newNode->catchBody.nodes.reserve(catchBody.nodes.size());
		for (auto node : catchBody.nodes) {
			newNode->catchBody.nodes.push_back(node->copy(in_data));
		}
	}
	if (hasFinally) {
		newNode->finallyBody.nodes.reserve(finallyBody.nodes.size());
		for (auto node : finallyBody.nodes) {
			newNode->finallyBody.nodes.push_back(node->copy(in_data));
		}
	}
	return newNode;
}

} // namespace Autolang

#endif