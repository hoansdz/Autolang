#ifndef NODE_OPTIMIZE_CPP
#define NODE_OPTIMIZE_CPP

#include "NodeOptimize.hpp"
#include "CreateNode.hpp"
#include "Debugger.hpp"

namespace AutoLang {

ConstValueNode* BinaryNode::calculate(in_func) {
	//std::cout<<"op "<<this<<":"<<Lexer::Token(0, op, "").toString(context)<<'\n';
	ConstValueNode* l;
	switch (left->kind) {
		case NodeType::CONST:
			l = static_cast<ConstValueNode*>(left);
			break;
		case NodeType::BINARY:
			l = static_cast<BinaryNode*>(left)->calculate(in_data);
			if (l == nullptr) return nullptr;
			delete left;
			left = l;
			break;
		default:
			return nullptr;
	}
	ConstValueNode* r;
	switch (right->kind) {
		case NodeType::CONST:
			r = static_cast<ConstValueNode*>(right);
			break;
		case NodeType::BINARY:
			r = static_cast<BinaryNode*>(right)->calculate(in_data);
			if (r == nullptr) return nullptr;
			delete right;
			right = r;
			break;
		default:
			return nullptr;
	}
	#define optimizeNode(token, func) case Lexer::TokenType::token: return func(l, r);
	try {
		switch (op) {
			using namespace AutoLang;
			optimizeNode(PLUS, plus);
			optimizeNode(MINUS, minus);
			optimizeNode(STAR, mul);
			optimizeNode(SLASH, divide)
			optimizeNode(PERCENT, mod)
			optimizeNode(EQEQ, op_eqeq)
			optimizeNode(NOTEQ, op_not_eq)
			optimizeNode(LTE, op_less_than_eq)
			optimizeNode(GTE, op_greater_than_eq)
			optimizeNode(LT, op_less_than)
			optimizeNode(GT, op_greater_than)
			case Lexer::TokenType::NOTEQEQ:
			case Lexer::TokenType::EQEQEQ: {
				const bool result = op == Lexer::TokenType::EQEQEQ;
				if (l->classId == AutoLang::DefaultClass::boolClassId) {
					if (r->classId == AutoLang::DefaultClass::boolClassId) {
						const bool equal = (l->obj->b == r->obj->b);
						return new ConstValueNode(result ? equal : !equal);
					} else 
					if (r->classId == AutoLang::DefaultClass::nullClassId)
						return new ConstValueNode(!result);
				} else
				if (l->classId == AutoLang::DefaultClass::nullClassId) {
					if (r->classId == AutoLang::DefaultClass::nullClassId) {
						return new ConstValueNode(result);
					} else 
					if (r->classId == AutoLang::DefaultClass::boolClassId)
						return new ConstValueNode(!result);
				}
				throw std::runtime_error("");
			}
			case Lexer::TokenType::AND_AND:
				return new ConstValueNode(l->obj->b && r->obj->b);
			case Lexer::TokenType::OR_OR:
				return new ConstValueNode(l->obj->b || r->obj->b);
			default:
				throw std::runtime_error("");
		}
	}
	catch (const std::runtime_error& err) {
		throw std::runtime_error("Cannot use "+Lexer::Token(0, op).toString(context)+" operator with "+
		compile.classes[l->classId].name+" and "+compile.classes[r->classId].name);
	}
}

void BinaryNode::optimize(in_func) {
	left->optimize(in_data);
	right->optimize(in_data);
	if (left->kind == NodeType::CONST)
		static_cast<ConstValueNode*>(left)->isLoadPrimary= true;
	if (right->kind == NodeType::CONST)
		static_cast<ConstValueNode*>(right)->isLoadPrimary = true;
	switch (op) {
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS:
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::SLASH: {
			
			if (left->classId == AutoLang::DefaultClass::boolClassId) {
				left = CastNode::createAndOptimize(in_data, left, AutoLang::DefaultClass::INTCLASSID);
			}
			//std::cout<<compile.classes[left->classId].name<<std::endl;
			
			if (right->classId == AutoLang::DefaultClass::boolClassId) {
				right = CastNode::createAndOptimize(in_data, right, AutoLang::DefaultClass::INTCLASSID);
			}
			break;
		}
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::EQEQEQ: {
			return;
		}
		default:
			break;
	}
	if (compile.getTypeResult(left->classId, right->classId, static_cast<uint8_t>(op), classId)) return;
	throw std::runtime_error(std::string("Cannot find use '")+Lexer::Token(0, op).toString(context)+"' between "+
	compile.classes[left->classId].name+" and "+compile.classes[right->classId].name);
}

ConstValueNode* UnaryNode::calculate(in_func) {
	if (value->kind == NodeType::UNARY) {
		auto node = static_cast<UnaryNode*>(value);
		if (node == nullptr || node->op != op) return nullptr;
		value = node->value;
		node->value = nullptr;
		delete node;
	}
	/*if (value->kind == NodeType::BINARY) {
		auto node = static_cast<BinaryNode*>(value);
		if (node == nullptr) return nullptr;
		delete value;
		value = node;
	}*/
	if (value->kind != NodeType::CONST) return nullptr;
	auto value = static_cast<ConstValueNode*>(this->value);
	switch (op) {
		using namespace AutoLang;
		case Lexer::TokenType::MINUS: {
			switch (value->classId) {
				case AutoLang::DefaultClass::INTCLASSID:
					value->i = -value->i;
					return value;
				case AutoLang::DefaultClass::FLOATCLASSID:
					value->f = -value->f;
					return value;
				default:
					if (value->classId == AutoLang::DefaultClass::boolClassId) {
						value->classId = AutoLang::DefaultClass::INTCLASSID;
						value->i = static_cast<long long>(-value->obj->b);
						return value;
					}
					break;
			}
			break;
		}
		case Lexer::TokenType::NOT: {
			if (value->classId == AutoLang::DefaultClass::boolClassId) {
				value->obj = ObjectManager::create(!value->obj->b);
				value->id = value->obj->b ? 1 : 2;
				return value;
			}
			if (value->classId == AutoLang::DefaultClass::nullClassId) {
				value->classId = AutoLang::DefaultClass::boolClassId;
				value->obj = ObjectManager::create(true);
				value->id = 1;
				return value;
			}
			break;
		}
		default:
			break;
	}
	throw std::runtime_error("Cannot find operator '"+Lexer::Token(0, op).toString(context)+"'");
}

void UnaryNode::optimize(in_func) {
	if (value->kind == NodeType::CONST)
		static_cast<ConstValueNode*>(value)->isLoadPrimary= true;
	value->optimize(in_data);
	classId = value->classId;
}

void ConstValueNode::optimize(in_func) {
	if (id != UINT32_MAX) return;
	switch (classId) {
		case AutoLang::DefaultClass::INTCLASSID:
			id = compile.registerConstPool(compile.manager.create(i));
			return;
		case AutoLang::DefaultClass::FLOATCLASSID:
			id = compile.registerConstPool(compile.manager.create(f));
			return;
		default:
			if (classId != AutoLang::DefaultClass::stringClassId)
				break;
			id = compile.registerConstPool(compile.manager.create(AString::from(*str)));
			delete str;
			return;
	}
}

HasClassIdNode* CastNode::createAndOptimize(in_func, HasClassIdNode* value, uint32_t classId) {
	if (value->classId == classId)
		return value;
	try {
		switch (value->kind) {
			case (NodeType::CONST): {
				switch (classId) {
					case AutoLang::DefaultClass::INTCLASSID:
						toInt(static_cast<ConstValueNode*>(value));
						return value;
					case AutoLang::DefaultClass::FLOATCLASSID:
						toFloat(static_cast<ConstValueNode*>(value));
						return value;
					default:
						break;
				}
				break;
			}
			default:
				break;
		}
	}
	catch (const std::runtime_error& err) {
		throw std::runtime_error("Cannot cast "+compile.classes[value->classId].name+" to "+compile.classes[classId].name);
	}
	return new CastNode(value, classId);
}

void BlockNode::optimize(in_func) {
	for (auto* node : nodes) {
		node->optimize(in_data);
	}
}

void UnknowNode::optimize(in_func) {
	auto it = compile.classMap.find(name);
	if (it == compile.classMap.end()) {
		if (clazz) {
			AClass* lastClass = context.currentClass;
			context.gotoClass(clazz);
			correctNode = context.currentClassInfo->findDeclaration(in_data, name);
			context.gotoClass(lastClass);
			if (correctNode) {
				correctNode->optimize(in_data);
				classId = correctNode->classId;
				switch (correctNode->kind) {
					case NodeType::GET_PROP: {
						classId = static_cast<GetPropNode*>(correctNode)->declaration->classId;
						break;
					}
					case NodeType::VAR: {
						classId = static_cast<VarNode*>(correctNode)->declaration->classId;
						break;
					}
				}
				return;
			}
		}
		throw std::runtime_error("UnknowNode: Cannot find class name: "+name);
	} else {

	}
	classId = it->second;
}

void GetPropNode::optimize(in_func) {
	caller->optimize(in_data);
	switch (caller->kind) {
		case NodeType::CALL:
		case NodeType::GET_PROP: {
			break;
		}
		case NodeType::UNKNOW: {
			auto node = static_cast<UnknowNode*>(caller)->correctNode;
			if (node && node->kind == NodeType::VAR) {
				static_cast<VarNode*>(node)->isStore = false;
			}
			break;
		}
		case NodeType::VAR: {
			static_cast<VarNode*>(caller)->isStore = false;
			break;
		}
		default: {
			throw std::runtime_error("Cannot find caller");
		}
	}
	auto clazz = &compile.classes[caller->classId];
	auto classInfo = &context.classInfo[clazz];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		//Find static member
		auto it_ = classInfo->staticMember.find(name);
		if (it_ == classInfo->staticMember.end())
			throw std::runtime_error("Cannot find member name: "+name);
		auto declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
			this->clazz != clazz) {
			throw std::runtime_error("Cannot access private member name '"+name+"'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
	}
	if (!isStatic) {
		//A.a = ...
		auto declarationNode = classInfo->member[it->second];
		isVal = !isInitial && declarationNode->isVal;
		if (declarationNode->accessModifier != Lexer::TokenType::PUBLIC &&
			this->clazz != clazz
		) {
			throw std::runtime_error("Cannot access private member name '"+name+"'");
		}
		
		id = it->second;
		// for (int i = 0; i<clazz->memberId.size(); ++i) {
		// 	printDebug("MemId: "+std::to_string(clazz->memberId[i]));
		// }
		if (clazz->memberId[id] != declarationNode->classId)
			clazz->memberId[id] = declarationNode->classId;
		classId = declarationNode->classId;//clazz->memberId[id];
		printDebug("Class " + clazz->name + " GetProp: "+name+" "+" has id: "+std::to_string(id)+" "+std::to_string(classId)+" "+compile.classes[classId].name);
	}
}

void IfNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throw std::runtime_error("Cannot use expression of type '" + condition->getClassName(in_data) + "' as a condition — expected 'Bool'");
	ifTrue.optimize(in_data);
	if (ifFalse) ifFalse->optimize(in_data);
}

void WhileNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throw std::runtime_error("Cannot use expression of type '" + condition->getClassName(in_data) + "' as a condition — expected 'Bool'");
	body.optimize(in_data);
}

void ForRangeNode::optimize(in_func) {
	detach->optimize(in_data);
	//if (detach->isVal)
	//	throw std::runtime_error("Cannot change because it's val");
	from->optimize(in_data);
	from = CastNode::createAndOptimize(in_data, from, AutoLang::DefaultClass::INTCLASSID);
	to->optimize(in_data);
	to = CastNode::createAndOptimize(in_data, to, AutoLang::DefaultClass::INTCLASSID);
	if (to->kind == NodeType::CONST) {
		static_cast<ConstValueNode*>(to)->isLoadPrimary = true;
	}
	body.optimize(in_data);
}

void SetNode::optimize(in_func) {
	detach->optimize(in_data);
	if (!value) return;
	value->optimize(in_data);
	classId = value->classId;
	auto detach = this->detach;
	new_detach:{}
	switch (detach->kind) {
		case NodeType::GET_PROP: {
			if (detach->classId != AutoLang::DefaultClass::nullClassId &&
				detach->classId != value->classId) {
				detach->classId = value->classId;
			}
			auto node = static_cast<GetPropNode*>(detach);
			node->isStore = true;
			if (node->isVal) {
				throw std::runtime_error("Cannot change "+
					compile.classes[node->caller->classId].name+"."+node->name+
					" because it's val");
			}
			if (!node->declaration) {
				auto clazz = &compile.classes[node->caller->classId];
				auto classInfo = &context.classInfo[clazz];
				for (auto* n:classInfo->member) {
					if (n->name != node->name) continue;
					node->declaration = n;
					goto next_getprop;
				}
				auto it = classInfo->staticMember.find(node->name);
				if (it != classInfo->staticMember.end()) {
					node->declaration = it->second;
					goto next_getprop;
				}
				throw std::runtime_error("Cannot find declaration "+clazz->name+"."+node->name);
			}
			next_getprop:{}
			if (node->declaration) {
				auto clazz = &compile.classes[node->caller->classId];
				//clazz->memberId[detach->declaration->id] = value->classId;
				node->declaration->classId = value->classId;
			}
			break;
		}
		case NodeType::VAR: {
			auto node = static_cast<VarNode*>(detach);
			node->isStore = true;
			if (detach->classId != AutoLang::DefaultClass::nullClassId &&
				detach->classId != node->declaration->classId) {
				detach->classId = node->declaration->classId;
			}
			if (detach->classId == AutoLang::DefaultClass::nullClassId) {
				if (node->declaration->classId == AutoLang::DefaultClass::nullClassId &&
					value->classId != AutoLang::DefaultClass::nullClassId) {
					node->declaration->classId = value->classId;
					printDebug(std::string("SetNode: Declaration ") + node->declaration->name + " is " + compile.classes[value->classId].name);
				}
				detach->classId = value->classId;
			}
			break;
		}
		case NodeType::UNKNOW: {
			detach = static_cast<UnknowNode*>(detach)->correctNode;
			goto new_detach;
		}
		default: break;
	}
	switch (value->kind) {
		case NodeType::CONST: {
			if (op != Lexer::TokenType::EQUAL) {
				static_cast<ConstValueNode*>(value)->isLoadPrimary = true;
			}
			break;
		}
		default: break;
	}
	if (detach->classId == value->classId) {
		if (op != Lexer::TokenType::EQUAL) {
			switch (detach->classId) {
				case AutoLang::DefaultClass::INTCLASSID:
				case AutoLang::DefaultClass::FLOATCLASSID:
					return;
				default:
					if (detach->classId == AutoLang::DefaultClass::stringClassId &&
						op == Lexer::TokenType::PLUS_EQUAL)
						return;
					break;
			}
			throw std::runtime_error("Cannot use "+Lexer::Token(0, op).toString(context)+" operator with "+
			compile.classes[detach->classId].name+" and "+compile.classes[value->classId].name);
		}
		return;
	}
	if ((detach->classId == AutoLang::DefaultClass::INTCLASSID || 
		  detach->classId == AutoLang::DefaultClass::FLOATCLASSID) && 
		 (value->classId == AutoLang::DefaultClass::INTCLASSID || 
		  value->classId == AutoLang::DefaultClass::FLOATCLASSID)) {
		if (value->kind != NodeType::CONST) {
			value = new CastNode(value, detach->classId);
			return;
		}
		//Optimize
		try {
			switch (detach->classId) {
				case AutoLang::DefaultClass::INTCLASSID:
					toInt(static_cast<ConstValueNode*>(value));
					return;
				case AutoLang::DefaultClass::FLOATCLASSID:
					toFloat(static_cast<ConstValueNode*>(value));
					return;
				default:
					throw std::runtime_error("");
			}
		}
		catch (const std::runtime_error& err) {
			throw std::runtime_error("Cannot cast "+compile.classes[value->classId].name+" to "+compile.classes[detach->classId].name);
		}
	}
	switch (detach->kind) {
		case NodeType::VAR: {
			throw std::runtime_error(static_cast<VarNode*>(detach)->declaration->name + " is declarated is " + compile.classes[detach->classId].name);
		}
		case NodeType::GET_PROP: {
			auto detach_ = static_cast<GetPropNode*>(detach);
			throw std::runtime_error(compile.classes[detach_->caller->classId].name +
				+ "." + detach_->name + " is declarated is " + compile.classes[detach->classId].name);
		}
		default:
			throw std::runtime_error(",Wtf");
	}
}

