#ifndef BINARY_NODE_CPP
#define BINARY_NODE_CPP

#include "Node.hpp"
#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/Node.hpp"
#include "shared/ClassFlags.hpp"
#include <iostream>

namespace Autolang {

#define optimizeNode(token, func)                                              \
	case Lexer::TokenType::token:                                              \
		return func(l, r);

ExprNode *BinaryNode::leftOpRight(in_func, ConstValueNode *l,
                                  ConstValueNode *r) {
	switch (op) {
		using namespace Autolang;
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
				case Autolang::DefaultClass::boolClassId: {
					if (r->classId == Autolang::DefaultClass::boolClassId) {
						const bool equal = (l->obj->b == r->obj->b);
						return context.constValuePool.push(
						    line, result ? equal : !equal);
					} else if (r->classId ==
					           Autolang::DefaultClass::nullClassId) {
						return context.constValuePool.push(line, !result);
					}
				}
				case Autolang::DefaultClass::nullClassId: {
					if (r->classId == Autolang::DefaultClass::nullClassId) {
						return context.constValuePool.push(line, result);
					} else if (r->classId ==
					           Autolang::DefaultClass::boolClassId) {
						return context.constValuePool.push(line, !result);
					}
					break;
				}
			}
			throw ParserError(
			    line, "Cannot use operator '" +
			              Lexer::Token(0, op).toString(context) + "' between " +
			              compile.classes[l->classId]->getName(compile) +
			              " and " +
			              compile.classes[r->classId]->getName(compile) +
			              "\nHint: Both operands must have compatible types for constant folding.");
		}
		case Lexer::TokenType::AND_AND:
			return context.constValuePool.push(line, l->obj->b && r->obj->b);
		case Lexer::TokenType::OR_OR:
			return context.constValuePool.push(line, l->obj->b || r->obj->b);
		default:
			throw ParserError(
			    line, "Cannot use operator '" +
			              Lexer::Token(0, op).toString(context) + "' between " +
			              compile.classes[l->classId]->getName(compile) +
			              " and " +
			              compile.classes[r->classId]->getName(compile) +
			              "\nHint: Both operands must have compatible types for constant folding.");
	}
}

