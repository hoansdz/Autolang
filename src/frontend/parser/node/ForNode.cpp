#ifndef FOR_NODE_CPP
#define FOR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *ForNode::resolve(in_func) {
	detach = static_cast<AccessNode *>(detach->resolve(in_data));
	data = static_cast<HasClassIdNode *>(data->resolve(in_data));
	body.resolve(in_data);
	return this;
}

void ForNode::optimize(in_func) {
	detach->optimize(in_data);
	if (data->kind == NodeType::RANGE) {
		switch (detach->classId) {
			case AutoLang::DefaultClass::intClassId: {
				break;
			}
			default: {
				throwError("Detach value must be Int");
			}
		}
		auto rangeNode = static_cast<RangeNode *>(data);
		rangeNode->optimize(in_data);
		rangeNode->from->optimize(in_data);
		switch (rangeNode->from->classId) {
			case AutoLang::DefaultClass::intClassId: {
				break;
			}
			default: {
				throwError("From value must be Int");
			}
		}
		rangeNode->to->optimize(in_data);
		switch (rangeNode->to->classId) {
			case AutoLang::DefaultClass::intClassId: {
				break;
			}
			default: {
				throwError("To value must be Int");
			}
		}
		if (rangeNode->to->kind == NodeType::CONST) {
			static_cast<ConstValueNode *>(rangeNode->to)->isLoadPrimary = true;
		}
	}
	body.optimize(in_data);
}

ExprNode *ForNode::copy(in_func) {
	return context.forPool.push(
	    line, static_cast<AccessNode *>(detach->copy(in_data)),
	    static_cast<HasClassIdNode *>(data->copy(in_data)));
}

void ForNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	// for (detach in from..to) { body }
	if (data->kind == NodeType::RANGE) {
		auto rangeNode = static_cast<RangeNode *>(data);
		// detach = from
		rangeNode->from->putBytecodes(in_data, bytecodes);
		detach->isStore = true;
		detach->putBytecodes(in_data, bytecodes);

		// Skip
		detach->isStore = false;
		detach->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		size_t firstSkipByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		// detach++ => skip first
		continuePos = bytecodes.size();
		detach->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::PLUS_PLUS);
		rewrite_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
		// compare
		rangeNode->to->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(rangeNode->lessThan ? Opcode::LESS_THAN
		                                           : Opcode::LESS_THAN_EQ);
		bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
		size_t jumpIfFalseByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		// body
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		breakPos = bytecodes.size();
	}
}

ForNode::~ForNode() {
	deleteNode(detach);
	deleteNode(data);
}

} // namespace AutoLang

#endif