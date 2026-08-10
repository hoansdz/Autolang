#ifndef RANGE_NODE_CPP
#define RANGE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *RangeNode::resolve(in_func) {
	from = static_cast<HasClassIdNode *>(from->resolve(in_data));
	to = static_cast<HasClassIdNode *>(to->resolve(in_data));
	return this;
}

void RangeNode::optimize(in_func) {
	from->optimize(in_data);
	if (from->kind == NodeType::CONST_VAL) {
		static_cast<ConstValueNode *>(from)->isLoadPrimary = true;
	}
	if (from->isNullable()) {
		throwError("Type mismatch: inferred type is " +
		           compile.classes[from->classId]->getName(compile) +
		           "? but Int was expected\nHint: The 'from' bound of range expression (..) cannot be nullable. Ensure the start value is non-null Int or unwrap it using '!'.");
	}
	to->optimize(in_data);
	if (to->kind == NodeType::CONST_VAL) {
		static_cast<ConstValueNode *>(to)->isLoadPrimary = true;
	}
	if (to->isNullable()) {
		throwError("Type mismatch: inferred type is " +
		           compile.classes[to->classId]->getName(compile) +
		           "? but Int was expected\nHint: The 'to' bound of range expression (..) cannot be nullable. Ensure the end value is non-null Int or unwrap it using '!'.");
	}
}

void RangeNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	from->putBytecodes(in_data, bytecodes);
	to->putBytecodes(in_data, bytecodes);
}

void RangeNode::rewrite(in_func, uint8_t *bytecodes) {
	from->rewrite(in_data, bytecodes);
	to->rewrite(in_data, bytecodes);
}

ExprNode *RangeNode::copy(in_func) {
	return context.rangeNode.push(
	    line, static_cast<HasClassIdNode *>(from->copy(in_data)),
	    static_cast<HasClassIdNode *>(to->copy(in_data)), lessThan);
}

RangeNode::~RangeNode() {
	deleteNode(from);
	deleteNode(to);
}

} // namespace Autolang

#endif