#ifndef UNARY_NODE_CPP
#define UNARY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *UnaryNode::resolve(in_func) {
	// if (value->kind == NodeType::UNARY)
	// {
	// 	auto node = static_cast<UnaryNode *>(value);
	// 	if (node->op != op) {
	// 		return nullptr;
	// 	}
	// 	value = node->value;
	// 	node->value = nullptr;
	// 	ExprNode::deleteNode(node);
	// }
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	switch (value->kind) {
		case NodeType::CONST: {
			auto value = static_cast<ConstValueNode *>(this->value);
			switch (op) {
				using namespace AutoLang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId: {
							// auto result = value;
							// value = nullptr;
							// ExprNode::deleteNode(this);
							// return result;
							return this;
						}
						case AutoLang::DefaultClass::boolClassId: {
							// value->classId =
							// AutoLang::DefaultClass::intClassId; value->i =
							// static_cast<int64_t>(value->obj->b); auto result
							// = value; value = nullptr;
							// ExprNode::deleteNode(this);
							// return result;
							return this;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::MINUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId: {
							value->i = -value->i;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::floatClassId: {
							value->f = -value->f;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(-value->obj->b);
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == AutoLang::DefaultClass::boolClassId) {
						value->obj = ObjectManager::create(!value->obj->b);
						value->id =
						    context.getBoolConstValuePosition(value->obj->b);
						auto result = value;
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					// if (value->classId ==
					// AutoLang::DefaultClass::nullClassId)
					// {
					// 	value->classId = AutoLang::DefaultClass::boolClassId;
					// 	value->obj = ObjectManager::create(true);
					// 	value->id = context.getBoolConstValuePosition(true);
					// 	return value;
					// }
					break;
				}
				default:
					break;
			}
			throwError("Cannot find operator '" +
			           Lexer::Token(0, op).toString(context) + "' with class " +
			           compile.classes[value->classId]->name);
		}
		case NodeType::CAST: {
			switch (op) {
				using namespace AutoLang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId: {
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::MINUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId:
						case AutoLang::DefaultClass::boolClassId: {
							return this;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == AutoLang::DefaultClass::boolClassId) {
						return this;
					}
					break;
				}
				default:
					break;
			}
			throwError("Cannot find operator '" +
			           Lexer::Token(0, op).toString(context) + "' with class " +
			           compile.classes[value->classId]->name);
		}
		default: {
		}
	}
	return this;
}

void UnaryNode::optimize(in_func) {
	if (value->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(value)->isLoadPrimary = true;
	if (value->kind == NodeType::CLASS_ACCESS) {
		throwError("Expected value if use operator '" +
		           Lexer::Token(0, op).toString(context) + "'");
	}
	value->optimize(in_data);
	switch (op) {
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS: {
			switch (value->classId) {
				case DefaultClass::intClassId:
				case DefaultClass::floatClassId:
				case DefaultClass::boolClassId: {
					break;
				}
				default:
					throwError("Cannot cast class " +
					           compile.classes[value->classId]->name +
					           " to number");
			}
		}
		case Lexer::TokenType::NOT: {
			if (value->classId == DefaultClass::boolClassId)
				break;
			throwError("Cannot cast class " +
			           compile.classes[value->classId]->name + " to Bool");
		}
		default:
			break;
	}
	classId = value->classId;
}

ExprNode *UnaryNode::copy(in_func) {
	return context.unaryNodePool.push(
	    line, op, static_cast<HasClassIdNode *>(value->copy(in_data)));
}

void UnaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::PLUS: {
			switch (value->classId) {
				case AutoLang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::TO_INT);
					return;
				}
				case AutoLang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::TO_FLOAT);
					return;
				}
				default:
					break;
			}
			break;
		}
		case Lexer::TokenType::MINUS: {
			bytecodes.emplace_back(Opcode::NEGATIVE);
			break;
		}
		case Lexer::TokenType::NOT: {
			bytecodes.emplace_back(Opcode::NOT);
			break;
		}
		default:
			break;
	}
}

UnaryNode::~UnaryNode() { deleteNode(value); }

} // namespace AutoLang

#endif