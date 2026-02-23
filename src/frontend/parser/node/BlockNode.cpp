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

void BlockNode::loadReturnValueClassId(in_func, uint32_t line,
                                       std::optional<ClassId> &currentClassId,
                                       ClassId newClassId) {
	if (!currentClassId) {
		currentClassId = newClassId;
		return;
	}
	if (*currentClassId == newClassId) {
		return;
	}
	if ((*currentClassId == DefaultClass::intClassId ||
	     *currentClassId == DefaultClass::floatClassId) &&
	    (newClassId == DefaultClass::intClassId ||
	     newClassId == DefaultClass::floatClassId)) {
		currentClassId = DefaultClass::floatClassId;
		return;
	}
	if (compile.classes[*currentClassId]->inheritance.get(newClassId)) {
		currentClassId = newClassId;
		return;
	}
	if (compile.classes[newClassId]->inheritance.get(*currentClassId)) {
		return;
	}
	throw ParserError(line,
	                  "Cannot cast '" + compile.classes[*currentClassId]->name +
	                      "' to '" + compile.classes[newClassId]->name + "'");
}

void BlockNode::loadClassAndOptimize(in_func) {
	std::optional<ClassId> currentClassId;
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto *node = nodes[i];
		node->optimize(in_data);
		switch (node->kind) {
			case NodeType::CALL: {
				auto *n = static_cast<CallNode *>(node);
				if (n->classId == DefaultClass::voidClassId)
					break;
				loadReturnValueClassId(in_data, line, currentClassId,
				                       n->classId);
				break;
			}
			case NodeType::CONST:
			case NodeType::BINARY:
			case NodeType::GET_PROP:
			case NodeType::VAR: {
				loadReturnValueClassId(
				    in_data, line, currentClassId,
				    static_cast<HasClassIdNode *>(node)->classId);
				break;
			}
			case NodeType::IF: {
				auto *n = static_cast<IfNode *>(node);
				if (n->mustReturnValue) {
					loadReturnValueClassId(in_data, line, currentClassId,
					                       n->classId);
					break;
				}
				break;
			}
			default:
				break;
		}
	}
	if (!currentClassId) {
		throwError("Expression branch must return a value");
	}
	if (context.mustReturnValueNode->classId == DefaultClass::nullClassId) {
		context.mustReturnValueNode->classId = *currentClassId;
		return;
	}
	loadReturnValueClassId(in_data, line, currentClassId,
	                       context.mustReturnValueNode->classId);
	context.mustReturnValueNode->classId = *currentClassId;
}

void BlockNode::optimize(in_func) {
	if (context.mustReturnValueNode) {
		loadClassAndOptimize(in_data);
		return;
	}
	for (auto *node : nodes) {
		node->optimize(in_data);
	}
}

void BlockNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.mustReturnValueNode) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			auto *node = nodes[i];
			node->putBytecodes(in_data, bytecodes);
			switch (node->kind) {
				case NodeType::CALL: {
					auto *n = static_cast<CallNode *>(node);
					if (n->classId == DefaultClass::voidClassId)
						break;
					if (i != nodes.size() - 1) {
						bytecodes.emplace_back(Opcode::JUMP);
						context.mustReturnValueNode->jumpPosition.push_back(
						    bytecodes.size());
						put_opcode_u32(bytecodes, 0);
					}
					break;
				}
				case NodeType::CONST:
				case NodeType::BINARY:
				case NodeType::GET_PROP:
				case NodeType::VAR: {
					if (i != nodes.size() - 1) {
						bytecodes.emplace_back(Opcode::JUMP);
						context.mustReturnValueNode->jumpPosition.push_back(
						    bytecodes.size());
						put_opcode_u32(bytecodes, 0);
					}
					break;
				}
				case NodeType::IF: {
					auto *n = static_cast<IfNode *>(node);
					if (!n->mustReturnValue)
						break;
					if (i != nodes.size() - 1) {
						bytecodes.emplace_back(Opcode::JUMP);
						context.mustReturnValueNode->jumpPosition.push_back(
						    bytecodes.size());
						put_opcode_u32(bytecodes, 0);
					}
					break;

					break;
				}
				default:
					break;
			}
		}
		return;
	}
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
	BlockNode *newNode = context.blockNodePool.push(line);
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