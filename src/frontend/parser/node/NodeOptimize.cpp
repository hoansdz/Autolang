#ifndef NODE_OPTIMIZE_CPP
#define NODE_OPTIMIZE_CPP

#include "frontend/parser/node/NodeOptimize.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

ConstValueNode *UnaryNode::calculate(in_func) {
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
	if (value->kind != NodeType::CONST)
		return nullptr;
	auto value = static_cast<ConstValueNode *>(this->value);
	switch (op) {
		using namespace AutoLang;
	case Lexer::TokenType::PLUS: {
		switch (value->classId) {
		case AutoLang::DefaultClass::intClassId:
		case AutoLang::DefaultClass::floatClassId: {
			return value;
		}
		case AutoLang::DefaultClass::boolClassId: {
			value->classId = AutoLang::DefaultClass::intClassId;
			value->i = static_cast<int64_t>(value->obj->b);
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
			value->i = -value->i;
			return value;
		case AutoLang::DefaultClass::floatClassId:
			value->f = -value->f;
			return value;
		case AutoLang::DefaultClass::boolClassId:
			value->classId = AutoLang::DefaultClass::intClassId;
			value->i = static_cast<int64_t>(-value->obj->b);
			return value;
		default:
			break;
		}
		break;
	}
	case Lexer::TokenType::NOT: {
		if (value->classId == AutoLang::DefaultClass::boolClassId) {
			value->obj = ObjectManager::create(!value->obj->b);
			value->id = context.getBoolConstValuePosition(value->obj->b);
			return value;
		}
		// if (value->classId == AutoLang::DefaultClass::nullClassId)
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
	           compile.classes[value->classId].name);
	return nullptr;
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
			           compile.classes[value->classId].name + " to number");
		}
	}
	case Lexer::TokenType::NOT: {
		if (value->classId == DefaultClass::boolClassId)
			break;
		throwError("Cannot cast class " + compile.classes[value->classId].name +
		           " to Bool");
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
		id = compile.registerConstPool<int64_t>(compile.constIntMap, i);
		return;
	case AutoLang::DefaultClass::floatClassId:
		id = compile.registerConstPool<double>(compile.constFloatMap, f);
		return;
	default:
		if (classId != AutoLang::DefaultClass::stringClassId)
			break;
		id = compile.registerConstPool(compile.constStringMap,
		                               AString::from(*str));
		delete str;
		str = nullptr;
		return;
	}
}

