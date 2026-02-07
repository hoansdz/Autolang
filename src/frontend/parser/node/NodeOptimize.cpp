#ifndef NODE_OPTIMIZE_CPP
#define NODE_OPTIMIZE_CPP

#include "frontend/parser/node/NodeOptimize.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"

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
	value->mode = mode;
	switch (value->kind) {
		case NodeType::CONST: {
			auto value = static_cast<ConstValueNode *>(this->value);
			switch (op) {
				using namespace AutoLang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId: {
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return value;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(value->obj->b);
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return value;
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
							return value;
						}
						case AutoLang::DefaultClass::floatClassId: {
							value->f = -value->f;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return value;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(-value->obj->b);
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return value;
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
						return value;
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
							return value;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return value;
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

void ConstValueNode::optimize(in_func) {
	if (id != UINT32_MAX)
		return;
	switch (classId) {
		case AutoLang::DefaultClass::intClassId:
			id = compile.registerConstPool<int64_t>(context.constIntMap, i);
			return;
		case AutoLang::DefaultClass::floatClassId:
			id = compile.registerConstPool<double>(context.constFloatMap, f);
			return;
		default:
			if (classId != AutoLang::DefaultClass::stringClassId)
				break;
			id = compile.registerConstPool(context.constStringMap,
			                               AString::from(*str));
			delete str;
			str = nullptr;
			return;
	}
}

ExprNode *UnknowNode::resolve(in_func) {
	auto it = compile.classMap.find(name);
	if (it == compile.classMap.end()) {
		if (contextCallClassId) {
			AClass *clazz = compile.classes[*contextCallClassId];
			AClass *lastClass = context.getCurrentClass(in_data);
			context.gotoClass(clazz);
			auto correctNode =
			    context.getCurrentClassInfo(in_data)->findDeclaration(
			        in_data, line, name);
			context.gotoClass(lastClass);
			if (correctNode) {
				static_cast<AccessNode *>(correctNode)->nullable = nullable;
				correctNode->mode = mode;
				ExprNode::deleteNode(this);
				return correctNode;
			}
		}
		throwError("UnknowNode: Variable name: " + name +
		           " is not be declarated");
	}
	// Founded class
	auto result = new ClassAccessNode(line, it->second);
	ExprNode::deleteNode(this);
	return result;
}

ExprNode *GetPropNode::resolve(in_func) {
	caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	caller->mode = mode;
	return this;
}

void GetPropNode::optimize(in_func) {
	caller->optimize(in_data);
	if (caller->isNullable()) {
		if (!accessNullable)
			throwError(
			    "You can't use '.' with nullable valuea, you must use '?.'");
	} else {
		if (accessNullable) {
			warning(in_data,
			        "You should use '.' with non null value instead of '?.'");
			accessNullable = false;
		}
	}
	switch (caller->kind) {
		case NodeType::CALL:
		case NodeType::GET_PROP: {
			break;
		}
		case NodeType::VAR: {
			static_cast<VarNode *>(caller)->isStore = false;
			break;
		}
		default: {
			throwError("Cannot find caller");
		}
	}
	auto clazz = compile.classes[caller->classId];
	auto classInfo = &context.classInfo[clazz->id];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		// Find static member
		auto it_ = classInfo->staticMember.find(name);
		if (it_ == classInfo->staticMember.end())
			throwError("Cannot find member name: " + name);
		declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private -a member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
	}
	if (!isStatic) {
		// a.a = ...
		declaration = classInfo->member[it->second];
		isVal = !isInitial && declaration->isVal;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private member -b name '" + name + "'");
		}
		id = it->second;
		// for (int i = 0; i<clazz->memberId.size(); ++i) {
		// 	printDebug("MemId: "+std::to_string(clazz->memberId[i]));
		// }
		// printDebug("Class " + clazz->name + " GetProp: "+name+" "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		if (clazz->memberId[id] != declaration->classId)
			clazz->memberId[id] = declaration->classId;
		classId = declaration->classId; // clazz->memberId[id];
		// printDebug("Class " + clazz->name + " GetProp: " + name + " " +
		//            " has id: " + std::to_string(id) + " " +
		//            std::to_string(classId) + " " +
		//            compile.classes[classId]->name);
	}
	if (nullable) {
		nullable = declaration->nullable;
	}
}

ExprNode *IfNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	condition->mode = mode;
	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (static_cast<ConstValueNode *>(condition)->obj->b) {
			if (ifFalse) {
				warning(in_data, "Else body will never be used");
			}
			auto result = new BlockNode(ifTrue);
			result->resolve(in_data);
			result->mode = mode;
			ifTrue.nodes.clear();
			ExprNode::deleteNode(this);
			return result;
		} else if (ifFalse) {
			auto result = ifFalse;
			result->resolve(in_data);
			result->mode = mode;
			ifFalse = nullptr;
			ExprNode::deleteNode(this);
			return result;
		}
		return this;
	}
	ifTrue.resolve(in_data);
	ifTrue.mode = mode;
	if (ifFalse) {
		ifFalse->resolve(in_data);
		ifFalse->mode = mode;
	}
	return this;
}

void IfNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'");
	ifTrue.optimize(in_data);
	if (ifFalse)
		ifFalse->optimize(in_data);
}

ExprNode *WhileNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	condition->mode = mode;
	body.resolve(in_data);
	return this;
}

void WhileNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'");
	body.optimize(in_data);
}

ExprNode *ForRangeNode::resolve(in_func) {
	detach = static_cast<AccessNode *>(detach->resolve(in_data));
	from = static_cast<HasClassIdNode *>(from->resolve(in_data));
	to = static_cast<HasClassIdNode *>(to->resolve(in_data));
	body.resolve(in_data);
	detach->mode = mode;
	from->mode = mode;
	to->mode = mode;
	return this;
}

void ForRangeNode::optimize(in_func) {
	detach->optimize(in_data);
	switch (detach->classId) {
		case AutoLang::DefaultClass::intClassId: {
			break;
		}
		default: {
			throwError("Detach value must be Int");
		}
	}
	// if (detach->isVal)
	// 	throwError("Cannot change because it's val");
	from->optimize(in_data);
	switch (from->classId) {
		case AutoLang::DefaultClass::intClassId: {
			break;
		}
		default: {
			throwError("From value must be Int");
		}
	}
	to->optimize(in_data);
	switch (to->classId) {
		case AutoLang::DefaultClass::intClassId: {
			break;
		}
		default: {
			throwError("To value must be Int");
		}
	}
	if (to->kind == NodeType::CONST) {
		static_cast<ConstValueNode *>(to)->isLoadPrimary = true;
	}
	body.optimize(in_data);
}

ExprNode *SetNode::resolve(in_func) {
	detach = static_cast<HasClassIdNode *>(detach->resolve(in_data));
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	detach->mode = mode;
	value->mode = mode;
	return this;
}

