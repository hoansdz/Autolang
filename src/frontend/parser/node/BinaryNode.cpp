#ifndef BINARY_NODE_CPP
#define BINARY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <iostream>

namespace AutoLang {

#define optimizeNode(token, func)                                              \
	case Lexer::TokenType::token:                                              \
		return func(l, r);

ExprNode *BinaryNode::leftOpRight(in_func, ConstValueNode *l,
                                  ConstValueNode *r) {
	switch (op) {
		using namespace AutoLang;
		case Lexer::TokenType::PLUS:
			return plus(in_data, l, r);
		case Lexer::TokenType::MINUS:
			return minus(in_data, l, r);
		case Lexer::TokenType::STAR:
			return mul(in_data, l, r);
		case Lexer::TokenType::SLASH:
			return divide(in_data, l, r);
		case Lexer::TokenType::PERCENT:
			return mod(in_data, l, r);

		case Lexer::TokenType::AND:
			return bitwise_and(in_data, l, r);
		case Lexer::TokenType::OR:
			return bitwise_or(in_data, l, r);

		case Lexer::TokenType::EQEQ:
			return op_eqeq(in_data, l, r);
		case Lexer::TokenType::NOTEQ:
			return op_not_eq(in_data, l, r);

		case Lexer::TokenType::LTE:
			return op_less_than_eq(in_data, l, r);
		case Lexer::TokenType::GTE:
			return op_greater_than_eq(in_data, l, r);
		case Lexer::TokenType::LT:
			return op_less_than(in_data, l, r);
		case Lexer::TokenType::GT:
			return op_greater_than(in_data, l, r);
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::EQEQEQ: {
			const bool result = op == Lexer::TokenType::EQEQEQ;
			switch (l->classId) {
				case AutoLang::DefaultClass::boolClassId: {
					if (r->classId == AutoLang::DefaultClass::boolClassId) {
						const bool equal = (l->obj->b == r->obj->b);
						return context.constValuePool.push(
						    line, result ? equal : !equal);
					} else if (r->classId ==
					           AutoLang::DefaultClass::nullClassId) {
						return context.constValuePool.push(line, !result);
					}
				}
				case AutoLang::DefaultClass::nullClassId: {
					if (r->classId == AutoLang::DefaultClass::nullClassId) {
						return context.constValuePool.push(line, result);
					} else if (r->classId ==
					           AutoLang::DefaultClass::boolClassId) {
						return context.constValuePool.push(line, !result);
					}
					break;
				}
			}
			throw ParserError(line, "What happen");
		}
		case Lexer::TokenType::AND_AND:
			return context.constValuePool.push(line, l->obj->b && r->obj->b);
		case Lexer::TokenType::OR_OR:
			return context.constValuePool.push(line, l->obj->b || r->obj->b);
		default:
			throw ParserError(
			    line, "Cannot use operator '" +
			              Lexer::Token(0, op).toString(context) + "' between " +
			              compile.classes[l->classId]->name + " and " +
			              compile.classes[r->classId]->name);
	}
}

ExprNode *BinaryNode::resolve(in_func) {
	left = static_cast<HasClassIdNode *>(left->resolve(in_data));
	right = static_cast<HasClassIdNode *>(right->resolve(in_data));
	switch (op) {
		case Lexer::TokenType::IN: {
			switch (right->kind) {
				case NodeType::RANGE: {
					switch (left->kind) {
						case NodeType::CLASS_ACCESS: {
							throwError("Expected value but class name found");
							break;
						}
						default: {
							if (left->classId != DefaultClass::intClassId) {
								throwError(
								    "Type mismatch: expected 'Int' but '" +
								    compile.classes[left->classId]->name +
								    "' found");
								break;
							}
							break;
						}
					}
					break;
				}
			}
			break;
		}
		case Lexer::TokenType::SAFE_CAST:
		case Lexer::TokenType::UNSAFE_CAST: {
			if (left->kind == CLASS_ACCESS) {
				throwError("Left operand of 'as' must be a value");
			}
			if (right->kind != CLASS_ACCESS) {
				throwError("Right operand of 'as' must be a class name");
			}
			auto result = context.runtimeCastPool.push(
			    left, right->classId, op == Lexer::TokenType::SAFE_CAST);
			left = nullptr;
			ExprNode::deleteNode(this);
			return result->resolve(in_data);
		}
		default:
			break;
	}
	ConstValueNode *l;
	switch (left->kind) {
		case NodeType::CONST:
			l = static_cast<ConstValueNode *>(left);
			break;
		default:
			return this;
	}
	ConstValueNode *r;
	switch (right->kind) {
		case NodeType::CONST:
			r = static_cast<ConstValueNode *>(right);
			break;
		default:
			return this;
	}
	try {
		auto value = leftOpRight(in_data, l, r);
		left = nullptr;
		right = nullptr;
		ExprNode::deleteNode(l);
		ExprNode::deleteNode(r);
		ExprNode::deleteNode(this);
		return value;
	} catch (const std::runtime_error &err) {
		// throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
		//            " operator with " + compile.classes[l->classId]->name +
		//            " and " + compile.classes[r->classId]->name);
		throw ParserError(line, err.what());
	}
}

void BinaryNode::optimize(in_func) {
	left->optimize(in_data);
	right->optimize(in_data);
	if (left->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(left)->isLoadPrimary = true;
	if (right->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(right)->isLoadPrimary = true;
	switch (op) {
		case Lexer::TokenType::IS: {
			if (left->kind == CLASS_ACCESS) {
				throwError("Left operand of 'is' must be a value");
			}
			if (right->kind != CLASS_ACCESS) {
				throwError("Right operand of 'is' must be a class name");
			}
			classId = DefaultClass::boolClassId;
			return;
		}
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS:
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::SLASH: {
			if (left->classId == AutoLang::DefaultClass::boolClassId) {
				left = context.castPool.push(
				    left, AutoLang::DefaultClass::intClassId);
			}
			// std::cout<<compile.classes[left->classId]->name<<'\n';

			if (right->classId == AutoLang::DefaultClass::boolClassId) {
				right = context.castPool.push(
				    right, AutoLang::DefaultClass::intClassId);
			}
			if (left->isNullable() || right->isNullable())
				throwError("Cannot use operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "' with nullable value");
			break;
		}
		case Lexer::TokenType::EQEQ: {
			if (left->classId == AutoLang::DefaultClass::nullClassId ||
			    right->classId == AutoLang::DefaultClass::nullClassId) {
				op = Lexer::TokenType::EQEQEQ;
				classId = AutoLang::DefaultClass::boolClassId;
				return;
			}
			break;
		}
		case Lexer::TokenType::NOTEQ: {
			if (left->classId == AutoLang::DefaultClass::nullClassId ||
			    right->classId == AutoLang::DefaultClass::nullClassId) {
				op = Lexer::TokenType::NOTEQEQ;
				classId = AutoLang::DefaultClass::boolClassId;
				return;
			}
			break;
		}
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::EQEQEQ: {
			return;
		}
		default:
			if (left->isNullable() || right->isNullable())
				throwError("Cannot use operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "' with nullable value");
			break;
	}
	if (context.getTypeResult(left->classId, right->classId,
	                          static_cast<uint8_t>(op), classId))
		return;
	throwError(std::string("Cannot use '") +
	           Lexer::Token(0, op).toString(context) + "' between " +
	           compile.classes[left->classId]->name + " and " +
	           compile.classes[right->classId]->name);
}

void BinaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (op == Lexer::TokenType::IS) {
		left->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::IS);
		put_opcode_u32(bytecodes, right->classId);
		return;
	}
	if ((left->classId == DefaultClass::nullClassId ||
	     right->classId == DefaultClass::nullClassId)) {
		if (left->classId != DefaultClass::nullClassId) {
			left->putBytecodes(in_data, bytecodes);
		} else {
			right->putBytecodes(in_data, bytecodes);
		}
		switch (op) {
			case Lexer::TokenType::EQEQEQ:
				bytecodes.emplace_back(AutoLang::Opcode::IS_NULL);
				return;
			case Lexer::TokenType::NOTEQEQ:
				bytecodes.emplace_back(AutoLang::Opcode::IS_NON_NULL);
				return;
			default:
				throwError("Wrong, this can't happen");
		}
		return;
	}
	left->putBytecodes(in_data, bytecodes);
	right->putBytecodes(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::PLUS: {
			switch (left->classId) {
				case DefaultClass::intClassId: {
					switch (right->classId) {
						case DefaultClass::intClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::I_PLUS_I);
							return;
						}
						case DefaultClass::floatClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::I_PLUS_F);
							return;
						}
					}
					break;
				}
				case DefaultClass::floatClassId: {
					switch (right->classId) {
						case DefaultClass::intClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::F_PLUS_I);
							return;
						}
						case DefaultClass::floatClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::F_PLUS_F);
							return;
						}
					}
					break;
				}
			}
			bytecodes.emplace_back(AutoLang::Opcode::PLUS);
			return;
		}
		case Lexer::TokenType::MINUS: {
			switch (left->classId) {
				case DefaultClass::intClassId: {
					switch (right->classId) {
						case DefaultClass::intClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::I_MINUS_I);
							return;
						}
						case DefaultClass::floatClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::I_MINUS_F);
							return;
						}
					}
					break;
				}
				case DefaultClass::floatClassId: {
					switch (right->classId) {
						case DefaultClass::intClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::F_MINUS_I);
							return;
						}
						case DefaultClass::floatClassId: {
							bytecodes.emplace_back(AutoLang::Opcode::F_MINUS_F);
							return;
						}
					}
					break;
				}
			}
			bytecodes.emplace_back(AutoLang::Opcode::MINUS);
			return;
		}
		case Lexer::TokenType::STAR:
			bytecodes.emplace_back(AutoLang::Opcode::MUL);
			return;
		case Lexer::TokenType::SLASH:
			bytecodes.emplace_back(AutoLang::Opcode::DIVIDE);
			return;
		case Lexer::TokenType::PERCENT:
			bytecodes.emplace_back(AutoLang::Opcode::MOD);
			return;
		case Lexer::TokenType::AND:
			bytecodes.emplace_back(AutoLang::Opcode::BITWISE_AND);
			return;
		case Lexer::TokenType::OR:
			bytecodes.emplace_back(AutoLang::Opcode::BITWISE_OR);
			return;
		case Lexer::TokenType::AND_AND:
			bytecodes.emplace_back(AutoLang::Opcode::AND_AND);
			return;
		case Lexer::TokenType::OR_OR:
			bytecodes.emplace_back(AutoLang::Opcode::OR_OR);
			return;
		case Lexer::TokenType::NOTEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_POINTER);
			return;
		case Lexer::TokenType::EQEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::EQUAL_POINTER);
			return;
		case Lexer::TokenType::NOTEQ:
			bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_VALUE);
			return;
		case Lexer::TokenType::EQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::EQUAL_VALUE);
			return;
		case Lexer::TokenType::LT:
			bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN);
			return;
		case Lexer::TokenType::GT:
			bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN);
			return;
		case Lexer::TokenType::GTE:
			bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN_EQ);
			return;
		case Lexer::TokenType::LTE:
			bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN_EQ);
			return;
		default:
			// std::cout<<this<<'\n';
			throwError(std::string("Cannot find operator '") +
			           Lexer::Token(0, op).toString(context) + "'");
	}
}

ExprNode *BinaryNode::copy(in_func) {
	return context.binaryNodePool.push(
	    line, op, static_cast<HasClassIdNode *>(left->copy(in_data)),
	    static_cast<HasClassIdNode *>(right->copy(in_data)));
}

} // namespace AutoLang

#endif