HasClassIdNode *CastNode::createAndOptimize(in_func, HasClassIdNode *value,
                                            uint32_t classId) {
	if (value->classId == classId)
		return value;
	try {
		switch (value->kind) {
		case (NodeType::CONST): {
			auto node = static_cast<ConstValueNode *>(value);
			if (node->isNullable()) return value;
			switch (classId) {
			case AutoLang::DefaultClass::intClassId:
				toInt(node);
				return value;
			case AutoLang::DefaultClass::floatClassId:
				toFloat(node);
				return value;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	} catch (const std::runtime_error &err) {
		value->throwError("Cannot cast " +
		                  compile.classes[value->classId].name + " to " +
		                  compile.classes[classId].name);
	}
	return new CastNode(value, classId);
}

void BlockNode::optimize(in_func) {
	for (auto *node : nodes) {
		node->optimize(in_data);
	}
}

void UnknowNode::optimize(in_func) {
	auto it = compile.classMap.find(name);
	if (it == compile.classMap.end()) {
		if (contextCallClassId) {
			AClass *clazz = &compile.classes[*contextCallClassId];
			AClass *lastClass = context.getCurrentClass(in_data);
			context.gotoClass(clazz);
			correctNode = context.getCurrentClassInfo(in_data)->findDeclaration(
			    in_data, line, name);
			context.gotoClass(lastClass);
			if (correctNode) {
				static_cast<AccessNode *>(correctNode)->nullable = nullable;
				correctNode->optimize(in_data);
				classId = correctNode->classId;
				switch (correctNode->kind) {
				case NodeType::GET_PROP: {
					classId = static_cast<GetPropNode *>(correctNode)
					              ->declaration->classId;
					break;
				}
				case NodeType::VAR: {
					classId = static_cast<VarNode *>(correctNode)
					              ->declaration->classId;
					break;
				}
				}
				return;
			}
		}
		throwError("UnknowNode: Variable name: " + name +
		           " is not be declarated");
	}
	// Founded class
	classId = it->second;
	correctNode = new ClassAccessNode(line, classId);
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
	case NodeType::UNKNOW: {
		auto node = static_cast<UnknowNode *>(caller)->correctNode;
		if (node && node->kind == NodeType::VAR) {
			static_cast<VarNode *>(node)->isStore = false;
		}
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
	auto clazz = &compile.classes[caller->classId];
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
		printDebug("Class " + clazz->name + " GetProp: " + name + " " +
		           " has id: " + std::to_string(id) + " " +
		           std::to_string(classId) + " " +
		           compile.classes[classId].name);
	}
	if (nullable) {
		nullable = declaration->nullable;
	}
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

void WhileNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'");
	body.optimize(in_data);
}

void ForRangeNode::optimize(in_func) {
	detach->optimize(in_data);
	// if (detach->isVal)
	//	throwError("Cannot change because it's val");
	from->optimize(in_data);
	from = CastNode::createAndOptimize(in_data, from,
	                                   AutoLang::DefaultClass::intClassId);
	to->optimize(in_data);
	to = CastNode::createAndOptimize(in_data, to,
	                                 AutoLang::DefaultClass::intClassId);
	if (to->kind == NodeType::CONST) {
		static_cast<ConstValueNode *>(to)->isLoadPrimary = true;
	}
	body.optimize(in_data);
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

	auto detach = this->detach;
new_detach:;
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
				compile.classes[detachNode->caller->classId].memberId[detachNode->declaration->id] = detach->classId;
				// Marked non null won't run example val a! = 1
				if (detachNode->declaration->mustInferenceNullable) {
					auto valueNode = value;
				back:;
					switch (valueNode->kind) {
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
					case UNKNOW: {
						valueNode =
						    static_cast<UnknowNode *>(value)->correctNode;
						goto back;
					}
					}
				}
				// printDebug(std::string("SetNode: Declaration ") +
				// node->declaration->name + " is " +
				// compile.classes[value->classId].name);
			}
			detach->classId = value->classId;
		}
		if (detachNode->isVal) {
			throwError("Cannot change " +
			           compile.classes[detachNode->caller->classId].name + "." +
			           detachNode->name + " because it's val");
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
				throwError(
				    detachNode->declaration->name + " cannot use operator" +
				    Lexer::Token(0, op).toString(context) + " with null value");
			}
			return;
		}
		auto clazz = &compile.classes[detachNode->caller->classId];
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
					auto valueNode = value;
				back1:;
					switch (valueNode->kind) {
					case VAR:
					case GET_PROP: {
						node->declaration->nullable =
						    static_cast<AccessNode *>(value)->nullable;
						break;
					}
					case CALL: {
						node->declaration->nullable =
						    static_cast<CallNode *>(value)->nullable;
						break;
					}
					case UNKNOW: {
						valueNode =
						    static_cast<UnknowNode *>(value)->correctNode;
						goto back1;
					}
					}
				}
				// printDebug(std::string("SetNode: Declaration ") +
				// node->declaration->name + " is " +
				// compile.classes[value->classId].name);
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
				throwError(node->declaration->name + " cannot use operator" +
				           Lexer::Token(0, op).toString(context) +
				           " with null value");
			}
			return;
		}
		break;
	}
	case NodeType::UNKNOW: {
		detach = static_cast<UnknowNode *>(detach)->correctNode;
		goto new_detach;
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
		case NodeType::UNKNOW: {
			value = static_cast<UnknowNode *>(value)->correctNode;
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
			if (!detachNullable && static_cast<AccessNode *>(value)->nullable) {
				std::string detachName;
				if (detach->kind == NodeType::UNKNOW) {
					detachName = static_cast<UnknowNode *>(detach)->name;
				} else {
					detachName =
					    static_cast<AccessNode *>(detach)->declaration->name;
				}
				throwError("Cannot detach nullable variable " +
				           static_cast<AccessNode *>(value)->declaration->name +
				           " to nonnull variable " + detachName);
			}
			break;
		}
		case NodeType::CALL: {
			if (!detachNullable && static_cast<CallNode *>(value)->nullable) {
				std::string detachName;
				if (detach->kind == NodeType::UNKNOW) {
					detachName = static_cast<UnknowNode *>(detach)->name;
				} else {
					detachName =
					    static_cast<AccessNode *>(detach)->declaration->name;
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
				if (detach->classId == AutoLang::DefaultClass::stringClassId &&
				    op == Lexer::TokenType::PLUS_EQUAL)
					return;
				break;
			}
			throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
			           " operator with " +
			           compile.classes[detach->classId].name + " and " +
			           compile.classes[value->classId].name);
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
			throwError("Cannot cast " + compile.classes[value->classId].name +
			           " to " + compile.classes[detach->classId].name);
		}
	}
	switch (detach->kind) {
	case NodeType::VAR: {
		throwError(static_cast<VarNode *>(detach)->declaration->name +
		           " is declarated is " +
		           compile.classes[detach->classId].name);
	}
	case NodeType::GET_PROP: {
		auto detach_ = static_cast<GetPropNode *>(detach);
		throwError(compile.classes[detach_->caller->classId].name + +"." +
		           detach_->name + " is declarated is " +
		           compile.classes[detach->classId].name);
	}
	default:
		throwError(",Wtf");
	}
}

