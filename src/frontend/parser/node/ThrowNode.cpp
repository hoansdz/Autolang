#ifndef THROW_NODE_CPP
#define THROW_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *ThrowNode::resolve(in_func) {
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	return this;
}

void ThrowNode::optimize(in_func) {
	value->optimize(in_data);
	switch (value->kind) {
		case NodeType::CLASS_ACCESS:
			throwError("Expression in throw statement must produce a value");
		default: {
			if (value->isNullable()) {
				throwError("Cannot throw nullable expression");
			}
			auto valueClass = compile.classes[value->classId];
			if (!(value->classId == DefaultClass::exceptionClassId ||
			      valueClass->inheritance.get(
			          DefaultClass::exceptionClassId))) {
				throwError("Thrown expression must be an Exception");
			}
		}
	}
}

void ThrowNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	value->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::THROW_EXCEPTION);
}

void ThrowNode::rewrite(in_func, uint8_t *bytecodes) {}

ExprNode *ThrowNode::copy(in_func) {
	return context.throwPool.push(
	    line, static_cast<HasClassIdNode *>(value->copy(in_data)));
}

ThrowNode::~ThrowNode() { deleteNode(value); }

} // namespace Autolang

#endif