void SetNode::optimize(in_func) {
	// Detach has nullClassId because it was not evaluated
	detach->optimize(in_data);
	value->optimize(in_data);

	if (value->isNullable() && op != Lexer::TokenType::EQUAL) {
		throwError("Cannot use operator '" +
		           Lexer::Token(0, op).toString(context) +
		           "' with nullable variables");
	}

	classId = value->classId;

	bool detachNullable;

	switch (detach->kind) {
		case NodeType::GET_PROP: {
			if (detach->classId != AutoLang::DefaultClass::nullClassId &&
			    detach->classId != value->classId) {
				detach->classId = value->classId;
			}
			auto detachNode = static_cast<GetPropNode *>(detach);
			detachNullable = detachNode->nullable;
			detachNode->isStore = true;
			if (detachNode->classId == AutoLang::DefaultClass::nullClassId) {
				if (detachNode->declaration->classId ==
				    AutoLang::DefaultClass::nullClassId) {
					if (value->classId == AutoLang::DefaultClass::nullClassId) {
						throwError("Ambigous inference member class id");
					}
					detachNode->declaration->classId = value->classId;
					compile.classes[detachNode->caller->classId]
					    ->memberId[detachNode->declaration->id] =
					    detach->classId;
					// Marked non null won't run example val a! = 1
					if (detachNode->declaration->mustInferenceNullable) {
						switch (value->kind) {
							case VAR:
							case GET_PROP: {
								detachNode->declaration->nullable =
								    static_cast<AccessNode *>(value)->nullable;
								break;
							}
							case CALL: {
								detachNode->declaration->nullable =
								    static_cast<CallNode *>(value)->nullable;
								break;
							}
						}
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
					throwError(detachNode->declaration->name +
					           " cannot detach null value, you must declare " +
					           detachNode->declaration->className +
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
						// std::cerr << "Set " << node->declaration->name << " is "
						//           << (detachNullable ? "nullable" : "nonnull")
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
					throwError(node->declaration->name +
					           " cannot detach null value, you must declare " +
					           node->declaration->className +
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
					if (detach->kind == NodeType::UNKNOW) {
						detachName = static_cast<UnknowNode *>(detach)->name;
					} else {
						detachName = static_cast<AccessNode *>(detach)
						                 ->declaration->name;
					}
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
					if (detach->kind == NodeType::UNKNOW) {
						detachName = static_cast<UnknowNode *>(detach)->name;
					} else {
						detachName = static_cast<AccessNode *>(detach)
						                 ->declaration->name;
					}
					throwError("Cannot detach nullable value " +
					           static_cast<CallNode *>(value)->name +
					           " to nonnull variable " + detachName);
				}
				break;
			}
			default:
				break;
		}
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
			value = new CastNode(value, detach->classId);
			return;
		}
		// Optimize
		try {
			switch (detach->classId) {
				case AutoLang::DefaultClass::intClassId:
					toInt(static_cast<ConstValueNode *>(value));
					return;
				case AutoLang::DefaultClass::floatClassId:
					toFloat(static_cast<ConstValueNode *>(value));
					return;
				default:
					throwError("What happen");
			}
		} catch (const std::runtime_error &err) {
			throwError("Cannot cast " + compile.classes[value->classId]->name +
			           " to " + compile.classes[detach->classId]->name);
		}
	}
	switch (detach->kind) {
		case NodeType::VAR: {
			throwError(static_cast<VarNode *>(detach)->declaration->name +
			           " is declarated is " +
			           compile.classes[detach->classId]->name);
		}
		case NodeType::GET_PROP: {
			auto detach_ = static_cast<GetPropNode *>(detach);
			throwError(compile.classes[detach_->caller->classId]->name + +"." +
			           detach_->name + " is declarated is " +
			           compile.classes[detach->classId]->name);
		}
		default:
			throwError(",Wtf");
	}
}

ExprNode *CallNode::resolve(in_func) {
	if (caller) {
		caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
		caller->mode = mode;
	}
	for (auto &argument : arguments) {
		argument = static_cast<HasClassIdNode *>(argument->resolve(in_data));
		argument->mode = mode;
	}
	return this;
}

void CallNode::optimize(in_func) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	std::string funcName;
	ClassId callerCanCallId; // never be used if is static
	uint8_t count = 0;
	std::vector<uint32_t> *funcVec[2];

	for (auto &argument : arguments) {
		argument->optimize(in_data);
	}

	if (caller) {
		// Caller.funcName() => Class.funcName()
		caller->optimize(in_data);
		if (caller->isNullable()) {
			// switch (caller->kind) {
			// 	case NodeType::GET_PROP: {
			// 		std::cerr<<"GET_PROP\n";
			// 		break;
			// 	}
			// 	case NodeType::UNKNOW: {
			// 		std::cerr<<"UNKNOW\n";
			// 		break;
			// 	}
			// 	case NodeType::CALL: {
			// 		std::cerr<<"CALL\n";
			// 		break;
			// 	}
			// }
			if (!accessNullable) {
				throwError("You can't use '.' with nullable valueb, you must "
				           "use '?.'");
			}
		} else {
			if (accessNullable) {
				warning(
				    in_data,
				    "You should use '.' with non null value instead of '?.'");
				accessNullable = false;
			}
		}

		switch (caller->kind) {
			case NodeType::VAR: {
				auto node = static_cast<VarNode *>(caller);
				node->isStore = false;
				node->classId = node->declaration->classId;
				break;
			}
			case NodeType::GET_PROP:
				break;
			case NodeType::CLASS_ACCESS:
				justFindStatic = true;
				break;
		}

		auto callerClass = compile.classes[caller->classId];
		funcName = callerClass->name + "." + name;
		auto it = callerClass->funcMap.find(name);
		if (it != callerClass->funcMap.end()) {
			funcVec[count++] = &it->second;
			callerCanCallId = caller->classId;
		}

	} else {
		// Check if constructor
		auto it = compile.classMap.find(name.substr(0, name.length() - 2));
		if (it == compile.classMap.end()) {
			funcName = name;
			if (contextCallClassId) {
				auto callerClass = compile.classes[*contextCallClassId];
				auto it = callerClass->funcMap.find(name);
				if (it != callerClass->funcMap.end()) {
					funcVec[count++] = &it->second;
					callerCanCallId = *contextCallClassId;
				}
			}
			// allowPrefix = clazz != nullptr;
		} else {
			// Return Id in putbytecode
			funcName = compile.classes[it->second]->name + '.' + name;
			caller = new ClassAccessNode(line, it->second);
		}

		{
			auto it = compile.funcMap.find(funcName);
			if (it != compile.funcMap.end()) {
				funcVec[count++] = &it->second;
			}
		}
	}
	// Find
	// if (allowPrefix) {
	// 	auto it = compile.funcMap.find(clazz->name + '.' + funcName);
	// 	if (it != compile.funcMap.end()) {
	// 		funcVec[count++] = &it->second;
	// 	}
	// }
	if (count == 0)
		throwError(std::string("Cannot find function name : ") + funcName);
	bool ambitiousCall = false;
	uint8_t foundIndex;
	bool found = false;
	MatchOverload first;
	MatchOverload second;
	int i = 0;
	int j = 0;
	// Find first function
	for (; j < count; ++j) {
		if (!match(in_data, first, *funcVec[j], i)) {
			i = 0;
			continue;
		}
		found = true;
		foundIndex = j;
		break;
	} // Find function
	for (; j < count; ++j) {
		std::vector<uint32_t> *vec = funcVec[j];
		while (match(in_data, second, *vec, i)) {
			if (second.score < first.score)
				continue;
			if (second.score == first.score) {
				ambitiousCall = true;
				continue;
			}
			foundIndex = j;
			ambitiousCall = false;
			first = second;
		}
		i = 0;
	}
	if (!found) {
		std::string currentFuncLog = funcName.substr(0, funcName.length() - 1);
		bool isFirst = true;
		for (auto argument : arguments) {
			if (isFirst)
				isFirst = false;
			else
				currentFuncLog += ", ";
			currentFuncLog += compile.classes[argument->classId]->name;
		}
		for (int j = 0; j < count; ++j) {
			auto &vecs = *funcVec[j];
			if (vecs.empty())
				printDebug("Empty");
			for (auto v : vecs) {
				printDebug("Founded " +
				           compile.functions[v]->toString(compile));
			}
		}
		throwError(std::string("Cannot find function has arguments : ") +
		           currentFuncLog + ")");
	}
	if (ambitiousCall) {
		for (int j = 0; j < count; ++j) {
			auto &vecs = *funcVec[j];
			for (auto v : vecs) {
				printDebug("Founded " +
				           compile.functions[v]->toString(compile));
			}
		}
		throwError(std::string("Ambiguous Call : ") + funcName);
	}
	funcId = first.id;
	classId = first.func->returnId;
	auto func = compile.functions[funcId];
	auto funcInfo = &context.functionInfo[funcId];

	nullable = func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;

	if (first.errorNonNullIfMatch)
		throwError(std::string("Cannot input null in non null arguments ") +
		           funcName);
	if (!(func->functionFlags & FunctionFlags::FUNC_PUBLIC) &&
	    (!contextCallClassId || *contextCallClassId != funcInfo->clazz->id))
		throwError("Cannot access private function name '" + funcName + "'");
	// Add this
	if (!caller && !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		caller = new VarNode(line,
		                     context.classInfo[callerCanCallId].declarationThis,
		                     false, false);
		caller->optimize(in_data);
	}
	if ((func->functionFlags & FunctionFlags::FUNC_IS_STATIC) && caller) {
		switch (caller->kind) {
			case NodeType::VAR: {
				bool callerClassId = caller->classId;
				ExprNode::deleteNode(caller);
				caller = new ClassAccessNode(line, callerClassId);
				break;
			}
			case NodeType::GET_PROP: {
				auto newCaller = static_cast<GetPropNode *>(caller)->caller;
				static_cast<GetPropNode *>(caller)->caller = nullptr;
				ExprNode::deleteNode(caller);
				caller = newCaller;
				// addPopBytecode = true;
				break;
			}
			case NodeType::CALL: {
				auto newCaller = static_cast<CallNode *>(caller)->caller;
				static_cast<CallNode *>(caller)->caller = nullptr;
				ExprNode::deleteNode(caller);
				caller = newCaller;
				// addPopBytecode = true;
				break;
			}
			default:
				break;
		}
	}
	if (caller && caller->kind == NodeType::CLASS_ACCESS &&
	    !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC) &&
	    !(func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR))
		throwError(func->name + " is not static function");
}