void CallNode::optimize(in_func) {
	AClass *clazz =
	    contextCallClassId ? &compile.classes[*contextCallClassId] : nullptr;
	std::string funcName;
	bool allowPrefix = false;
	if (caller) {
		// Caller.funcName() => Class.funcName()
		caller->optimize(in_data);
		if (caller->isNullable()) {
			if (!accessNullable)
				throwError(
				    "You can't use '.' with nullable value, you must use '?.'");
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
		}
		funcName = compile.classes[caller->classId].name + '.' + name;
		if (caller->kind == NodeType::CLASS) {
			ExprNode::deleteNode(caller);
			caller = nullptr;
		}
	} else {
		// Check if constructor
		auto it = compile.classMap.find(name.substr(0, name.length() - 2));
		if (it == compile.classMap.end()) {
			funcName = name;
			allowPrefix = clazz != nullptr;
		} else {
			isConstructor = true;
			// Return Id in putbytecode
			caller = new ClassAccessNode(line, it->second);
			funcName = compile.classes[it->second].name + '.' + name;
		}
	}
	for (auto &argument : arguments) {
		argument->optimize(in_data);
	}
	uint8_t count = 0;
	std::vector<uint32_t> *funcVec[2];
	// Find
	if (allowPrefix) {
		auto it = compile.funcMap.find(clazz->name + '.' + funcName);
		if (it != compile.funcMap.end()) {
			funcVec[count++] = &it->second;
		}
	}
	{
		auto it = compile.funcMap.find(funcName);
		if (it != compile.funcMap.end()) {
			funcVec[count++] = &it->second;
		}
	}
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
		std::string argumentsStr;
		bool isFirst = true;
		for (auto argument : arguments) {
			if (isFirst)
				isFirst = false;
			else
				argumentsStr += ", ";
			argumentsStr += compile.classes[argument->classId].name;
		}
		std::string detailFuncError =
		    funcName.insert(funcName.size() - 1, argumentsStr);
		throwError(std::string("Cannot find function name with arguments : ") +
		           detailFuncError);
	}
	if (ambitiousCall)
		throwError(std::string("Ambitious Call : ") + funcName);
	funcId = first.id;
	classId = first.func->returnId;
	auto func = &compile.functions[funcId];
	auto funcInfo = &context.functionInfo[funcId];

	nullable = func->returnNullable;

	if (first.errorNonNullIfMatch)
		throwError(std::string("Cannot input null in non null arguments ") +
		           funcName);
	if (funcInfo->accessModifier != Lexer::TokenType::PUBLIC &&
	    (!contextCallClassId || *contextCallClassId != funcInfo->clazz->id))
		throwError("Cannot access private function name '" + funcName + "'");
	// Add this
	if (allowPrefix && foundIndex == 0) {
		caller = new VarNode(line, context.classInfo[clazz->id].declarationThis,
		                     false, false);
		caller->optimize(in_data);
	}
	if (func->isStatic && caller) {
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
	if (caller &&
	    (caller->kind == NodeType::CLASS || caller->kind == NodeType::UNKNOW) &&
	    !isConstructor && !func->isStatic)
		throwError(func->name + " is not static function");
}

bool CallNode::match(in_func, MatchOverload &match,
                     std::vector<uint32_t> &functions, int &i) {
	match.score = 0;
	for (; i < functions.size(); ++i) {
		match.id = functions[i];
		match.func = &compile.functions[match.id];
		bool skip = false;
		if (!match.func->isStatic) {
			if (!caller || (!isConstructor && justFindStatic &&
			                (caller->kind == NodeType::CLASS ||
			                 caller->kind == NodeType::UNKNOW)))
				continue;
			if (caller->classId != match.func->args[0])
				continue;
			skip = true;
		}
		if (match.func->args.size != arguments.size() + skip)
			continue;
		bool matched = true;
		match.errorNonNullIfMatch = false;
		for (int j = 0; j < arguments.size(); ++j) {
			uint32_t inputClassId = arguments[j]->classId;
			uint32_t funcArgClassId = match.func->args[j + skip];
			// printDebug(compile.classes[inputClassId].name + " and " +
			// compile.classes[funcArgClassId].name);
			if (funcArgClassId != inputClassId) {
				if (funcArgClassId == AutoLang::DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				if (inputClassId == AutoLang::DefaultClass::nullClassId) {
					++match.score;
					if (!match.errorNonNullIfMatch)
						match.errorNonNullIfMatch =
						    !match.func->nullableArgs[j + skip];
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

void ReturnNode::optimize(in_func) {
	auto func = &compile.functions[funcId];
	if (value) {
		if (func->returnId == AutoLang::DefaultClass::nullClassId) {
			throwError("Cannot return value, function return Void");
		}
		value->optimize(in_data);
		if (!func->returnNullable) {
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
		if (value->classId == func->returnId)
			return;
		value = CastNode::createAndOptimize(in_data, value, func->returnId);
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