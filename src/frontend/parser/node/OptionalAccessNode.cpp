#ifndef OPTIONAL_ACCESS_NODE_CPP
#define OPTIONAL_ACCESS_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

void OptionalAccessNode::optimize(in_func) {
    value->optimize(in_data);
    classId = value->classId;
    nullable = value->isNullable();
}

void OptionalAccessNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
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

OptionalAccessNode::~OptionalAccessNode() {
	ExprNode::deleteNode(value);
}

}

#endif