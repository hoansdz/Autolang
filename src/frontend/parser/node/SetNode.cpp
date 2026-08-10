#ifndef SET_NODE_CPP
#define SET_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *SetNode::resolve(in_func) {
	detach = static_cast<HasClassIdNode *>(detach->resolve(in_data));
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	if (detach->kind == NodeType::CALL) {
		auto result = static_cast<CallNode *>(detach);
		if (result->nameId != lexerIdLRBRACKET)
			return this;
		if (op != Lexer::TokenType::EQUAL) {
			throwError("Cannot perform read-modify-write operation on index "
			           "access. Use direct assignment instead\nHint: Index access operations (like `arr[i] += val`) must be performed using direct assignment (`arr[i] = ...`).");
		}
		result->nameId = lexerIdset;
		result->arguments.push_back(value);
		detach = nullptr;
		value = nullptr;
		return result;
	}
	return this;
}

void SetNode::optimize(in_func) {
	// Detach has nullClassId because it was not evaluated
	detach->optimize(in_data);

	switch (value->classId) {
		case DefaultClass::nullClassId: {
			switch (value->kind) {
				case NodeType::CREATE_ARRAY: {
					auto createArrayNode =
					    static_cast<CreateArrayNode *>(value);
					if (!createArrayNode->classDeclaration) {
						auto clazz = compile.classes[detach->classId];
						if (clazz->genericBaseClassId !=
						    DefaultClass::arrayClassId) {
							if (detach->classId == DefaultClass::nullClassId) {
								throwError("Cannot infer type for initializer. Autolang requires explicit type parameters for collection sugar.\nHint: Declare explicitly, for example: `<Type>[]`.");
							}
							throwError("Type mismatch: Expected Array<> but '" +
							           detach->getClassName(in_data) +
							           "' found\nHint: Target assignment variable must be of type Array<T>.");
						}
						createArrayNode->classId = detach->classId;
					}
					break;
				}
				case NodeType::CREATE_SET: {
					auto createSetNode = static_cast<CreateSetNode *>(value);
					if (!createSetNode->classDeclaration) {
						auto clazz = compile.classes[detach->classId];
						if (clazz->genericBaseClassId !=
						    DefaultClass::setClassId) {
							if (clazz->genericBaseClassId ==
							        DefaultClass::mapClassId &&
							    createSetNode->values.empty()) {
								auto newValue = context.createMapPool.push(
								    value->line, nullptr,
								    std::vector<std::pair<HasClassIdNode *,
								                          HasClassIdNode *>>());
								value = newValue;
								newValue->classId = detach->classId;
							} else {
								if (detach->classId ==
								    DefaultClass::nullClassId) {
									throwError(
									    "Cannot infer type for initializer. Autolang requires explicit type parameters for collection sugar.\nHint: Declare explicitly, for example: `<Type>{}`.");
								}
								throwError("Type mismatch: Expected " +
								           detach->getClassName(in_data) +
								           " but Set<> found\nHint: Ensure assignment target matches Set<T> type.");
							}
						} else {
							createSetNode->classId = detach->classId;
						}
					}
					break;
				}
				case NodeType::CREATE_MAP: {
					auto createMapNode = static_cast<CreateMapNode *>(value);
					if (!createMapNode->classDeclaration) {
						auto clazz = compile.classes[detach->classId];
						if (clazz->genericBaseClassId !=
						    DefaultClass::mapClassId) {
							if (detach->classId == DefaultClass::nullClassId) {
								throwError("Cannot infer type for initializer. Autolang requires explicit type parameters for collection sugar.\nHint: Declare explicitly, for example: `<KeyClass, ValueClass>{}`.");
							}
							throwError("Type mismatch: Expected Map<> but '" +
							           detach->getClassName(in_data) +
							           "' found\nHint: Target assignment variable must be of type Map<K, V>.");
						}
						createMapNode->classId = detach->classId;
					}
					break;
				}
				case NodeType::IF: {
					auto ifNode = static_cast<IfNode *>(value);
					ifNode->classId = detach->classId;
					ifNode->nullable = detach->isNullable();
					if (ifNode->classId == DefaultClass::functionClassId) {
						ifNode->classDeclaration = detach->classDeclaration;
					}
					break;
				}
				case NodeType::WHEN: {
					auto whenNode = static_cast<WhenNode *>(value);
					whenNode->classId = detach->classId;
					whenNode->nullable = detach->isNullable();
					if (whenNode->classId == DefaultClass::functionClassId) {
						whenNode->classDeclaration = detach->classDeclaration;
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case DefaultClass::functionClassId: {
			if (detach->classId != DefaultClass::nullClassId) {
				switch (value->kind) {
					case NodeType::FUNCTION_ACCESS: {
						auto n = static_cast<HasClassIdNode *>(value);
						n->classDeclaration = detach->classDeclaration;
						break;
					}
					case NodeType::CREATE_CLOSURE: {
						auto n = static_cast<CreateClosureNode *>(value);
						n->inferFrom(in_data, detach->classDeclaration);
						break;
					}
					default:
						break;
				}
			}
			break;
		}
	}

	if (detach->classId == DefaultClass::functionClassId &&
	    value->kind == NodeType::GET_PROP) {
		auto valueNode = static_cast<GetPropNode *>(value);
		if (valueNode->optimizeSkipIfNotFoundMember(in_data)) {
			// Skiped
			auto callClassInfo = context.classInfo[valueNode->caller->classId];
			auto it = callClassInfo->allFunction.find(valueNode->nameId);
			if (it == callClassInfo->allFunction.end()) {
				auto clazz = compile.classes[valueNode->caller->classId];
				throwError("Cannot find member name '" +
				           context.lexerString[valueNode->nameId] +
				           "' in class " + clazz->getName(compile) +
				           "\nHint: Verify member name spelling and accessibility in class " + clazz->getName(compile) + ".");
			}
			std::vector<FunctionId> *funcs[1];
			funcs[0] = &it->second;
			auto caller = valueNode->caller->isStaticValue()
			                  ? nullptr
			                  : valueNode->caller;
			value = context.functionAccessPool.push(
			    valueNode->line, caller, valueNode->nameId, 1, nullptr, funcs);
			value->classDeclaration = detach->classDeclaration;
			value->optimize(in_data);
		}
	} else {
		value->optimize(in_data);
	}

	if (justDetachStatic && !value->isStaticValue()) {
		throwError("Assigned value must be a static value\nHint: Static property assignment requires a static value expression.");
	}

	if (value->classId == DefaultClass::voidClassId) {
		throwError("Cannot assign expression of type 'Void'\nHint: Expressions returning Void do not produce a value and cannot be assigned to variables.");
	}

	if (value->isNullable() && op != Lexer::TokenType::EQUAL) {
		throwError("Cannot use operator '" +
		           Lexer::Token(0, op).toString(context) +
		           "' with nullable variables\nHint: Compound assignment operators cannot be used on nullable values. Unwrap the value with '!' or perform a null check.");
	}

	classId = value->classId;

	switch (detach->kind) {
		case NodeType::GET_PROP: {
			auto detachNode = static_cast<GetPropNode *>(detach);
			detachNode->isStore = true;
			detachNode->cloneable = false;
			if (detach->classId != Autolang::DefaultClass::nullClassId) {
				break;
			}
			if (detachNode->classId == Autolang::DefaultClass::nullClassId) {
				if (detachNode->declaration->classId ==
				    Autolang::DefaultClass::nullClassId) {
					if (value->classId == Autolang::DefaultClass::nullClassId) {
						throwError("Ambiguous type inference for member variable\nHint: Provide an explicit type annotation when declaring member variable initialized with null.");
					}
					detachNode->declaration->classId = value->classId;
					if (value->classId == DefaultClass::functionClassId) {
						detachNode->declaration->classDeclaration =
						    value->classDeclaration;
					}
					// Marked non null won't run example val a! = 1
					if (detachNode->declaration->mustInferenceNullable) {
						detachNode->declaration->nullable = value->isNullable();
						detachNode->nullable =
						    detachNode->declaration->nullable;
					}
					// printDebug(std::string("SetNode: Declaration ") +
					// node->declaration->getName(compile) + " is " +
					// compile.classes[value->classId]->getName(compile));
				}
				detach->classId = value->classId;
				if (value->classId == DefaultClass::functionClassId) {
					detach->classDeclaration = value->classDeclaration;
				}
			}
			// if (detachNode->declaration->accessModifier ==
			// Lexer::TokenType::PRIVATE) { 	if (detachNode->classId !=
			// detachNode->declaration->classId)
			// }
			if (detachNode->isVal) {
				throwError(
				    "Cannot change " +
				    compile.classes[detachNode->caller->classId]->getName(
				        compile) +
				    "." + context.lexerString[detachNode->nameId] +
				    " because it's val\nHint: Properties declared with 'val' are immutable and cannot be reassigned.");
			}
			// Nullable
			if (value->classId == Autolang::DefaultClass::nullClassId) {
				if (!detachNode->declaration->nullable) {
					throwError(
					    detachNode->declaration->name +
					    " cannot detach null value, you must declare " +
					    compile.classes[detachNode->declaration->classId]
					        ->getName(compile) +
					    "? to can detach null\nHint: Declare member variable as nullable type (" + compile.classes[detachNode->declaration->classId]->getName(compile) + "?) to allow null assignment.");
				}
				if (op != Lexer::TokenType::EQUAL) {
					throwError(detachNode->declaration->name +
					           " cannot use operator " +
					           Lexer::Token(0, op).toString(context) +
					           " with null value\nHint: Compound assignment cannot be used when assigned value is null.");
				}
				return;
			}
			auto clazz = compile.classes[detachNode->caller->classId];
			// clazz->memberId[detach->declaration->id] = value->classId;
			detachNode->declaration->classId = value->classId;
			if (value->classId == DefaultClass::functionClassId) {
				detachNode->declaration->classDeclaration =
				    value->classDeclaration;
			}
			break;
		}
		case NodeType::VAR: {
			auto node = static_cast<VarNode *>(detach);
			node->isStore = true;
			node->cloneable = false;
			if (detach->classId != Autolang::DefaultClass::nullClassId &&
			    detach->classId != node->declaration->classId) {
				detach->classId = node->declaration->classId;
			}
			// First value example val a = 1
			if (detach->classId == Autolang::DefaultClass::nullClassId) {
				if (node->declaration->classId ==
				        Autolang::DefaultClass::nullClassId &&
				    value->classId != Autolang::DefaultClass::nullClassId) {
					node->declaration->classId = value->classId;
					if (value->classId == DefaultClass::functionClassId) {
						node->declaration->classDeclaration =
						    value->classDeclaration;
					}
					// Marked non null won't run example val a! = 1
					if (node->declaration->mustInferenceNullable) {
						node->declaration->nullable = value->isNullable();
						node->nullable = node->declaration->nullable;
						// std::cerr << "Set " <<
						// node->declaration->getName(compile) << " is "
						//           << (detachNullable ? "nullable" :
						//           "non null")
						//           << "\n";
					}
					// if (value->classDeclaration) {
					// 	std::cerr
					// 	    << (std::string("SetNode: Declaration ") +
					// 	        node->declaration->getName(compile) + " is " +
					// 	        value->classDeclaration->getName<true>(in_data))
					// 	    << "\n";
					// } else {
					// 	std::cerr << (std::string("SetNode__: Declaration ") +
					// 	              node->declaration->getName(compile) + " is
					// " + compile.classes[value->classId]->getName(compile))
					// 	          << "\n";
					// 	//   value->getClassName(in_data))
					// }
				}
				detach->classId = value->classId;
				if (value->classId == DefaultClass::functionClassId) {
					detach->classDeclaration = value->classDeclaration;
				}
			}
			// Nullable
			if (value->classId == Autolang::DefaultClass::nullClassId) {
				if (!detach->isNullable()) {
					throwError(
					    node->declaration->name +
					    " cannot detach null value, you must declare " +
					    compile.classes[node->declaration->classId]->getName(
					        compile) +
					    "? to can detach null\nHint: Declare variable as nullable type (" + compile.classes[node->declaration->classId]->getName(compile) + "?) to allow null assignment.");
				}
				if (op != Lexer::TokenType::EQUAL) {
					throwError(node->declaration->name +
					           " cannot use operator " +
					           Lexer::Token(0, op).toString(context) +
					           " with null value\nHint: Compound assignment cannot be used when assigned value is null.");
				}
				return;
			}
			break;
		}
		default: {
			throwError("Invalid assignment target\nHint: Assignment target must be a variable, property, or index expression.");
		}
	}

	{
		auto value = this->value;
	changedValue:;
		switch (value->kind) {
			case NodeType::OPTIONAL_ACCESS: {
				value = static_cast<OptionalAccessNode *>(value)->value;
				goto changedValue;
			}
			case NodeType::CONST_VAL: {
				if (op != Lexer::TokenType::EQUAL ||
				    static_cast<AccessNode *>(detach)->isVal) {
					// Optimize call primary instead of copies
					static_cast<ConstValueNode *>(value)->isLoadPrimary = true;
				}
				break;
			}
			case NodeType::VAR:
			case NodeType::GET_PROP: {
				auto node = static_cast<AccessNode *>(value);
				auto detachNode = static_cast<AccessNode *>(detach);
				if (!detach->isNullable() && node->nullable) {
					std::string detachName;
					detachName = detachNode->declaration->name;
					throwError("Cannot assign nullable variable '" +
					           node->declaration->name +
					           "' to non-null variable '" + detachName + "'\nHint: Use non-null assertion ('!') or check nullability before assignment.");
				}
				// if (detachNode->isVal && node->isVal) {
				// 	node->cloneable = false;
				// }
				break;
			}
			case NodeType::CALL: {
				if (!detach->isNullable() &&
				    static_cast<CallNode *>(value)->nullable) {
					std::string detachName;
					detachName =
					    static_cast<AccessNode *>(detach)->declaration->name;
					throwError(
					    "Cannot assign nullable return value of '" +
					    context.lexerString[static_cast<CallNode *>(value)
					                            ->nameId] +
					    "' to non-null variable '" + detachName + "'\nHint: Function return type is nullable. Unwrap return value with '!' or declare variable as nullable.");
				}
				break;
			}
			default:
				break;
		}
	}

	if (detach->isNullable()) {
		if (op != Lexer::TokenType::EQUAL) {
			throwError("Cannot use operator '" +
			           Lexer::Token(0, op).toString(context) +
			           "' with nullable value\nHint: Cannot use compound assignment operators on nullable target without prior null check.");
		}
	} else if (value->isNullable()) {
		throwError("Cannot assign nullable type '" +
		           compile.classes[value->classId]->getName(compile) +
		           "?' to non-null variable of type '" +
		           compile.classes[detach->classId]->getName(compile) + "'\nHint: Target variable is non-nullable. Unwrap assigned value using '!' or declare target as nullable.");
	}

	if (detach->classId == value->classId) {
		if (op != Lexer::TokenType::EQUAL) {
			switch (detach->classId) {
				case Autolang::DefaultClass::intClassId:
				case Autolang::DefaultClass::floatClassId:
					return;
				default:
					if (detach->classId ==
					        Autolang::DefaultClass::stringClassId &&
					    op == Lexer::TokenType::PLUS_EQUAL)
						return;
					break;
			}
			throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
			           " operator with " +
			           compile.classes[detach->classId]->getName(compile) +
			           " and " +
			           compile.classes[value->classId]->getName(compile) +
			           "\nHint: Operator '" + Lexer::Token(0, op).toString(context) + "' is not supported for these operand types.");
		} else {
			if (detach->classId == DefaultClass::functionClassId &&
			    detach->classDeclaration != value->classDeclaration) {
				size_t size = detach->classDeclaration->inputClassId.size();
				if (size == value->classDeclaration->inputClassId.size()) {
					for (int i = 0; i < size; ++i) {
						if (detach->classDeclaration->inputClassId[i]
						        ->classId !=
						    value->classDeclaration->inputClassId[i]->classId) {
							throwError(
							    "Type mismatch: expected '" +
							    detach->classDeclaration->getName(in_data) +
							    "' but found '" +
							    value->classDeclaration->getName(in_data) +
							    "'\nHint: Function signatures do not match parameter types.");
						}
					}
					return;
				} else {
					throwError("Type mismatch: expected '" +
					           detach->classDeclaration->getName(in_data) +
					           "' but found '" +
					           value->classDeclaration->getName(in_data) +
					           "'\nHint: Function signature argument count or return type mismatch.");
				}
			}
		}
		return;
	}
	if ((detach->classId == Autolang::DefaultClass::intClassId ||
	     detach->classId == Autolang::DefaultClass::floatClassId) &&
	    (value->classId == Autolang::DefaultClass::intClassId ||
	     value->classId == Autolang::DefaultClass::floatClassId)) {
		if (detach->classId == Autolang::DefaultClass::intClassId &&
		    value->classId == Autolang::DefaultClass::floatClassId) {
			throwError("Cannot cast 'Float' to 'Int'\nHint: Implicit truncation from Float to Int is disallowed. Convert explicitly.");
		}
		if (value->kind != NodeType::CONST_VAL) {
			value = context.castPool.push(value, detach->classId);
			return;
		}
		// Optimize
		try {
			switch (detach->classId) {
				case Autolang::DefaultClass::intClassId:
					value =
					    toInt(in_data, static_cast<ConstValueNode *>(value));
					value->optimize(in_data);
					return;
				case Autolang::DefaultClass::floatClassId:
					value =
					    toFloat(in_data, static_cast<ConstValueNode *>(value));
					value->optimize(in_data);
					return;
				default:
					throwError("Invalid cast target type\nHint: Target type is invalid for primitive constant casting.");
			}
		} catch (const ParserError &err) {
			throwError("Cannot cast " +
			           compile.classes[value->classId]->getName(compile) +
			           " to " +
			           compile.classes[detach->classId]->getName(compile) +
			           "\nHint: No valid type conversion exists between these types.");
		}
	}
	if (detach->classId == DefaultClass::anyClassId) {
		return;
	}
	if (detach->isNullable() && value->classId == DefaultClass::nullClassId) {
		return;
	}
	if (compile.classes[value->classId]->inheritance.get(detach->classId)) {
		return;
	}
	switch (detach->kind) {
		case NodeType::VAR:
		case NodeType::GET_PROP:
		default:
			throwError("Type mismatch: expected '" +
			           compile.classes[detach->classId]->getName(compile) +
			           "' but found '" +
			           compile.classes[value->classId]->getName(compile) +
			           (value->isNullable() ? "?" : "") + "'\nHint: Assigned expression type does not match target variable/property type.");
	}
}

#define operator_plus_case(type, op)                                           \
	case Lexer::TokenType::type: {                                             \
		auto _node = static_cast<AccessNode *>(detach);                        \
		_node->isStore = false;                                                \
		_node->putBytecodes(in_data, bytecodes);                               \
		value->putBytecodes(in_data, bytecodes);                               \
		bytecodes.emplace_back(Opcode::op);                                    \
		return;                                                                \
	}

void SetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	if (BinaryNode::putOptimizedBytecode(in_data, bytecodes, op, detach,
	                                     value)) {
		return;
	}
	switch (op) {
		operator_plus_case(PLUS_EQUAL, PLUS_EQUAL);
		operator_plus_case(MINUS_EQUAL, MINUS_EQUAL);
		operator_plus_case(STAR_EQUAL, MUL_EQUAL);
		operator_plus_case(SLASH_EQUAL, DIVIDE_EQUAL);
		default: {
			break;
			// throwError("Unexpected op "+ Lexer::Token(0,
			// op).toString(context));
		}
	}
	switch (detach->kind) {
		case NodeType::VAR: {
			auto detachNode = static_cast<VarNode *>(detach);
			switch (value->kind) {
				case NodeType::VAR: {
					auto valueNode = static_cast<VarNode *>(value);
					if (detachNode->declaration->isGlobal) {
						if (valueNode->cloneable) {
							bytecodes.emplace_back(
							    valueNode->declaration->isGlobal
							        ? Opcode::GLOBAL_STORE_GLOBAL_CLONE
							        : Opcode::GLOBAL_STORE_LOCAL_CLONE);
						} else {
							bytecodes.emplace_back(
							    valueNode->declaration->isGlobal
							        ? Opcode::GLOBAL_STORE_GLOBAL
							        : Opcode::GLOBAL_STORE_LOCAL);
						}
						put_opcode_u32(bytecodes, detachNode->declaration->id);
						put_opcode_u32(bytecodes, valueNode->declaration->id);
						return;
					}
					if (valueNode->cloneable) {
						bytecodes.emplace_back(
						    valueNode->declaration->isGlobal
						        ? Opcode::LOCAL_STORE_GLOBAL_CLONE
						        : Opcode::LOCAL_STORE_LOCAL_CLONE);
					} else {
						bytecodes.emplace_back(valueNode->declaration->isGlobal
						                           ? Opcode::LOCAL_STORE_GLOBAL
						                           : Opcode::LOCAL_STORE_LOCAL);
					}

					put_opcode_u32(bytecodes, detachNode->declaration->id);
					put_opcode_u32(bytecodes, valueNode->declaration->id);
					return;
				}
				case NodeType::CONST_VAL: {
					auto valueNode = static_cast<ConstValueNode *>(value);
					if (valueNode->isLoadPrimary) {
						bytecodes.emplace_back(detachNode->declaration->isGlobal
						                           ? Opcode::GLOBAL_STORE_CONST
						                           : Opcode::LOCAL_STORE_CONST);
					} else {
						bytecodes.emplace_back(
						    detachNode->declaration->isGlobal
						        ? Opcode::GLOBAL_STORE_CONST_CLONE
						        : Opcode::LOCAL_STORE_CONST_CLONE);
					}
					put_opcode_u32(bytecodes, detachNode->declaration->id);
					put_opcode_u32(bytecodes, valueNode->id);
					return;
				}
				default:
					break;
			}

			break;
		}
		default:
			break;
	}
	value->putBytecodes(in_data, bytecodes);
	detach->putBytecodes(in_data, bytecodes);
}

ExprNode *SetNode::copy(in_func) {
	auto newDetachNode = static_cast<HasClassIdNode *>(detach->copy(in_data));
	auto newValueNode = static_cast<HasClassIdNode *>(value->copy(in_data));
	return context.setValuePool.push(line, newDetachNode, newValueNode,
	                                 justDetachStatic);
}

SetNode::~SetNode() {
	deleteNode(detach);
	deleteNode(value);
}

} // namespace Autolang

#endif