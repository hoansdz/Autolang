#ifndef UNARY_NODE_CPP
#define UNARY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

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
		case NodeType::CONST_VAL: {
			auto value = static_cast<ConstValueNode *>(this->value);
			switch (op) {
				using namespace Autolang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case Autolang::DefaultClass::intClassId: {
							return this;
						}
						case Autolang::DefaultClass::floatClassId: {
							return context.constValuePool.push(
							    value->line, static_cast<int64_t>(value->f));
						}
						case Autolang::DefaultClass::boolClassId: {
							return context.constValuePool.push(
							    value->line,
							    static_cast<int64_t>(value->obj->b));
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::MINUS: {
					switch (value->classId) {
						case Autolang::DefaultClass::intClassId: {
							return context.constValuePool.push(value->line,
							                                   -value->i);
						}
						case Autolang::DefaultClass::floatClassId: {
							return context.constValuePool.push(value->line,
							                                   -value->f);
						}
						case Autolang::DefaultClass::boolClassId: {
							return context.constValuePool.push(
							    value->line,
							    -static_cast<int64_t>(value->obj->b));
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == Autolang::DefaultClass::boolClassId) {
						return context.constValuePool.push(value->line,
						                                   !value->obj->b);
					}
					// if (value->classId ==
					// Autolang::DefaultClass::nullClassId)
					// {
					// 	value->classId = Autolang::DefaultClass::boolClassId;
					// 	value->obj = ObjectManager::create(true);
					// 	value->id = context.getBoolConstValuePosition(true);
					// 	return value;
					// }
					break;
				}
				default:
					break;
			}
			throwError("Cannot find unary operator '" +
			           Lexer::Token(0, op).toString(context) + "' for type '" +
			           compile.classes[value->classId]->getName(compile) +
			           "'\nHint: Unary operator '" + Lexer::Token(0, op).toString(context) +
			           "' is only supported on compatible numeric or boolean constant types.");
		}
		case NodeType::CAST: {
			switch (op) {
				using namespace Autolang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case Autolang::DefaultClass::intClassId:
						case Autolang::DefaultClass::floatClassId: {
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case Autolang::DefaultClass::boolClassId: {
							value->classId = Autolang::DefaultClass::intClassId;
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
						case Autolang::DefaultClass::intClassId:
						case Autolang::DefaultClass::floatClassId:
						case Autolang::DefaultClass::boolClassId: {
							return this;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == Autolang::DefaultClass::boolClassId) {
						return this;
					}
					break;
				}
				default:
					break;
			}
			throwError("Cannot find unary operator '" +
			           Lexer::Token(0, op).toString(context) + "' for cast type '" +
			           compile.classes[value->classId]->getName(compile) +
			           "'\nHint: Ensure the expression can be evaluated with unary operator '" +
			           Lexer::Token(0, op).toString(context) + "'.");
		}
		default: {
		}
	}
	return this;
}

void UnaryNode::optimize(in_func) {
	switch (value->kind) {
		case NodeType::CONST_VAL: {
			static_cast<ConstValueNode *>(value)->isLoadPrimary =
			    op != Lexer::TokenType::PLUS;
			break;
		}
		case NodeType::CLASS_ACCESS: {
			throwError("Expected value expression for operator '" +
			           Lexer::Token(0, op).toString(context) +
			           "'\nHint: A class reference cannot be used as an operand. Provide an instance or value expression.");
		}
		case NodeType::VAR:
		case NodeType::GET_PROP: {
			static_cast<AccessNode *>(value)->cloneable = false;
			break;
		}
		default: {
			break;
		}
	}
	value->optimize(in_data);
	if (value->isNullable()) {
		throwError("Operator '" + Lexer::Token(0, op).toString(context) +
		           "' cannot be applied to nullable operand of type '" +
		           compile.classes[value->classId]->getName(compile) + "?'\nHint: Unwrap the nullable value using '!' or perform a null check before applying unary operator '" +
		           Lexer::Token(0, op).toString(context) + "'.");
	}
	switch (op) {
		case Lexer::TokenType::PLUS: {
			switch (value->classId) {
				case DefaultClass::intClassId:
				case DefaultClass::boolClassId: {
					classId = DefaultClass::intClassId;
					return;
				}
				case DefaultClass::floatClassId: {
					classId = DefaultClass::floatClassId;
					return;
				}
				default:
					throwError(
					    "Cannot convert type '" +
					    compile.classes[value->classId]->getName(compile) +
					    "' to numeric type\nHint: Unary '+' operator requires operand of type Int, Float, or Bool.");
			}
		}
		case Lexer::TokenType::MINUS: {
			switch (value->classId) {
				case DefaultClass::intClassId:
				case DefaultClass::boolClassId: {
					classId = DefaultClass::intClassId;
					return;
				}
				case DefaultClass::floatClassId: {
					classId = DefaultClass::floatClassId;
					return;
				}
				default:
					throwError(
					    "Cannot convert type '" +
					    compile.classes[value->classId]->getName(compile) +
					    "' to numeric type\nHint: Unary '-' operator requires operand of type Int, Float, or Bool.");
			}
		}
		case Lexer::TokenType::NOT: {
			if (value->classId == DefaultClass::boolClassId) {
				classId = DefaultClass::boolClassId;
				return;
			}
			throwError("Cannot convert type '" +
			           compile.classes[value->classId]->getName(compile) +
			           "' to Bool\nHint: Logical NOT '!' operator requires a Bool operand.");
		}
		default: {
			classId = value->classId;
			return;
		}
	}
}

ExprNode *UnaryNode::copy(in_func) {
	return context.unaryNodePool.push(
	    line, op, static_cast<HasClassIdNode *>(value->copy(in_data)));
}

template <Opcode normal, Opcode local, Opcode global, Opcode local_member,
          Opcode global_member>
void UnaryNode::putOptimizedBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	switch (value->kind) {
		case NodeType::VAR: {
			auto node = static_cast<VarNode *>(value);
			if (node->declaration->isGlobal) {
				bytecodes.emplace_back(global);
				put_opcode_u32(bytecodes, node->declaration->id);
			} else {
				bytecodes.emplace_back(local);
				put_opcode_u32(bytecodes, node->declaration->id);
			}
			return;
		}
		case NodeType::GET_PROP: {
			auto node = static_cast<GetPropNode *>(value);
			if (node->declaration->isGlobal) {
				bytecodes.emplace_back(global);
				put_opcode_u32(bytecodes, node->declaration->id);
			} else {
				if (node->caller->kind != NodeType::VAR) {
					value->putBytecodes(in_data, bytecodes);
					bytecodes.emplace_back(normal);
					return;
				}
				auto caller = static_cast<VarNode *>(node->caller);
				if (caller->declaration->isGlobal) {
					bytecodes.emplace_back(global_member);
					put_opcode_u32(bytecodes, caller->declaration->id);
					put_opcode_u32(bytecodes, node->declaration->id);
				} else {
					bytecodes.emplace_back(local_member);
					put_opcode_u32(bytecodes, caller->declaration->id);
					put_opcode_u32(bytecodes, node->declaration->id);
				}
			}
			return;
		}
		default: {
			value->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(normal);
			return;
		}
	}
}

void UnaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::PLUS: {
			value->putBytecodes(in_data, bytecodes);
			if (value->kind == NodeType::CONST_VAL) {
				return;
			}
			switch (value->classId) {
				case Autolang::DefaultClass::intClassId: {
					bytecodes.emplace_back(Opcode::TO_INT);
					return;
				}
				case Autolang::DefaultClass::floatClassId: {
					bytecodes.emplace_back(Opcode::TO_FLOAT);
					return;
				}
				case Autolang::DefaultClass::boolClassId: {
					bytecodes.emplace_back(Opcode::TO_INT);
					return;
				}
				default:
					break;
			}
			break;
		}
		case Lexer::TokenType::MINUS: {
			putOptimizedBytecodes<Opcode::NEGATIVE, Opcode::NEGATIVE_LOCAL,
			                      Opcode::NEGATIVE_GLOBAL,
			                      Opcode::NEGATIVE_LOCAL_MEMBER,
			                      Opcode::NEGATIVE_GLOBAL_MEMBER>(in_data,
			                                                      bytecodes);
			break;
		}
		case Lexer::TokenType::NOT: {
			putOptimizedBytecodes<Opcode::NOT, Opcode::NOT_LOCAL,
			                      Opcode::NOT_GLOBAL, Opcode::NOT_LOCAL_MEMBER,
			                      Opcode::NOT_GLOBAL_MEMBER>(in_data,
			                                                 bytecodes);
			break;
		}
		default: {
			break;
		}
	}
}

UnaryNode::~UnaryNode() { deleteNode(value); }

} // namespace Autolang

#endif