bool CallNode::match(in_func, MatchOverload &match,
                     std::vector<uint32_t> &functions, int &i) {
	match.score = 0;
	for (; i < functions.size(); ++i) {
		match.id = functions[i];
		match.func = compile.functions[match.id];
		auto funcInfo = &context.functionInfo[match.id];
		bool skip = false;
		if (!(match.func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
			if (justFindStatic)
				continue;
			skip = true;
		}
		if (match.func->argSize != arguments.size() + skip)
			continue;
		bool matched = true;
		match.errorNonNullIfMatch = false;
		for (int j = 0; j < arguments.size(); ++j) {
			uint32_t inputClassId = arguments[j]->classId;
			uint32_t funcArgClassId = match.func->args[j + skip];
			// printDebug(compile.classes[inputClassId]->name + " and " +
			// compile.classes[funcArgClassId]->name);
			if (funcArgClassId != inputClassId) {
				if (funcArgClassId == AutoLang::DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				if (inputClassId == AutoLang::DefaultClass::nullClassId) {
					++match.score;
					if (!match.errorNonNullIfMatch)
						match.errorNonNullIfMatch =
						    !funcInfo->nullableArgs[j + skip];
					continue;
				} else if (inputClassId == AutoLang::DefaultClass::intClassId &&
				           funcArgClassId ==
				               AutoLang::DefaultClass::floatClassId) {
					++match.score;
					continue;
				}
				matched = false;
				break;
			}
			match.score += 2;
		}
		if (matched) {
			++i;
			return true;
		}
	}
	return false;
}

ExprNode *ReturnNode::resolve(in_func) {
	if (value) {
		auto func = compile.functions[funcId];
		if (func->returnId == AutoLang::DefaultClass::nullClassId) {
			throwError("Cannot return value, function return Void");
		}
		if (value->classId == func->returnId)
			return this;
		if (value->classId == DefaultClass::nullClassId)
			return this;
		auto castNode = new CastNode(value, func->returnId);
		castNode->mode = mode;
		value = static_cast<HasClassIdNode *>(castNode->resolve(in_data));
		value->mode = mode;
	}
	return this;
}

void ReturnNode::optimize(in_func) {
	auto func = compile.functions[funcId];
	if (value) {
		value->optimize(in_data);
		if (!(func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE)) {
			if (value->classId == AutoLang::DefaultClass::nullClassId) {
				throwError("Cannot return null because functions returns "
				           "nonnull value");
			}
			if (value->isNullable()) {
				throwError("Cannot return nullable variable because functions "
				           "returns nonnull value");
			}
			return;
		}
		return;
	}
	if (func->returnId != AutoLang::DefaultClass::nullClassId) {
		throwError("Must return value");
	}
}

void VarNode::optimize(in_func) {
	classId = declaration->classId;
	isVal = declaration->isVal;
	if (nullable) {
		nullable = declaration->nullable; // #
	}
}

} // namespace AutoLang

#endif