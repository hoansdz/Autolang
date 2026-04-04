#ifndef SET_NODE_CPP
#define SET_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *SetNode::resolve(in_func) {
	detach = static_cast<HasClassIdNode *>(detach->resolve(in_data));
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	if (detach->kind == NodeType::CALL) {
		auto result = static_cast<CallNode *>(detach);
		if (result->nameId != lexerIdLRBRACKET)
			return this;
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
						auto classInfo = context.classInfo[detach->classId];
						if (classInfo->baseClassId !=
						    DefaultClass::arrayClassId) {
							if (detach->classId == DefaultClass::nullClassId) {
								throwError("Cannot infer type for initializer "
								           ". Autolang requires explicit type "
								           "parameters for collection sugar. "
								           "Use <Type>[]");
							}
							throwError("Type mismatch: Expected Array<> but '" +
							           compile.classes[detach->classId]->name +
							           "' found");
						}
						createArrayNode->classId = detach->classId;
					}
					break;
				}
				case NodeType::CREATE_SET: {
					auto createSetNode = static_cast<CreateSetNode *>(value);
					if (!createSetNode->classDeclaration) {
						auto classInfo = context.classInfo[detach->classId];
						if (classInfo->baseClassId !=
						    DefaultClass::setClassId) {
							if (classInfo->baseClassId ==
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
									    "Cannot infer type for initializer "
									    ". Autolang requires explicit type "
									    "parameters for collection sugar. "
									    "Use <Type>{}");
								}
								throwError(
								    "Type mismatch: Expected " +
								    compile.classes[detach->classId]->name +
								    " but Set<> found");
							}
						} else {
							createSetNode->classId = detach->classId;
						}
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case DefaultClass::functionClassId: {
			auto n = static_cast<FunctionAccessNode *>(value);
			n->classDeclaration = detach->classDeclaration;
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
				           "' in class " + clazz->name);
			}
			std::vector<FunctionId> *funcs[1];
			funcs[0] = &it->second;
			auto caller = valueNode->caller->isStaticValue()
			                  ? nullptr
			                  : valueNode->caller;
			value = context.functionAccessPool.push(
			    valueNode->line, caller, valueNode->nameId, 1,
			    std::vector<HasClassIdNode *>{}, funcs);
			value->classDeclaration = detach->classDeclaration;
			value->optimize(in_data);
		}
	} else {
		value->optimize(in_data);
	}

	if (justDetachStatic && !value->isStaticValue()) {
		throwError("Value must be static");
	}

	if (value->classId == DefaultClass::voidClassId) {
		throwError("Cannot detach void value");
	}

	if (value->isNullable() && op != Lexer::TokenType::EQUAL) {
		throwError("Cannot use operator '" +
		           Lexer::Token(0, op).toString(context) +
		           "' with nullable variables");
	}

	classId = value->classId;

	bool detachNullable;

	switch (detach->kind) {
		case NodeType::GET_PROP: {
			auto detachNode = static_cast<GetPropNode *>(detach);
			detachNullable = detachNode->nullable;
			detachNode->isStore = true;
			if (detach->classId != AutoLang::DefaultClass::nullClassId) {
				break;
			}
			if (detachNode->classId == AutoLang::DefaultClass::nullClassId) {
				if (detachNode->declaration->classId ==
				    AutoLang::DefaultClass::nullClassId) {
					if (value->classId == AutoLang::DefaultClass::nullClassId) {
						throwError("Ambigous inference member class id");
					}
					detachNode->declaration->classId = value->classId;
					detachNode->declaration->classDeclaration =
					    value->classDeclaration;
					compile.classes[detachNode->caller->classId]
					    ->memberId[detachNode->declaration->id] =
					    detachNode->declaration->classId;
					// Marked non null won't run example val a! = 1
					if (detachNode->declaration->mustInferenceNullable) {
						detachNode->declaration->nullable = value->isNullable();
						detachNode->nullable =
						    detachNode->declaration->nullable;
						detachNullable = detachNode->nullable;
					}
					// printDebug(std::string("SetNode: Declaration ") +
					// node->declaration->name + " is " +
					// compile.classes[value->classId]->name);
				}
				detach->classId = value->classId;
				detach->classDeclaration = value->classDeclaration;
			}
			// if (detachNode->declaration->accessModifier ==
			// Lexer::TokenType::PRIVATE) { 	if (detachNode->classId !=
			// detachNode->declaration->classId)
			// }
			if (detachNode->isVal) {
				throwError("Cannot change " +
				           compile.classes[detachNode->caller->classId]->name +
				           "." + context.lexerString[detachNode->nameId] +
				           " because it's val");
			}
			// Nullable
			if (value->classId == AutoLang::DefaultClass::nullClassId) {
				if (!detachNode->declaration->nullable) {
					throwError(
					    detachNode->declaration->name +
					    " cannot detach null value, you must declare " +
					    compile.classes[detachNode->declaration->classId]
					        ->name +
					    "? to can detach null");
				}
				if (op != Lexer::TokenType::EQUAL) {
					throwError(detachNode->declaration->name +
					           " cannot use operator" +
					           Lexer::Token(0, op).toString(context) +
					           " with null value");
				}
				return;
			}
			auto clazz = compile.classes[detachNode->caller->classId];
			// clazz->memberId[detach->declaration->id] = value->classId;
			detachNode->declaration->classId = value->classId;
			detachNode->declaration->classDeclaration = value->classDeclaration;
			break;
		}
		case NodeType::VAR: {
			auto node = static_cast<VarNode *>(detach);
			node->isStore = true;
			detachNullable = node->nullable;
			if (detach->classId != AutoLang::DefaultClass::nullClassId &&
			    detach->classId != node->declaration->classId) {
				detach->classId = node->declaration->classId;
			}
			// First value example val a = 1
			if (detach->classId == AutoLang::DefaultClass::nullClassId) {
				if (node->declaration->classId ==
				        AutoLang::DefaultClass::nullClassId &&
				    value->classId != AutoLang::DefaultClass::nullClassId) {
					node->declaration->classId = value->classId;
					node->declaration->classDeclaration =
					    value->classDeclaration;
					// Marked non null won't run example val a! = 1
					if (node->declaration->mustInferenceNullable) {
						node->declaration->nullable = value->isNullable();
						node->nullable = node->declaration->nullable;
						detachNullable = node->nullable;
						// std::cerr << "Set " << node->declaration->name << "
						// is "
						//           << (detachNullable ? "nullable" :
						//           "nonnull")
						//           << "\n";
					}
					// printDebug(std::string("SetNode: Declaration ") +
					// node->declaration->name + " is " +
					// compile.classes[value->classId]->name);
				}
				detach->classId = value->classId;
				detach->classDeclaration = value->classDeclaration;
			}
			// Nullable
			if (value->classId == AutoLang::DefaultClass::nullClassId) {
				if (!detachNullable) {
					throwError(
					    node->declaration->name +
					    " cannot detach null value, you must declare " +
					    compile.classes[node->declaration->classId]->name +
					    "? to can detach null");
				}
				if (op != Lexer::TokenType::EQUAL) {
					throwError(node->declaration->name +
					           " cannot use operator" +
					           Lexer::Token(0, op).toString(context) +
					           " with null value");
				}
				return;
			}
			break;
		}
		default: {
			throwError("Invalid assignment target");
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
			case NodeType::CONST: {
				if (op != Lexer::TokenType::EQUAL) {
					// Optimize call primary instead of copies
					static_cast<ConstValueNode *>(value)->isLoadPrimary = true;
				}
				break;
			}
			case NodeType::VAR:
			case NodeType::GET_PROP: {
				if (!detachNullable &&
				    static_cast<AccessNode *>(value)->nullable) {
					std::string detachName;
					detachName =
					    static_cast<AccessNode *>(detach)->declaration->name;
					throwError(
					    "Cannot detach nullable variable " +
					    static_cast<AccessNode *>(value)->declaration->name +
					    " to nonnull variable " + detachName);
				}
				break;
			}
			case NodeType::CALL: {
				if (!detachNullable &&
				    static_cast<CallNode *>(value)->nullable) {
					std::string detachName;
					detachName =
					    static_cast<AccessNode *>(detach)->declaration->name;
					throwError(
					    "Cannot detach nullable value " +
					    context.lexerString[static_cast<CallNode *>(value)
					                            ->nameId] +
					    " to nonnull variable " + detachName);
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
			           "' with nullable value");
		}
	} else if (value->isNullable()) {
		throwError("Cannot detach '" + compile.classes[detach->classId]->name +
		           "' with '" + compile.classes[value->classId]->name +
		           "?' (Nullable value)");
	}

	if (detach->classId == value->classId) {
		if (op != Lexer::TokenType::EQUAL) {
			switch (detach->classId) {
				case AutoLang::DefaultClass::intClassId:
				case AutoLang::DefaultClass::floatClassId:
					return;
				default:
					if (detach->classId ==
					        AutoLang::DefaultClass::stringClassId &&
					    op == Lexer::TokenType::PLUS_EQUAL)
						return;
					break;
			}
			throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
			           " operator with " +
			           compile.classes[detach->classId]->name + " and " +
			           compile.classes[value->classId]->name);
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
							    value->classDeclaration->getName(in_data));
						}
					}
					return;
				} else {
					throwError("Type mismatch: expected '" +
					           detach->classDeclaration->getName(in_data) +
					           "' but found '" +
					           value->classDeclaration->getName(in_data));
				}
			}
		}
		return;
	}
	if ((detach->classId == AutoLang::DefaultClass::intClassId ||
	     detach->classId == AutoLang::DefaultClass::floatClassId) &&
	    (value->classId == AutoLang::DefaultClass::intClassId ||
	     value->classId == AutoLang::DefaultClass::floatClassId)) {
		if (value->kind != NodeType::CONST) {
			value = context.castPool.push(value, detach->classId);
			return;
		}
		// Optimize
		try {
			switch (detach->classId) {
				case AutoLang::DefaultClass::intClassId:
					toInt(in_data, static_cast<ConstValueNode *>(value));
					return;
				case AutoLang::DefaultClass::floatClassId:
					toFloat(in_data, static_cast<ConstValueNode *>(value));
					return;
				default:
					throwError("What happen");
			}
		} catch (const std::runtime_error &err) {
			throwError("Cannot cast " + compile.classes[value->classId]->name +
			           " to " + compile.classes[detach->classId]->name);
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
		case NodeType::VAR: {
			throwError("Type mismatch: expected '" +
			           compile.classes[detach->classId]->name +
			           "' but found '" + compile.classes[value->classId]->name +
			           (value->isNullable() ? "?" : "") + "'");
		}
		case NodeType::GET_PROP: {
			throwError("Type mismatch: expected '" +
			           compile.classes[detach->classId]->name +
			           "' but found '" + compile.classes[value->classId]->name +
			           (value->isNullable() ? "?" : "") + "'");
		}
		default:
			throwError(",Wtf");
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
						bytecodes.emplace_back(
						    valueNode->declaration->isGlobal
						        ? Opcode::GLOBAL_STORE_GLOBAL
						        : Opcode::GLOBAL_STORE_LOCAL);
						put_opcode_u32(bytecodes, detachNode->declaration->id);
						put_opcode_u32(bytecodes, valueNode->declaration->id);
						return;
					}
					bytecodes.emplace_back(valueNode->declaration->isGlobal
					                           ? Opcode::LOCAL_STORE_GLOBAL
					                           : Opcode::LOCAL_STORE_LOCAL);
					put_opcode_u32(bytecodes, detachNode->declaration->id);
					put_opcode_u32(bytecodes, valueNode->declaration->id);
					return;
				}
				case NodeType::CONST: {
					auto valueNode = static_cast<ConstValueNode *>(value);
					bytecodes.emplace_back(detachNode->declaration->isGlobal
					                           ? Opcode::GLOBAL_STORE_CONST
					                           : Opcode::LOCAL_STORE_CONST);
					put_opcode_u32(bytecodes, detachNode->declaration->id);
					put_opcode_u32(bytecodes, valueNode->id);
					return;
				}
			}

			break;
		}
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

} // namespace AutoLang

#endif