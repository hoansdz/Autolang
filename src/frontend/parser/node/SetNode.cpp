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

	if (value->classId == DefaultClass::nullClassId) {
		switch (value->kind) {
			case NodeType::CREATE_ARRAY: {
				auto createArrayNode = static_cast<CreateArrayNode *>(value);
				if (!createArrayNode->classDeclaration) {
					auto classInfo = context.classInfo[detach->classId];
					if (classInfo->baseClassId != DefaultClass::arrayClassId) {
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
					if (classInfo->baseClassId != DefaultClass::setClassId) {
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
							if (detach->classId == DefaultClass::nullClassId) {
								throwError("Cannot infer type for initializer "
								           ". Autolang requires explicit type "
								           "parameters for collection sugar. "
								           "Use <Type>{}");
							}
							throwError("Type mismatch: Expected " +
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
	}

	value->optimize(in_data);

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
			}
			// if (detachNode->declaration->accessModifier ==
			// Lexer::TokenType::PRIVATE) { 	if (detachNode->classId !=
			// detachNode->declaration->classId)
			// }
			if (detachNode->isVal) {
				throwError("Cannot change " +
				           compile.classes[detachNode->caller->classId]->name +
				           "." + detachNode->name + " because it's val");
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

	if (value->isNullable() && !detach->isNullable()) {
		throwError("Cannot detach '" + compile.classes[detach->classId]->name +
		           "' with '" + compile.classes[value->classId]->name + "?'");
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
	value->putBytecodes(in_data, bytecodes);
	detach->putBytecodes(in_data, bytecodes);
}

ExprNode *SetNode::copy(in_func) {
	return context.setValuePool.push(
	    line, static_cast<HasClassIdNode *>(detach->copy(in_data)),
	    static_cast<HasClassIdNode *>(value->copy(in_data)));
}

SetNode::~SetNode() {
	deleteNode(detach);
	deleteNode(value);
}

} // namespace AutoLang

#endif