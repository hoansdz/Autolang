#ifndef IF_NODE_CPP
#define IF_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *IfNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	// if (condition->kind == NodeType::CONST) {
	// 	if (static_cast<ConstValueNode *>(condition)->classId !=
	// 	    AutoLang::DefaultClass::boolClassId) {
	// 		throwError("Cannot use expression of type '" +
	// 		           condition->getClassName(in_data) +
	// 		           "' as a condition, expected 'Bool'");
	// 	}
	// 	// Is bool because optimize forbiddened others
	// 	if (static_cast<ConstValueNode *>(condition)->obj->b) {
	// 		if (ifFalse) {
	// 			warning(in_data, "Else body will never be used");
	// 		}
	// 		auto result = context.blockNodePool.push(ifTrue.line);
	// 		result->nodes = std::move(ifTrue.nodes);
	// 		result->resolve(in_data);
	// 		ExprNode::deleteNode(this);
	// 		return result;
	// 	} else if (ifFalse) {
	// 		auto result = ifFalse;
	// 		result->resolve(in_data);
	// 		ifFalse = nullptr;
	// 		ExprNode::deleteNode(this);
	// 		return result;
	// 	}
	// 	return this;
	// }
	ifTrue.resolve(in_data);
	if (ifFalse) {
		ifFalse->resolve(in_data);
	}
	return this;
}

void IfNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'");

	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
	}
	ifTrue.optimize(in_data);
	if (ifFalse)
		ifFalse->optimize(in_data);
	if (mustReturnValue) {
		context.mustReturnValueNode = lastMustReturnValueNode;
	}
}

ExprNode *IfNode::copy(in_func) {
	auto newNode = context.ifPool.push(line, mustReturnValue);
	newNode->ifTrue.nodes.reserve(ifTrue.nodes.size());
	for (auto node : ifTrue.nodes) {
		newNode->ifTrue.nodes.push_back(node->copy(in_data));
	}
	if (ifFalse) {
		newNode->ifFalse = static_cast<BlockNode *>(ifFalse->copy(in_data));
	}
	return newNode;
}

void IfNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
	}
	ifTrue.putBytecodes(in_data, bytecodes);
	if (ifFalse) {
		bytecodes.emplace_back(Opcode::JUMP);
		size_t jumpIfTrueByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		ifFalse->putBytecodes(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes, jumpIfTrueByte, bytecodes.size());
	} else {
		rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	}
	if (mustReturnValue) {
		context.mustReturnValueNode = lastMustReturnValueNode;
	}
	BytecodePos endBlock = bytecodes.size();
	for (auto pos : jumpPosition) {
		rewrite_opcode_u32(bytecodes, pos, endBlock);
	}
}

void IfNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	ifTrue.rewrite(in_data, bytecodes);
	if (ifFalse)
		ifFalse->rewrite(in_data, bytecodes);
}

IfNode::~IfNode() {
	deleteNode(condition);
	deleteNode(ifFalse);
}

} // namespace AutoLang

#endif