ExprNode *BinaryNode::resolve(in_func) {
	left = static_cast<HasClassIdNode *>(left->resolve(in_data));
	right = static_cast<HasClassIdNode *>(right->resolve(in_data));
	switch (op) {
		case Lexer::TokenType::IN_: {
			switch (right->kind) {
				case NodeType::RANGE: {
					if (left->kind == NodeType::CLASS_ACCESS) {
						throwError("Expected value but class name found\nHint: The left operand of 'in' must be a value or variable, not a class type.");
					}
					break;
				}
				case NodeType::CLASS_ACCESS: {
					throwError("Expected value but class name found\nHint: The right operand of 'in' with Range must be a value or range expression, not a class type.");
				}
				default: {
					auto *result = context.callNodePool.push(
					    line, tokenIndex, contextCallClassId, right,
					    lexerIdcontains, std::vector<HasClassIdNode *>{left},
					    context.justFindStatic, true, false);
					left = nullptr;
					right = nullptr;
					return result;
				}
			}
			classId = DefaultClass::boolClassId;
			break;
		}
		case Lexer::TokenType::SAFE_CAST:
		case Lexer::TokenType::UNSAFE_CAST: {
			if (left->kind == CLASS_ACCESS) {
				throwError("Left operand of 'as' must be a value\nHint: Provide an instance or expression on the left side of 'as' (e.g. value as Type).");
			}
			if (right->kind != CLASS_ACCESS) {
				throwError("Right operand of 'as' must be a class name\nHint: Provide a valid class name on the right side of 'as' (e.g. value as Type).");
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
		case NodeType::CONST_VAL:
			l = static_cast<ConstValueNode *>(left);
			break;
		default:
			return this;
	}
	ConstValueNode *r;
	switch (right->kind) {
		case NodeType::CONST_VAL:
			r = static_cast<ConstValueNode *>(right);
			break;
		default:
			return this;
	}
	try {
		auto value = leftOpRight(in_data, l, r);
		left = nullptr;
		right = nullptr;
		// ExprNode::deleteNode(l);
		// ExprNode::deleteNode(r);
		// ExprNode::deleteNode(this);
		return value;
	} catch (const std::runtime_error &err) {
		// throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
		//            " operator with " +
		//            compile.classes[l->classId]->getName(compile) + " and " +
		//            compile.classes[r->classId]->getName(compile));
		throw ParserError(line, err.what());
	}
}

void BinaryNode::optimize(in_func) {
	left->optimize(in_data);
	right->optimize(in_data);
	switch (left->kind) {
		case NodeType::CONST_VAL:
			static_cast<ConstValueNode *>(left)->isLoadPrimary = true;
			break;
		case NodeType::VAR:
		case NodeType::GET_PROP:
			static_cast<AccessNode *>(left)->cloneable = false;
			break;
		default:
			break;
	}
	switch (right->kind) {
		case NodeType::CONST_VAL:
			static_cast<ConstValueNode *>(right)->isLoadPrimary = true;
			break;
		case NodeType::VAR:
		case NodeType::GET_PROP:
			static_cast<AccessNode *>(right)->cloneable = false;
			break;
		default:
			break;
	}
	switch (op) {
		case Lexer::TokenType::IS: {
			if (left->kind == CLASS_ACCESS) {
				throwError("Left operand of 'is' must be a value\nHint: Provide an instance or variable on the left side of 'is' (e.g. obj is ClassName).");
			}
			if (right->kind != CLASS_ACCESS) {
				throwError("Right operand of 'is' must be a class name\nHint: Provide a valid class name on the right side of 'is' (e.g. obj is ClassName).");
			}
			classId = DefaultClass::boolClassId;
			return;
		}
		case Lexer::TokenType::IN_: {
			if (left->isNullable() || right->isNullable()) {
				throwError(
				    "Cannot use operator '" +
				    Lexer::Token(0, op).toString(context) +
				    "' with nullable value: " + left->getClassName(in_data) +
				    " " + Lexer::Token(0, op).toString(context) + " " +
				    right->getClassName(in_data) +
				    "\nHint: Ensure both operands are non-nullable using '!!' or check for null before using 'in'.");
			}
			if (right->kind == NodeType::RANGE &&
			    left->classId != DefaultClass::intClassId) {
				throwError("Type mismatch: expected 'Int' but '" +
				           left->getClassName(in_data) + "' found\nHint: Range membership checks require an integer index value on the left side.");
			}

			classId = DefaultClass::boolClassId;
			return;
		}
		case Lexer::TokenType::PLUS: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Operator '+' requires value operands, not class names.");
			}

			switch (left->classId) {
				case Autolang::DefaultClass::boolClassId: {
					left = context.castPool.push(
					    left, Autolang::DefaultClass::intClassId);
					break;
				}
				case Autolang::DefaultClass::stringClassId: {
					switch (right->classId) {
						case DefaultClass::intClassId:
						case DefaultClass::floatClassId:
						case DefaultClass::boolClassId:
						case DefaultClass::stringClassId: {
							break;
						}
						default: {
							auto classInfo = context.classInfo[right->classId];
							auto it =
							    classInfo->allFunction.find(lexerIdtoString);
							if (it == classInfo->allFunction.end()) {
								break;
							}
							auto &vec = it->second;
							for (auto funcId : vec) {
								auto func = compile.functions[funcId];
								if (func->argSize !=
								        (!(func->functionFlags &
								           FunctionFlags::FUNC_IS_STATIC)) ||
								    func->returnId !=
								        DefaultClass::stringClassId ||
								    !(func->functionFlags &
								      FunctionFlags::FUNC_PUBLIC))
									continue;
								auto callNode = context.callNodePool.push(
								    right->line, tokenIndex, std::nullopt,
								    right, lexerIdtoString,
								    std::vector<HasClassIdNode *>{}, false,
								    right->isNullable(), false);
								right = callNode;
								callNode->funcId = funcId;
								callNode->classId = DefaultClass::stringClassId;
								break;
							}
							break;
						}
					}
					break;
				}
			}
			// std::cerr<<compile.classes[left->classId]->getName(compile)<<'\n';

			switch (right->classId) {
				case Autolang::DefaultClass::boolClassId: {
					right = context.castPool.push(
					    right, Autolang::DefaultClass::intClassId);
					break;
				}
				case Autolang::DefaultClass::stringClassId: {
					switch (left->classId) {
						case DefaultClass::intClassId:
						case DefaultClass::floatClassId:
						case DefaultClass::boolClassId:
						case DefaultClass::stringClassId: {
							break;
						}
						default: {
							auto classInfo = context.classInfo[left->classId];
							auto it =
							    classInfo->allFunction.find(lexerIdtoString);
							if (it == classInfo->allFunction.end()) {
								break;
							}
							auto &vec = it->second;
							for (auto funcId : vec) {
								auto func = compile.functions[funcId];
								if (func->argSize !=
								        (!(func->functionFlags &
								           FunctionFlags::FUNC_IS_STATIC)) ||
								    func->returnId !=
								        DefaultClass::stringClassId ||
								    !(func->functionFlags &
								      FunctionFlags::FUNC_PUBLIC))
									continue;
								auto callNode = context.callNodePool.push(
								    left->line, tokenIndex, std::nullopt, left,
								    lexerIdtoString,
								    std::vector<HasClassIdNode *>{}, false,
								    left->isNullable(), false);
								left = callNode;
								callNode->funcId = funcId;
								callNode->classId = DefaultClass::stringClassId;
								break;
							}
							break;
						}
					}
					break;
				}
			}

			if (left->isNullable() || right->isNullable()) {
				throwError(
				    "Cannot use operator '" +
				    Lexer::Token(0, op).toString(context) +
				    "' with nullable value: " + left->getClassName(in_data) +
				    " " + Lexer::Token(0, op).toString(context) + " " +
				    right->getClassName(in_data) +
				    "\nHint: Perform a null check or unwrap nullable value with '!!' before adding.");
			}
			break;
		}
		case Lexer::TokenType::MINUS:
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::SLASH: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Arithmetic operators require value operands, not class names.");
			}
			if (left->classId == Autolang::DefaultClass::boolClassId) {
				left = context.castPool.push(
				    left, Autolang::DefaultClass::intClassId);
				break;
			}
			// std::cerr<<compile.classes[left->classId]->getName(compile)<<'\n';

			if (right->classId == Autolang::DefaultClass::boolClassId) {
				right = context.castPool.push(
				    right, Autolang::DefaultClass::intClassId);
			}
			if (left->isNullable() || right->isNullable()) {
				throwError(
				    "Cannot use operator '" +
				    Lexer::Token(0, op).toString(context) +
				    "' with nullable value: " + left->getClassName(in_data) +
				    " " + Lexer::Token(0, op).toString(context) + " " +
				    right->getClassName(in_data) +
				    "\nHint: Perform a null check or unwrap nullable value with '!!' before arithmetic operations.");
			}
			break;
		}
		case Lexer::TokenType::EQEQ: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Equality operator '==' requires value operands, not class names.");
			}
			classId = Autolang::DefaultClass::boolClassId;
			if (left->classId == Autolang::DefaultClass::nullClassId ||
			    right->classId == Autolang::DefaultClass::nullClassId) {
				op = Lexer::TokenType::EQEQEQ;
				return;
			}
			if (left->classId == right->classId &&
			    compile.classes[left->classId]->classFlags &
			        ClassFlags::CLASS_IS_ENUM) {
				op = Lexer::TokenType::EQEQEQ;
				return;
			}
			break;
		}
		case Lexer::TokenType::NOTEQ: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Inequality operator '!=' requires value operands, not class names.");
			}
			classId = Autolang::DefaultClass::boolClassId;
			if (left->classId == Autolang::DefaultClass::nullClassId ||
			    right->classId == Autolang::DefaultClass::nullClassId) {
				op = Lexer::TokenType::NOTEQEQ;
				return;
			}
			if (left->classId == right->classId &&
			    compile.classes[left->classId]->classFlags &
			        ClassFlags::CLASS_IS_ENUM) {
				op = Lexer::TokenType::NOTEQEQ;
				return;
			}
			break;
		}
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::EQEQEQ: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Identity comparison operator requires value operands, not class names.");
			}
			classId = DefaultClass::boolClassId;
			return;
		}
		default: {
			if (left->kind == NodeType::CLASS_ACCESS ||
			    right->kind == NodeType::CLASS_ACCESS) {
				throwError("Expected value operand when using operator '" +
				           Lexer::Token(0, op).toString(context) +
				           "'\nHint: Binary operators require value operands, not class names.");
			}
			if (left->isNullable() || right->isNullable())
				throwError(
				    "Cannot use operator '" +
				    Lexer::Token(0, op).toString(context) +
				    "' with nullable value: " + left->getClassName(in_data) +
				    " " + Lexer::Token(0, op).toString(context) + " " +
				    right->getClassName(in_data) +
				    "\nHint: Ensure operands are non-null before performing binary operations.");
			break;
		}
	}
	if (context.getTypeResult(left->classId, right->classId,
	                          static_cast<uint8_t>(op), classId))
		return;
	throwError(std::string("Cannot use '") +
	           Lexer::Token(0, op).toString(context) + "' between " +
	           compile.classes[left->classId]->getName(compile) + " and " +
	           compile.classes[right->classId]->getName(compile) +
	           "\nHint: Ensure both operand types support operator '" +
	           Lexer::Token(0, op).toString(context) + "' or provide a valid type conversion.");
}

