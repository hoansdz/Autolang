#ifndef NODE_CPP
#define NODE_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/Node.cpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

ExprNode::ExprNode(NodeType kind, uint32_t line) : line(line), kind(kind) {
	mode = ParserContext::mode;
}

void ExprNode::throwError(std::string message) {
	ParserContext::mode = mode;
	throw ParserError(line, message);
}

void ExprNode::warning(in_func, std::string message) {
	auto lastMode = context.mode;
	context.mode = mode;
	context.warning(line, message);
	context.mode = lastMode;
}

#define STRINGIFY(x) #x
#define GET_NODE_TYPE_CASE(x) case x: return STRINGIFY(x);

std::string ExprNode::getNodeType() {
    switch (kind) {
        GET_NODE_TYPE_CASE(UNKNOW)
        GET_NODE_TYPE_CASE(VAR)
        GET_NODE_TYPE_CASE(BINARY)
        GET_NODE_TYPE_CASE(CONST)
        GET_NODE_TYPE_CASE(GET_PROP)
        GET_NODE_TYPE_CASE(DECLARATION)
        GET_NODE_TYPE_CASE(CALL)
        GET_NODE_TYPE_CASE(CAST)
        GET_NODE_TYPE_CASE(SET)
        GET_NODE_TYPE_CASE(UNARY)
        GET_NODE_TYPE_CASE(IF)
        GET_NODE_TYPE_CASE(BLOCK)
        GET_NODE_TYPE_CASE(WHILE)
        GET_NODE_TYPE_CASE(FOR)
        GET_NODE_TYPE_CASE(CREATE_FUNC)
        GET_NODE_TYPE_CASE(CREATE_CLASS)
        GET_NODE_TYPE_CASE(CREATE_CONSTRUCTOR)
        GET_NODE_TYPE_CASE(RET)
        GET_NODE_TYPE_CASE(SKIP)
        GET_NODE_TYPE_CASE(CLASS_ACCESS)
        GET_NODE_TYPE_CASE(OPTIONAL_ACCESS)
        GET_NODE_TYPE_CASE(NULL_COALESCING)
        GET_NODE_TYPE_CASE(TRY_CATCH)
        GET_NODE_TYPE_CASE(THROW)
        GET_NODE_TYPE_CASE(RUNTIME_CAST)
        GET_NODE_TYPE_CASE(GENERIC_DECLARATION)
    }

    return "UNKNOWN_NODE_TYPE";
}

void ExprNode::deleteNode(ExprNode* node) {
	// if (!node) return;
	// switch (node->kind) {
	// 	case NodeType::IF:
	// 	case NodeType::WHILE:
	// 	case NodeType::TRY_CATCH: 
	// 	case NodeType::THROW: 
	// 	case NodeType::RET:
	// 	case NodeType::SET:
	// 	case NodeType::DECLARATION:
	// 	case NodeType::CREATE_FUNC:
	// 	case NodeType::CREATE_CLASS:
	// 	case NodeType::CREATE_CONSTRUCTOR:
	// 	// case NodeType::BINARY:
	// 	{
	// 		return;
	// 	}
	// 	default: break;
	// }
	// // std::cerr<<std::to_string(node->kind)<<'\n';
	// delete node;
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

GetPropNode::~GetPropNode() {
	deleteNode(caller);
}

WhileNode::~WhileNode() {
	deleteNode(condition);
}

SetNode::~SetNode() {
	deleteNode(detach);
	deleteNode(value);
}

AClass* findClass(in_func, std::string name) {
	auto it = compile.classMap.find(name);
	if (it == compile.classMap.end())
		return nullptr;
	return compile.classes[it->second];
}

}

#endif