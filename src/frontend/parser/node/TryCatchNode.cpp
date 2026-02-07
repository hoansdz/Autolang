#ifndef TRY_CATCH_CPP
#define TRY_CATCH_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *TryCatchNode::resolve(in_func) {
	body.resolve(in_data);
	catchBody.resolve(in_data);
	return this;
}

void TryCatchNode::optimize(in_func) {
	body.optimize(in_data);
	catchBody.optimize(in_data);
}

void TryCatchNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	bytecodes.emplace_back(Opcode::ADD_TRY_BLOCK);
	Offset jumpToCatchPos = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::REMOVE_TRY_AND_JUMP);
	Offset jumpToNewCommand = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	BytecodePos startCatchPos = bytecodes.size();
	bytecodes.emplace_back(Opcode::LOAD_EXCEPTION);
	bytecodes.emplace_back(exceptionDeclaration->isGlobal
	                           ? Opcode::STORE_GLOBAL
	                           : Opcode::STORE_LOCAL);
	put_opcode_u32(bytecodes, exceptionDeclaration->id);
	catchBody.putBytecodes(in_data, bytecodes);
	rewrite_opcode_u32(bytecodes, jumpToCatchPos, startCatchPos);
	rewrite_opcode_u32(bytecodes, jumpToNewCommand, bytecodes.size());
}

void TryCatchNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	body.rewrite(in_data, bytecodes);
	catchBody.rewrite(in_data, bytecodes);
}

} // namespace AutoLang

#endif