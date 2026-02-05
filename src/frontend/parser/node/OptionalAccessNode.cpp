#ifndef OPTIONAL_ACCESS_NODE_CPP
#define OPTIONAL_ACCESS_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *OptionalAccessNode::resolve(in_func) {
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	value->mode = mode;
	switch (value->kind) {
		case NodeType::VAR:
		case NodeType::CONST:
			return this;
		default:
			break;
	};
	return this;
}

void OptionalAccessNode::optimize(in_func) {
	value->optimize(in_data);
	classId = value->classId;
	nullable = value->isNullable();
}

void OptionalAccessNode::putBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	auto lastJumpIfNullNode = context.jumpIfNullNode;
	context.jumpIfNullNode = this;
	value->putBytecodes(in_data, bytecodes);
	context.jumpIfNullNode = lastJumpIfNullNode;

	jumpIfNullPos = bytecodes.size();
}

void OptionalAccessNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	auto lastJumpIfNullNode = context.jumpIfNullNode;
	context.jumpIfNullNode = this;
	value->rewrite(in_data, bytecodes);
	context.jumpIfNullNode = lastJumpIfNullNode;
}

OptionalAccessNode::~OptionalAccessNode() { ExprNode::deleteNode(value); }

} // namespace AutoLang

#endif