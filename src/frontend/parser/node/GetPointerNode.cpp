#ifndef GET_POINTER_NODE_CPP
#define GET_POINTER_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *GetPointerNode::resolve(in_func) { return value->resolve(in_data); }

void GetPointerNode::optimize(in_func) {
	value->optimize(in_data);
	classId = value->classId;
	nullable = value->isNullable();
	switch (value->kind) {
		case NodeType::VAR:
		case NodeType::GET_PROP: {
			auto node = static_cast<AccessNode *>(value);
			node->cloneable = false;
			break;
		}
		default: {
			throwError("Cannot take the address of this expression; '&' "
			           "requires a variable or a member.");
		}
	}
	switch (classId) {
		case DefaultClass::intClassId:
		case DefaultClass::floatClassId: {
			break;
		}
		default: {
			throwError(
			    "Operator '&' can only be applied to primitive types (Int, "
			    "Float, Bool). Class types are pointers by default.");
		}
	}
}

void GetPointerNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
}

void GetPointerNode::rewrite(in_func, uint8_t *bytecodes) {}

ExprNode *GetPointerNode::copy(in_func) {
	return context.getPointerPool.push(
	    line, static_cast<HasClassIdNode *>(value->copy(in_data)));
}

} // namespace AutoLang

#endif