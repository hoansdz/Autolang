#ifndef WHEN_NODE_CPP
#define WHEN_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *WhenNode::resolve(in_func) {
	if (ifNode == nullptr)
		return this;
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	ifNode->resolve(in_data);
	return this;
}

void WhenNode::optimize(in_func) {
	value->optimize(in_data);
	if (ifNode) {
		ifNode->optimize(in_data);
		classId = ifNode->classId;
	}
}

void WhenNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (ifNode == nullptr)
		return;
	ifNode->putBytecodes(in_data, bytecodes);
}

void WhenNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	if (ifNode == nullptr)
		return;
	ifNode->rewrite(in_data, bytecodes);
}

ExprNode *WhenNode::copy(in_func) {
	if (ifNode == nullptr)
		this;
	return context.whenNodePool.push(
	    line, static_cast<HasClassIdNode *>(value->copy(in_data)),
	    static_cast<IfNode *>(ifNode->copy(in_data)));
}

WhenNode::~WhenNode() {}

} // namespace AutoLang

#endif