void CallNode::optimize(in_func) {
	std::string funcName;
	bool allowPrefix = false;
	if (caller) {
		//Caller.funcName() => Class.funcName()
		caller->optimize(in_data);
		switch (caller->kind) {
			case NodeType::VAR: {
				auto node = static_cast<VarNode*>(caller);
				node->isStore = false;
				node->classId = node->declaration->classId;
				break;
			}
			case NodeType::GET_PROP:
				break;
		}
		funcName = compile.classes[caller->classId].name + '.' + name;
		if (caller->kind == NodeType::CLASS) {
			delete caller;
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
			//Return Id in putbytecode
			caller = new ClassAccessNode(it->second);
			funcName = compile.classes[it->second].name + '.' + name;
		}
	}
	for (auto& argument:arguments) {
		argument->optimize(in_data);
	}
	uint8_t count = 0;
	std::vector<uint32_t> *funcVec[2];
	//Find
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
		throw std::runtime_error(std::string("Cannot find function name : ") + funcName);
	bool ambitiousCall = false;
	uint8_t foundIndex;
	bool found = false;
	MatchOverload first;
	MatchOverload second;
	int i = 0;
	int j = 0;
	//Find first function
	for (; j<count; ++j) {
		if (!match(compile, first, *funcVec[j], i)) {
			i = 0;
			continue;
		}
		found = true;
		foundIndex = j;
		break;
	}	//Find function
	for (; j<count; ++j) {
		std::vector<uint32_t> *vec = funcVec[j];
		while (match(compile, second, *vec, i)) {
			if (second.score < first.score) continue;
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
				isFirst = false; else
				argumentsStr += ", ";
			argumentsStr += compile.classes[argument->classId].name;
		}
		std::string detailFuncError = funcName.insert(funcName.size() - 1, argumentsStr);
		throw std::runtime_error(std::string("Cannot find function name with arguments : ") + detailFuncError);
	}
	if (ambitiousCall)
		throw std::runtime_error(std::string("Ambitious Call : ") + funcName);
	funcId = first.id;
	classId = first.func->returnId;
	auto func = &compile.functions[funcId];
	auto funcInfo = &context.functionInfo[func];
	if (funcInfo->accessModifier != Lexer::TokenType::PUBLIC &&
		 this->clazz != funcInfo->clazz)
		throw std::runtime_error("Cannot access private function name '"+funcName+"'");
	//Add this
	if (allowPrefix && foundIndex == 0) {
		caller = new VarNode(
			context.classInfo[clazz].declarationThis,
			false
		);
		caller->optimize(in_data);
	}
	if (func->isStatic && caller) {
		switch (caller->kind) {
			case NodeType::VAR: {
				bool callerClassId = caller->classId;
				delete caller;
				caller = new ClassAccessNode(callerClassId);
				break;
			}
			case NodeType::GET_PROP: {
				auto newCaller = static_cast<GetPropNode*>(caller)->caller;
				static_cast<GetPropNode*>(caller)->caller = nullptr;
				delete caller;
				caller = newCaller;
				//addPopBytecode = true;
				break;
			}
			case NodeType::CALL: {
				auto newCaller = static_cast<CallNode*>(caller)->caller;
				static_cast<CallNode*>(caller)->caller = nullptr;
				delete caller;
				caller = newCaller;
				//addPopBytecode = true;
				break;
			}
			default:
				break;
		}
	}
	if (caller && (caller->kind == NodeType::CLASS || caller->kind == NodeType::UNKNOW) && 
		!isConstructor && !func->isStatic)
		throw std::runtime_error(func->name + " is not static function");
}

