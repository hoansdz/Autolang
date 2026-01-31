#ifndef NODE_CPP
#define NODE_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/Node.cpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

void ExprNode::throwError(std::string message) {
	throw ParserError(line, message);
}

void ExprNode::warning(in_func, std::string message) {
	context.warning(line, message);
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
		case NodeType::CREATE_CONSTRUCTOR:
		// case NodeType::BINARY:
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
	if (classId != AutoLang::DefaultClass::stringClassId || !str) return;
	delete str;
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
	return compile.classes[it->second];
}

}

#endif