bool BinaryNode::putOptimizedBytecode(in_func, std::vector<uint8_t> &bytecodes,
                                      Lexer::TokenType op, HasClassIdNode *left,
                                      HasClassIdNode *right) {
	if (op == Lexer::TokenType::IS) {
		left->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::IS);
		put_opcode_u32(bytecodes, right->classId);
		return true;
	}
	auto it = context.operatorTable.find(op);
	if (it == context.operatorTable.end())
		return false;
	OperatorId operatorId = it->second;
	switch (left->kind) {
		case NodeType::VAR: {
			auto leftNode = static_cast<VarNode *>(left);
			if (leftNode->isForceNonNull) {
				return false;
			}
			switch (right->kind) {
				case NodeType::VAR: {
					auto rightNode = static_cast<VarNode *>(right);
					if (rightNode->isForceNonNull) {
						return false;
					}
					if (leftNode->declaration->isGlobal) {
						bytecodes.emplace_back(rightNode->declaration->isGlobal
						                           ? Opcode::GLOBAL_CAL_GLOBAL
						                           : Opcode::GLOBAL_CAL_LOCAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					bytecodes.emplace_back(rightNode->declaration->isGlobal
					                           ? Opcode::LOCAL_CAL_GLOBAL
					                           : Opcode::LOCAL_CAL_LOCAL);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				case NodeType::CONST_VAL: {
					auto rightNode = static_cast<ConstValueNode *>(right);
					bytecodes.emplace_back(leftNode->declaration->isGlobal
					                           ? Opcode::GLOBAL_CAL_CONST
					                           : Opcode::LOCAL_CAL_CONST);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, rightNode->id);
					return true;
				}
				case NodeType::GET_PROP: {
					auto rightNode = static_cast<GetPropNode *>(right);
					if (rightNode->isForceNonNull ||
					    rightNode->caller->kind != NodeType::VAR ||
					    rightNode->accessNullable ||
					    rightNode->declaration->isLateInit ||
					    static_cast<VarNode *>(rightNode->caller)
					        ->isForceNonNull) {
						return false;
					}
					if (rightNode->isStatic) {
						rightNode->caller->putBytecodesIfMustBeCalled(
						    in_data, bytecodes);
						bytecodes.emplace_back(leftNode->declaration->isGlobal
						                           ? Opcode::GLOBAL_CAL_GLOBAL
						                           : Opcode::LOCAL_CAL_GLOBAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					auto caller = static_cast<VarNode *>(rightNode->caller);
					if (leftNode->declaration->isGlobal) {
						bytecodes.emplace_back(
						    caller->declaration->isGlobal
						        ? Opcode::GLOBAL_CAL_GLOBAL_MEMBER
						        : Opcode::GLOBAL_CAL_LOCAL_MEMBER);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, caller->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					bytecodes.emplace_back(
					    caller->declaration->isGlobal
					        ? Opcode::LOCAL_CAL_GLOBAL_MEMBER
					        : Opcode::LOCAL_CAL_LOCAL_MEMBER);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, caller->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				default:
					break;
			}
			break;
		}
		case NodeType::CONST_VAL: {
			auto leftNode = static_cast<ConstValueNode *>(left);
			switch (right->kind) {
				case NodeType::VAR: {
					auto rightNode = static_cast<VarNode *>(right);
					if (rightNode->isForceNonNull) {
						return false;
					}
					bytecodes.emplace_back(rightNode->declaration->isGlobal
					                           ? Opcode::CONST_CAL_GLOBAL
					                           : Opcode::CONST_CAL_LOCAL);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftNode->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				case NodeType::GET_PROP: {
					auto rightNode = static_cast<GetPropNode *>(right);
					if (rightNode->isForceNonNull ||
					    rightNode->caller->kind != NodeType::VAR ||
					    rightNode->accessNullable ||
					    rightNode->declaration->isLateInit ||
					    static_cast<VarNode *>(rightNode->caller)
					        ->isForceNonNull) {
						return false;
					}
					if (rightNode->isStatic) {
						rightNode->caller->putBytecodesIfMustBeCalled(
						    in_data, bytecodes);
						bytecodes.emplace_back(CONST_CAL_GLOBAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					auto caller = static_cast<VarNode *>(rightNode->caller);
					bytecodes.emplace_back(
					    caller->declaration->isGlobal
					        ? Opcode::CONST_CAL_GLOBAL_MEMBER
					        : Opcode::CONST_CAL_LOCAL_MEMBER);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftNode->id);
					put_opcode_u32(bytecodes, caller->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				default:
					break;
			}
			break;
		}
		case NodeType::GET_PROP: {
			auto leftNode = static_cast<GetPropNode *>(left);
			if (leftNode->declaration->isLateInit || leftNode->accessNullable ||
			    leftNode->isForceNonNull ||
			    (leftNode->caller->isNullableNode() &&
			     static_cast<NullableNode *>(leftNode->caller)
			         ->isForceNonNull)) {
				return false;
			}
			if (leftNode->isStatic) {
				leftNode->caller->putBytecodesIfMustBeCalled(in_data,
				                                             bytecodes);
				switch (right->kind) {
					case NodeType::VAR: {
						auto rightNode = static_cast<VarNode *>(right);
						if (rightNode->isForceNonNull) {
							return false;
						}
						bytecodes.emplace_back(rightNode->declaration->isGlobal
						                           ? Opcode::GLOBAL_CAL_GLOBAL
						                           : Opcode::GLOBAL_CAL_LOCAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					case NodeType::CONST_VAL: {
						auto rightNode = static_cast<ConstValueNode *>(right);
						bytecodes.emplace_back(Opcode::GLOBAL_CAL_CONST);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->id);
						return true;
					}
					case NodeType::GET_PROP: {
						auto rightNode = static_cast<GetPropNode *>(right);
						if (rightNode->isForceNonNull ||
						    rightNode->caller->kind != NodeType::VAR ||
						    rightNode->accessNullable ||
						    rightNode->declaration->isLateInit ||
						    static_cast<VarNode *>(rightNode->caller)
						        ->isForceNonNull) {
							return false;
						}
						if (rightNode->isStatic) {
							rightNode->caller->putBytecodesIfMustBeCalled(
							    in_data, bytecodes);
							bytecodes.emplace_back(Opcode::GLOBAL_CAL_GLOBAL);
							bytecodes.emplace_back(operatorId);
							put_opcode_u32(bytecodes,
							               leftNode->declaration->id);
							put_opcode_u32(bytecodes,
							               rightNode->declaration->id);
							return true;
						}
						auto caller = static_cast<VarNode *>(rightNode->caller);
						bytecodes.emplace_back(
						    caller->declaration->isGlobal
						        ? Opcode::GLOBAL_CAL_GLOBAL_MEMBER
						        : Opcode::GLOBAL_CAL_LOCAL_MEMBER);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, caller->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					default:
						break;
				}
				return false;
			}
			if (leftNode->caller->kind != NodeType::VAR) {
				return false;
			}
			auto leftCaller = static_cast<VarNode *>(leftNode->caller);
			switch (right->kind) {
				case NodeType::VAR: {
					auto rightNode = static_cast<VarNode *>(right);
					if (rightNode->isForceNonNull) {
						return false;
					}
					if (leftCaller->declaration->isGlobal) {
						bytecodes.emplace_back(
						    rightNode->declaration->isGlobal
						        ? Opcode::GLOBAL_MEMBER_CAL_GLOBAL
						        : Opcode::GLOBAL_MEMBER_CAL_LOCAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftCaller->declaration->id);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					bytecodes.emplace_back(
					    rightNode->declaration->isGlobal
					        ? Opcode::LOCAL_MEMBER_CAL_GLOBAL
					        : Opcode::LOCAL_MEMBER_CAL_LOCAL);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftCaller->declaration->id);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				case NodeType::CONST_VAL: {
					auto rightNode = static_cast<ConstValueNode *>(right);
					bytecodes.emplace_back(
					    leftCaller->declaration->isGlobal
					        ? Opcode::GLOBAL_MEMBER_CAL_CONST
					        : Opcode::LOCAL_MEMBER_CAL_CONST);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftCaller->declaration->id);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, rightNode->id);
					return true;
				}
				case NodeType::GET_PROP: {
					auto rightNode = static_cast<GetPropNode *>(right);
					if (rightNode->isForceNonNull ||
					    rightNode->caller->kind != NodeType::VAR ||
					    rightNode->declaration->isLateInit ||
					    static_cast<VarNode *>(rightNode->caller)
					        ->isForceNonNull) {
						return false;
					}
					if (rightNode->isStatic) {
						rightNode->caller->putBytecodesIfMustBeCalled(
						    in_data, bytecodes);
						bytecodes.emplace_back(
						    leftCaller->declaration->isGlobal
						        ? Opcode::GLOBAL_MEMBER_CAL_GLOBAL
						        : Opcode::LOCAL_MEMBER_CAL_GLOBAL);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftCaller->declaration->id);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					auto caller = static_cast<VarNode *>(rightNode->caller);
					if (leftCaller->declaration->isGlobal) {
						bytecodes.emplace_back(
						    caller->declaration->isGlobal
						        ? Opcode::GLOBAL_MEMBER_CAL_GLOBAL_MEMBER
						        : Opcode::GLOBAL_MEMBER_CAL_LOCAL_MEMBER);
						bytecodes.emplace_back(operatorId);
						put_opcode_u32(bytecodes, leftCaller->declaration->id);
						put_opcode_u32(bytecodes, leftNode->declaration->id);
						put_opcode_u32(bytecodes, caller->declaration->id);
						put_opcode_u32(bytecodes, rightNode->declaration->id);
						return true;
					}
					bytecodes.emplace_back(
					    caller->declaration->isGlobal
					        ? Opcode::LOCAL_MEMBER_CAL_GLOBAL_MEMBER
					        : Opcode::LOCAL_MEMBER_CAL_LOCAL_MEMBER);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, leftCaller->declaration->id);
					put_opcode_u32(bytecodes, leftNode->declaration->id);
					put_opcode_u32(bytecodes, caller->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					return true;
				}
				default:
					break;
			}
			return false;
		}
		default:
			break;
	}
	return false;
}

void BinaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	optimized = putOptimizedBytecode(in_data, bytecodes, op, left, right);
	if (optimized) {
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
				bytecodes.emplace_back(Autolang::Opcode::IS_NULL);
				return;
			case Lexer::TokenType::NOTEQEQ:
				bytecodes.emplace_back(Autolang::Opcode::IS_NON_NULL);
				return;
			default: {
				throwError("Cannot use operator '" +
				           Lexer::Token(0, op).toString(context) + "' with '" +
				           compile.classes[left->classId]->getName(compile) +
				           "' and '" +
				           compile.classes[right->classId]->getName(compile) +
				           "'\nHint: Cannot perform this operation directly with 'null'. Use identity comparison (=== or !==) or null-coalescing (?:).");
			}
		}
		return;
	}
	switch (op) {
		case Lexer::TokenType::AND_AND: {
			left->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::JUMP_IF_FALSE_NO_POP);
			BytecodePos jumpOffset =
			    bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			right->putBytecodes(in_data, bytecodes);
			rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
			                   jumpOffset,
			                   bytecodes.size() - context.currentBytecodePos);
			return;
		}
		case Lexer::TokenType::OR_OR: {
			left->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::JUMP_IF_TRUE_NO_POP);
			BytecodePos jumpOffset =
			    bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			right->putBytecodes(in_data, bytecodes);
			rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
			                   jumpOffset,
			                   bytecodes.size() - context.currentBytecodePos);
			return;
		}
		default:
			break;
	}
	left->putBytecodes(in_data, bytecodes);
	right->putBytecodes(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::IN_: {
			switch (right->kind) {
				case NodeType::RANGE: {
					bytecodes.emplace_back(Opcode::IN_RANGE);
					bytecodes.emplace_back(
					    static_cast<RangeNode *>(right)->lessThan);
					return;
				}
				default: {
					throwError("Operator 'in' is currently only supported for "
					           "Range types\nHint: Check if the right operand is a valid range (e.g. start..end).");
				}
			}
		}
		case Lexer::TokenType::PLUS: {
			// switch (left->classId) {
			// 	case DefaultClass::intClassId: {
			// 		switch (right->classId) {
			// 			case DefaultClass::intClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::I_CAL_I);
			// 				return;
			// 			}
			// 			case DefaultClass::floatClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::I_CAL_F);
			// 				return;
			// 			}
			// 		}
			// 		break;
			// 	}
			// 	case DefaultClass::floatClassId: {
			// 		switch (right->classId) {
			// 			case DefaultClass::intClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::F_CAL_I);
			// 				return;
			// 			}
			// 			case DefaultClass::floatClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::F_CAL_F);
			// 				return;
			// 			}
			// 		}
			// 		break;
			// 	}
			// }
			bytecodes.emplace_back(Autolang::Opcode::PLUS);
			return;
		}
		case Lexer::TokenType::MINUS: {
			// switch (left->classId) {
			// 	case DefaultClass::intClassId: {
			// 		switch (right->classId) {
			// 			case DefaultClass::intClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::I_MINUS_I);
			// 				return;
			// 			}
			// 			case DefaultClass::floatClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::I_MINUS_F);
			// 				return;
			// 			}
			// 		}
			// 		break;
			// 	}
			// 	case DefaultClass::floatClassId: {
			// 		switch (right->classId) {
			// 			case DefaultClass::intClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::F_MINUS_I);
			// 				return;
			// 			}
			// 			case DefaultClass::floatClassId: {
			// 				bytecodes.emplace_back(Autolang::Opcode::F_MINUS_F);
			// 				return;
			// 			}
			// 		}
			// 		break;
			// 	}
			// }
			bytecodes.emplace_back(Autolang::Opcode::MINUS);
			return;
		}
		case Lexer::TokenType::STAR:
			bytecodes.emplace_back(Autolang::Opcode::MUL);
			return;
		case Lexer::TokenType::SLASH:
			bytecodes.emplace_back(Autolang::Opcode::DIVIDE);
			return;
		case Lexer::TokenType::PERCENT:
			bytecodes.emplace_back(Autolang::Opcode::MOD);
			return;
		case Lexer::TokenType::AND:
			bytecodes.emplace_back(Autolang::Opcode::BITWISE_AND);
			return;
		case Lexer::TokenType::OR:
			bytecodes.emplace_back(Autolang::Opcode::BITWISE_OR);
			return;
		case Lexer::TokenType::NOTEQEQ:
			bytecodes.emplace_back(Autolang::Opcode::NOTEQ_POINTER);
			return;
		case Lexer::TokenType::EQEQEQ:
			bytecodes.emplace_back(Autolang::Opcode::EQUAL_POINTER);
			return;
		case Lexer::TokenType::NOTEQ:
			bytecodes.emplace_back(Autolang::Opcode::NOTEQ_VALUE);
			return;
		case Lexer::TokenType::EQEQ:
			bytecodes.emplace_back(Autolang::Opcode::EQUAL_VALUE);
			return;
		case Lexer::TokenType::LT:
			bytecodes.emplace_back(Autolang::Opcode::LESS_THAN);
			return;
		case Lexer::TokenType::GT:
			bytecodes.emplace_back(Autolang::Opcode::GREATER_THAN);
			return;
		case Lexer::TokenType::GTE:
			bytecodes.emplace_back(Autolang::Opcode::GREATER_THAN_EQ);
			return;
		case Lexer::TokenType::LTE:
			bytecodes.emplace_back(Autolang::Opcode::LESS_THAN_EQ);
			return;
		default:
			// std::cerr<<this<<'\n';
			throwError(std::string("Cannot find operator '") +
			           Lexer::Token(0, op).toString(context) +
			           "'\nHint: The binary operator is unsupported or missing bytecode implementation.");
	}
}

ExprNode *BinaryNode::copy(in_func) {
	return context.binaryNodePool.push(
	    line, tokenIndex, contextCallClassId, op,
	    static_cast<HasClassIdNode *>(left->copy(in_data)),
	    static_cast<HasClassIdNode *>(right->copy(in_data)));
}

} // namespace Autolang

#endif