bool CallNode::match(CompiledProgram& compile, MatchOverload& match, std::vector<uint32_t>& functions, int& i) {
	match.score = 0;
	for (; i<functions.size(); ++i) {
		match.id = functions[i];
		match.func = &compile.functions[match.id];
		bool skip = false;
		if (!match.func->isStatic) {
			if (!caller || (justFindStatic && !isConstructor && 
				(caller->kind == NodeType::CLASS || caller->kind == NodeType::UNKNOW))) 
				continue;
			if (caller->classId != match.func->args[0])
				continue;
			skip =  true;
		}
		if (match.func->args.size() != arguments.size() + skip)
			continue;
		bool matched = true;
		for (int j=0; j<arguments.size(); ++j) {
			uint32_t inputClassId = arguments[j]->classId;
			uint32_t funcArgClassId = match.func->args[j + skip];
			if (funcArgClassId != inputClassId) {
				if (funcArgClassId == AutoLang::DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				if (inputClassId == AutoLang::DefaultClass::INTCLASSID &&
					funcArgClassId == AutoLang::DefaultClass::FLOATCLASSID) {
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
	if (value) {
		value->optimize(in_data);
		classId = value->classId;
	} else
		classId = AutoLang::DefaultClass::nullClassId;
	if (classId == func->returnId) return;
	if (!value) throw std::runtime_error("Must return value");
	if (func->returnId == AutoLang::DefaultClass::nullClassId) 
		throw std::runtime_error("Cannot return value, function return Void");
	value = CastNode::createAndOptimize(in_data, value, func->returnId);
}

void VarNode::optimize(in_func) {
	classId = declaration->classId;
	isVal = declaration->isVal;
}

}

#endif