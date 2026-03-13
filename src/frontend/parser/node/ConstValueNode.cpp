#ifndef CONST_VALUE_NODE_CPP
#define CONST_VALUE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

void ConstValueNode::optimize(in_func) {
	if (id != UINT32_MAX)
		return;
	switch (classId) {
		case AutoLang::DefaultClass::intClassId:
			id = compile.registerConstPool<int64_t>(context.constIntMap, i);
			return;
		case AutoLang::DefaultClass::floatClassId:
			id = compile.registerConstPool<double>(context.constFloatMap, f);
			return;
		case AutoLang::DefaultClass::stringClassId:
			id = compile.registerConstPool(context.constStringMap,
			                               AString::from(*str));
			return;
		default:
			break;
	}
}

ExprNode *ConstValueNode::copy(in_func) {
	switch (classId) {
		case DefaultClass::intClassId:
			return context.constValuePool.push(line, i);
		case DefaultClass::floatClassId:
			return context.constValuePool.push(line, f);
		case DefaultClass::boolClassId:
			return context.constValuePool.push(line, bool(obj->b));
		case DefaultClass::stringClassId:
			return context.constValuePool.push(line, *str);
		default:
			return context.constValuePool.push(line, obj, id);
	}
}

void ConstValueNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (classId == AutoLang::DefaultClass::nullClassId) {
		bytecodes.emplace_back(Opcode::LOAD_NULL);
		return;
	}
	if (classId == AutoLang::DefaultClass::boolClassId) {
		bytecodes.emplace_back(obj->b ? Opcode::LOAD_TRUE : LOAD_FALSE);
		return;
	}
	bytecodes.emplace_back(isLoadPrimary ? Opcode::LOAD_CONST_PRIMARY
	                                     : Opcode::LOAD_CONST);
	put_opcode_u32(bytecodes, id);
}

ConstValueNode::~ConstValueNode() {
	if (classId != AutoLang::DefaultClass::stringClassId || !str)
		return;
	delete str;
}

} // namespace AutoLang

#endif