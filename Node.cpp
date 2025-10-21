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

void put_opcode_u32(std::vector<uint8_t>& code, size_t pos, uint32_t value) {
	code[pos] = (value >> 0) & 0xFF;
	code[pos + 1] = (value >> 8) & 0xFF;
	code[pos + 2] = (value >> 16) & 0xFF;
	code[pos + 3] = (value >> 24) & 0xFF;
}

ReturnNode::~ReturnNode() {
	if (value)
		delete value;
}

UnaryNode::~UnaryNode() {
	if (value) {
		delete value;
	}
}

BinaryNode::~BinaryNode() {
	if (left)
		delete left;
	if (right)
		delete right;
}

ConstValueNode::~ConstValueNode() {

}

CastNode::~CastNode() {
	if (value)
		delete value;
}

ForRangeNode::~ForRangeNode() {
	delete from;
	delete to;
}

GetPropNode::~GetPropNode() {
	delete caller;
}

BlockNode::~BlockNode() {
	for (auto* node : nodes) {
		delete node;
	}
}

IfNode::~IfNode() {
	delete condition;
	if (ifFalse) delete ifFalse;
}

WhileNode::~WhileNode() {
	delete condition;
}

CallNode::~CallNode() {
	if (caller)
		delete caller;
	for (auto* argument:arguments) {
		delete argument;
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