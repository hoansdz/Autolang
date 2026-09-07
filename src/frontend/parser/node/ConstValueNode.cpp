#ifndef CONST_VALUE_NODE_CPP
#define CONST_VALUE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *ConstValueNode::optimize(in_func) {
	if (id != UINT32_MAX)
		return this;
	switch (classId) {
		case Autolang::DefaultClass::intClassId:
			id = compile.registerConstPool<int64_t>(context.constIntMap, i);
			return this;
		case Autolang::DefaultClass::floatClassId:
			id = compile.registerConstPool<double>(context.constFloatMap, f);
			return this;
		case Autolang::DefaultClass::stringClassId:
			id = compile.registerConstPool(context.constStringMap,
			                               AString::from(*str));
			return this;
		default:
			break;
	}
	return this;
}

ExprNode *ConstValueNode::copy(in_func) { return this; }

void ConstValueNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (classId == Autolang::DefaultClass::nullClassId) {
		bytecodes.emplace_back(Opcode::LOAD_NULL);
		return;
	}
	if (classId == Autolang::DefaultClass::boolClassId) {
		bytecodes.emplace_back(obj->b ? Opcode::LOAD_TRUE : LOAD_FALSE);
		return;
	}
	bytecodes.emplace_back(isLoadPrimary ? Opcode::LOAD_CONST_PRIMARY
	                                     : Opcode::LOAD_CONST);
	put_opcode_u32(bytecodes, id);
}

ConstValueNode::~ConstValueNode() {
	if (classId != Autolang::DefaultClass::stringClassId || !str)
		return;
	delete str;
}

} // namespace Autolang

#endif