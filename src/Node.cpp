#ifndef NODE_CPP
#define NODE_CPP

#include "Debugger.hpp"
#include "Node.hpp"
#include "CreateNode.hpp"

namespace AutoLang {

void put_opcode_u32(std::vector<uint8_t>& code, uint32_t value) {
	code.push_back((value >> 0) & 0xFF);
	code.push_back((value >> 8) & 0xFF);
	code.push_back((value >> 16) & 0xFF);
	code.push_back((value >> 24) & 0xFF);
}

void rewrite_opcode_u32(std::vector<uint8_t>& code, size_t pos, uint32_t value) {
	code[pos] = (value >> 0) & 0xFF;
	code[pos + 1] = (value >> 8) & 0xFF;
	code[pos + 2] = (value >> 16) & 0xFF;
	code[pos + 3] = (value >> 24) & 0xFF;
}

void ExprNode::deleteNode(ExprNode* node) {
	if (!node) return;
	switch (node->kind) {
		case NodeType::IF:
		case NodeType::WHILE: 
		case NodeType::RET:
		case NodeType::SET:
		case NodeType::DECLARATION:
		case NodeType::CREATE_FUNC:
		case NodeType::CREATE_CLASS:
		{
			return;
		}
		default: break;
	}
	// std::cerr<<std::to_string(node->kind)<<'\n';
	delete node;
}

UnknowNode::~UnknowNode() {
	deleteNode(correctNode);
}

ReturnNode::~ReturnNode() {
	deleteNode(value);
}

UnaryNode::~UnaryNode() {
	deleteNode(value);
}

BinaryNode::~BinaryNode() {
	deleteNode(left);
	deleteNode(right);
}

ConstValueNode::~ConstValueNode() {

}

CastNode::~CastNode() {
	deleteNode(value);
}

ForRangeNode::~ForRangeNode() {
	deleteNode(detach);
	deleteNode(from);
	deleteNode(to);
}

GetPropNode::~GetPropNode() {
	deleteNode(caller);
}

BlockNode::~BlockNode() {
	for (auto* node : nodes) {
		deleteNode(node);
	}
}

IfNode::~IfNode() {
	deleteNode(condition);
	deleteNode(ifFalse);
}

WhileNode::~WhileNode() {
	deleteNode(condition);
}

SetNode::~SetNode() {
	deleteNode(detach);
	deleteNode(value);
}

CallNode::~CallNode() {
	deleteNode(caller);
	for (auto* argument:arguments) {
		deleteNode(argument);
	}
}

AClass* findClass(in_func, std::string name) {
	auto it = compile.classMap.find(name);
	if (it == compile.classMap.end())
		return nullptr;
	return &compile.classes[it->second];
}

}

#endif