#ifndef IF_NODE_CPP
#define IF_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *IfNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	// if (condition->kind == NodeType::CONST_VAL) {
	// 	if (static_cast<ConstValueNode *>(condition)->classId !=
	// 	    Autolang::DefaultClass::boolClassId) {
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
	if (condition->classId != Autolang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'\nHint: Ensure the condition expression evaluates to a 'Bool' value or explicit boolean comparison.");

	auto lastMustReturnValueNode = context.mustReturnValueNode;
	bool loadReturnBlock = mustReturnValue || lastMustReturnValueNode;
	if (loadReturnBlock) {
		context.mustReturnValueNode = this;
	}
	if (!ifFalse || mustReturnValue) {
		ifTrue.optimize(in_data);
		ClassId trueClassId = classId;
		if (ifFalse) {
			ifFalse->optimize(in_data);
		} else {
			if (ifTrue.hasValue && condition->kind == NodeType::CONST_VAL &&
			    static_cast<ConstValueNode *>(condition)->obj->b) {
				mustReturnValue = true;
			}
		}
		if (classId == DefaultClass::floatClassId &&
		    trueClassId == DefaultClass::intClassId) {
			ifTrue.autoCastToFloat = true;
		}
	} else {
		ifTrue.optimize(in_data);
		ClassId trueClassId = classId;
		if (ifFalse)
			ifFalse->optimize(in_data);
		if (classId == DefaultClass::floatClassId &&
		    trueClassId == DefaultClass::intClassId) {
			ifTrue.autoCastToFloat = true;
		}
		if (ifTrue.hasValue && ifFalse->hasValue) {
			mustReturnValue = true;
		}
	}
	// std::cerr << getClassName(in_data) << "\n";
	if (loadReturnBlock) {
		if (classId == DefaultClass::nullClassId && nullable) {
			throwError("Cannot infer return type for 'if' expression because "
			           "its body is a null literal\nHint: Explicitly specify the type or ensure the expression body returns a concrete typed value.");
		}
		context.mustReturnValueNode = lastMustReturnValueNode;
	}
}

ExprNode *IfNode::copy(in_func) {
	auto newNode = context.ifPool.push(line, mustReturnValue);
	newNode->condition =
	    static_cast<HasClassIdNode *>(condition->copy(in_data));
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
	loadOpcodeLine(in_data, bytecodes);
	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
	put_opcode_u32(bytecodes, 0);
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
	} else {
		context.mustReturnValueNode = nullptr;
	}
	ifTrue.putBytecodes(in_data, bytecodes);
	if (ifFalse) {
		bytecodes.emplace_back(Opcode::JUMP);
		size_t jumpIfTrueByte = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfFalseByte,
		                   bytecodes.size() - context.currentBytecodePos);
		ifFalse->putBytecodes(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfTrueByte,
		                   bytecodes.size() - context.currentBytecodePos);
	} else {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfFalseByte,
		                   bytecodes.size() - context.currentBytecodePos);
	}
	context.mustReturnValueNode = lastMustReturnValueNode;
	BytecodePos endBlock = bytecodes.size() - context.currentBytecodePos;
	for (auto pos : jumpPosition) {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos, pos,
		                   endBlock);
	}
}

void IfNode::rewrite(in_func, uint8_t *bytecodes) {
	ifTrue.rewrite(in_data, bytecodes);
	if (ifFalse)
		ifFalse->rewrite(in_data, bytecodes);
}

IfNode::~IfNode() {
	deleteNode(condition);
	deleteNode(ifFalse);
}

} // namespace Autolang

#endif