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
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(value->obj->b);
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
		case AutoLang::DefaultClass::stringClassId:
			id = compile.registerConstPool(context.constStringMap,
			                               AString::from(*str));
			return;
		default:
			break;
	}
}

ExprNode *ConstValueNode::copy(in_func) {
	switch (classId) {
		case DefaultClass::intClassId:
			return context.constValuePool.push(line, i);
		case DefaultClass::floatClassId:
			return context.constValuePool.push(line, f);
		case DefaultClass::boolClassId:
			return context.constValuePool.push(line, bool(obj->b));
		case DefaultClass::stringClassId:
			return context.constValuePool.push(line, *str);
		default:
			return context.constValuePool.push(line, obj, id);
	}
}

ExprNode *UnknowNode::resolve(in_func) {
	auto it = context.defaultClassMap.find(nameId);
	if (it == context.defaultClassMap.end()) {
		if (contextCallClassId) {
			AClass *clazz = compile.classes[*contextCallClassId];
			AClass *lastClass = context.getCurrentClass(in_data);
			context.gotoClass(clazz);
			auto correctNode =
			    context.getCurrentClassInfo(in_data)->findDeclaration(
			        in_data, line, context.lexerString[nameId]);
			context.gotoClass(lastClass);
			if (correctNode) {
				static_cast<AccessNode *>(correctNode)->nullable = nullable;
				ExprNode::deleteNode(this);
				return correctNode;
			}
		}
		throwError("UnknowNode: Variable name: " + context.lexerString[nameId] +
		           " is not be declarated");
	}
	// Founded class
	auto result = context.classAccessPool.push(line, it->second);
	ExprNode::deleteNode(this);
	return result;
}

ExprNode *UnknowNode::copy(in_func) {
	if (contextCallClassId) {
		auto classInfo = context.classInfo[*contextCallClassId];
		auto genericDeclaration = classInfo->findGenericDeclaration(nameId);
		if (genericDeclaration) {
			return context.classAccessPool.push(line,
			                                    genericDeclaration->classId);
		}
	}
	return context.unknowNodePool.push(line, context.currentClassId, nameId,
	                                   nullable);
}

void UnknowNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	throwError("Unknow node can't putbytecodes");
}

ExprNode *GetPropNode::resolve(in_func) {
	caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	if (caller->kind == NodeType::CLASS_ACCESS) {
		auto *classInfo = context.classInfo[caller->classId];
		auto it = classInfo->staticMember.find(name);
		if (it == classInfo->staticMember.end()) {
			throwError("Cannot find static member name: '" + name + "'");
		}
		auto declarationNode = it->second;
		ExprNode::deleteNode(caller);
		return context.varPool.push(line, declarationNode, isStore, nullable);
	}
	return this;
}

void GetPropNode::optimize(in_func) {
	caller->optimize(in_data);
	if (caller->isNullable()) {
		if (!accessNullable)
			throwError(
			    "You can't use '.' with nullable value, you must use '?.'");
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
	auto classInfo = context.classInfo[clazz->id];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		// Find static member
		auto it_ = classInfo->staticMember.find(name);
		if (it_ == classInfo->staticMember.end())
			throwError("Cannot find member name '" + name + "' in class " +
			           clazz->name);
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
			throwError("Cannot access private member name '" + name + "'");
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

ExprNode *GetPropNode::copy(in_func) {
	auto newNode = context.getPropPool.push(
	    line,
	    declaration ? static_cast<DeclarationNode *>(declaration->copy(in_data))
	                : nullptr,
	    contextCallClassId,
	    caller ? static_cast<HasClassIdNode *>(caller->copy(in_data)) : nullptr,
	    name, isInitial, nullable, accessNullable);
	newNode->isStore = isStore;
	return newNode;
}

ExprNode *WhileNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
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

ExprNode *WhileNode::copy(in_func) {
	auto newNode = context.whilePool.push(line);
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto node : body.nodes) {
		newNode->body.nodes.push_back(node->copy(in_data));
	}
	return newNode;
}

ExprNode *SetNode::resolve(in_func) {
	detach = static_cast<HasClassIdNode *>(detach->resolve(in_data));
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	if (detach->kind == NodeType::CALL) {
		auto result = static_cast<CallNode *>(detach);
		if (result->name != "[]")
			return this;
		result->name = "set()";
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

ExprNode *SetNode::copy(in_func) {
	return context.setValuePool.push(
	    line, static_cast<HasClassIdNode *>(detach->copy(in_data)),
	    static_cast<HasClassIdNode *>(value->copy(in_data)));
}

ExprNode *ReturnNode::resolve(in_func) {
	if (loaded) {
		return this;
	}
	if (value) {
		auto func = compile.functions[funcId];
		if (func->returnId == DefaultClass::voidClassId) {
			throwError("Cannot return value, function return Void");
		}
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
		if (func->returnId == DefaultClass::nullClassId) {
			return this;
		}
		if (value->classId == func->returnId) {
			return this;
		}
		if (value->classId == DefaultClass::nullClassId) {
			return this;
		}
		value = context.castPool.push(value, func->returnId);
		// auto castNode = context.castPool.push(value, func->returnId);
		// value = static_cast<HasClassIdNode *>(castNode->resolve(in_data));
	}
	return this;
}

void ReturnNode::optimize(in_func) {
	if (loaded) {
		return;
	}
	auto func = compile.functions[funcId];
	// std::cerr<<"Loading "<<func->name<<"\n";
	if (value) {
		value->optimize(in_data);
		// Marks auto
		switch (func->returnId) {
			case DefaultClass::anyClassId: {
				return;
			}
			case DefaultClass::nullClassId: {
				// std::cerr << "Loaded " << func->name << "\n";
				func->returnId = value->classId;
				if (value->isNullable()) {
					func->functionFlags |= FunctionFlags::FUNC_RETURN_NULLABLE;
				}
				break;
			}
		}
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
	if (func->returnId != AutoLang::DefaultClass::voidClassId) {
		throwError("Must return value");
	}
}

ExprNode *ReturnNode::copy(in_func) {
	return context.returnPool.push(
	    line, context.currentFunctionId,
	    value ? static_cast<HasClassIdNode *>(value->copy(in_data)) : nullptr);
}

void VarNode::optimize(in_func) {
	// std::cerr << "loaded " << declaration->name << " "
	//           << compile.classes[declaration->classId]->name << "\n";
	classId = declaration->classId;
	isVal = declaration->isVal;
	if (nullable) {
		nullable = declaration->nullable; // #
	}
}

ExprNode *VarNode::copy(in_func) {
	return context.varPool.push(
	    line, static_cast<DeclarationNode *>(declaration->copy(in_data)),
	    isStore, nullable);
}

} // namespace AutoLang

#endif