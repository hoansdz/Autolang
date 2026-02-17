#ifndef BLOCK_NODE_CPP
#define BLOCK_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *BlockNode::resolve(in_func) {
	ParserContext::mode = mode;
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto &node = nodes[i];
		node = node->resolve(in_data);
	}
	return this;
}

void BlockNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *node : nodes)
		node->rewrite(in_data, bytecodes);
}

void BlockNode::refresh() {
	for (auto *node : nodes) {
		ExprNode::deleteNode(node);
	}
	nodes.clear();
}

void BlockNode::optimize(in_func) {
	for (auto *node : nodes) {
		node->optimize(in_data);
	}
}

void BlockNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *node : nodes) {
		switch (node->kind) {
			case NodeType::CALL: {
				auto currentNode = static_cast<CallNode *>(node);
				node->putBytecodes(in_data, bytecodes);
				// if (currentNode->isSuper) break;
				if (currentNode->classId != DefaultClass::voidClassId)
					bytecodes.emplace_back(currentNode->isSuper
					                           ? Opcode::POP_NO_RELEASE
					                           : Opcode::POP);
				break;
			}
			case NodeType::OPTIONAL_ACCESS: {
				auto currentNode = static_cast<OptionalAccessNode *>(node);
				currentNode->returnNullIfNull = false;
				node->putBytecodes(in_data, bytecodes);
				if (currentNode->value->kind != NodeType::CALL ||
				    currentNode->value->classId != DefaultClass::voidClassId)
					bytecodes.emplace_back(Opcode::POP);
				currentNode->jumpIfNullPos = bytecodes.size();
				break;
			}
			case NodeType::UNARY:
			case NodeType::NULL_COALESCING:
			case NodeType::BINARY:
			case NodeType::CAST:
			case NodeType::GET_PROP: {
				node->putBytecodes(in_data, bytecodes);
				bytecodes.emplace_back(Opcode::POP);
				break;
			}
			case NodeType::CLASS_ACCESS:
			case NodeType::CONST:
			case NodeType::VAR: {
				break;
			}
			default: {
				node->putBytecodes(in_data, bytecodes);
				break;
			}
		}
	}
}

ExprNode *BlockNode::copy(in_func) {
	BlockNode* newNode = context.blockNodePool.push(line);
	newNode->mode = mode;
	newNode->nodes.reserve(nodes.size());
	for (auto &node : nodes) {
		newNode->nodes.push_back(node);
	}
	return newNode;
}

BlockNode::~BlockNode() {
	for (auto *node : nodes) {
		deleteNode(node);
	}
}

} // namespace AutoLang

#endif