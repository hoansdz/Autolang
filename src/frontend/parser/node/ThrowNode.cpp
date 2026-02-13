#ifndef THROW_NODE_CPP
#define THROW_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *ThrowNode::resolve(in_func) {
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	return this;
}

void ThrowNode::optimize(in_func) {
	value->optimize(in_data);
	switch (value->kind) {
		case NodeType::CLASS_ACCESS:
			throwError("The thing being throw must be value");
		default: {
			if (value->isNullable()) {
				throwError("The thing being throw must be non nullable");
			}
			auto valueClass = compile.classes[value->classId];
			if (!(value->classId == DefaultClass::exceptionClassId ||
			     valueClass->inheritance.get(DefaultClass::exceptionClassId))) {
				throwError("The thing being throw must be a exception");
			}
		}
	}
}

void ThrowNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::THROW_EXCEPTION);
}

void ThrowNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {}

ThrowNode::~ThrowNode() { deleteNode(value); }

} // namespace AutoLang

#endif