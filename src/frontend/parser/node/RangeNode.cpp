#ifndef RANGE_NODE_CPP
#define RANGE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *RangeNode::resolve(in_func) {
	from = static_cast<HasClassIdNode *>(from->resolve(in_data));
	to = static_cast<HasClassIdNode *>(to->resolve(in_data));
	return this;
}

void RangeNode::optimize(in_func) {}

ExprNode *RangeNode::copy(in_func) {
	return context.rangeNode.push(
	    line, static_cast<HasClassIdNode *>(from->copy(in_data)), static_cast<HasClassIdNode *>(to->copy(in_data)), lessThan);
}

RangeNode::~RangeNode() { 
	deleteNode(from);
	deleteNode(to); 
}

} // namespace AutoLang

#endif