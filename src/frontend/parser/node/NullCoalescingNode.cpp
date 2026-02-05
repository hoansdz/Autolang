#ifndef NULL_COALESCING_NODE_CPP
#define NULL_COALESCING_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode* NullCoalescingNode::resolve(in_func) {
	left = static_cast<HasClassIdNode*>(left->resolve(in_data));
	right = static_cast<HasClassIdNode*>(right->resolve(in_data));
	left->mode = mode;
	right->mode = mode;
	switch (left->kind) {
		case NodeType::CONST: {
			if (left->classId == DefaultClass::nullClassId) {
				warning(in_data, "Left expression will never be used");
				auto result = right;
				right = nullptr;
				ExprNode::deleteNode(this);
				return result;
			}
			warning(in_data, "Right expression will never be used");
			auto result = left;
			left = nullptr;
			ExprNode::deleteNode(this);
			return result;
		}
		default: break;
	}
	// If an expression is expicitly marked non-nullable (via '!'),
	// It guaranteed by language semantics to never produce null.
	// Unknown case are treated as nullable at resolve time
	// Example: in `if (a != null) { a ?? b }`, a is still considered nullable in resolve.
	if (!left->isNullable()) {
		warning(in_data, "Right expression will never be used");
		auto result = left;
		left = nullptr;
		ExprNode::deleteNode(this);
		return result;
	}
	return this;
}

void NullCoalescingNode::optimize(in_func) {
	left->optimize(in_data);
	right->optimize(in_data);
	if (!left->isNullable()) {
		warning(in_data, "Right expression won't never be used");
	} else {
		if (left->classId == DefaultClass::nullClassId) {
			warning(in_data, "Left expression won't never be used");
			ExprNode::deleteNode(left);
			left = nullptr;
			classId = right->classId;
			return;
		}
	}
	classId = left->classId;
	if (left->classId != right->classId) {
		throwError("Left and right must same type");
	}
	nullable = right->isNullable();
}

void NullCoalescingNode::putBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	if (!left) {
		right->putBytecodes(in_data, bytecodes);
		return;
	}
	auto lastJumpIfNullNode = context.jumpIfNullNode;
	context.jumpIfNullNode = this;
	if (left->kind == NodeType::OPTIONAL_ACCESS) {
		static_cast<OptionalAccessNode *>(left)->value->putBytecodes(in_data,
		                                                             bytecodes);
	} else {
		left->putBytecodes(in_data, bytecodes);
	}
	bytecodes.emplace_back(Opcode::JUMP_AND_DELETE_IF_NULL);
	uint32_t rCheckAndJumpPos = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	bytecodes.emplace_back(Opcode::JUMP);
	uint32_t rJumpIfNonNullPos = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	jumpIfNullPos = bytecodes.size();
	context.jumpIfNullNode = lastJumpIfNullNode;
	right->putBytecodes(in_data, bytecodes);

	rewrite_opcode_u32(bytecodes, rCheckAndJumpPos, jumpIfNullPos);
	rewrite_opcode_u32(bytecodes, rJumpIfNonNullPos, bytecodes.size());
}

void NullCoalescingNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	if (!left) {
		right->rewrite(in_data, bytecodes);
		return;
	}
	auto lastJumpIfNullNode = context.jumpIfNullNode;
	context.jumpIfNullNode = this;
	if (left->kind == NodeType::OPTIONAL_ACCESS) {
		static_cast<OptionalAccessNode *>(left)->value->rewrite(in_data,
		                                                        bytecodes);
	} else {
		left->rewrite(in_data, bytecodes);
	}
	context.jumpIfNullNode = lastJumpIfNullNode;
	right->rewrite(in_data, bytecodes);
}

NullCoalescingNode::~NullCoalescingNode() {
	deleteNode(left);
	deleteNode(right);
}

} // namespace AutoLang

#endif