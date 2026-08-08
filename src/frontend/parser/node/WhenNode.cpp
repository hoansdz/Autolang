#ifndef WHEN_NODE_CPP
#define WHEN_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *WhenNode::resolve(in_func) {
	if (ifNode == nullptr)
		return this;
	if (value)
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	ifNode->resolve(in_data);
	return this;
}

void WhenNode::optimize(in_func) {
	if (isForceNonNull) {
		ifNode->isForceNonNull = true;
	}
	if (value) {
		value->optimize(in_data);
	}
	if (ifNode) {
		if (classId != DefaultClass::nullClassId) {
			ifNode->classId = classId;
			if (classId == DefaultClass::functionClassId) {
				ifNode->classDeclaration = classDeclaration;
			}
			ifNode->nullable = nullable;
			ifNode->optimize(in_data);
			return;
		}
		ifNode->optimize(in_data);
		classId = ifNode->classId;
		nullable = ifNode->nullable;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = ifNode->classDeclaration;
		}
	}
}

void WhenNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	if (ifNode == nullptr)
		return;
	ifNode->putBytecodes(in_data, bytecodes);
}

void WhenNode::rewrite(in_func, uint8_t *bytecodes) {
	if (value)
		value->rewrite(in_data, bytecodes);
	if (ifNode == nullptr)
		return;
	ifNode->rewrite(in_data, bytecodes);
}

ExprNode *WhenNode::copy(in_func) {
	if (ifNode == nullptr)
		return this;
	auto newValue =
	    value ? static_cast<HasClassIdNode *>(value->copy(in_data)) : nullptr;
	return context.whenNodePool.push(
	    line, newValue, static_cast<IfNode *>(ifNode->copy(in_data)));
}

WhenNode::~WhenNode() {}

} // namespace Autolang

#endif