#ifndef CALL_NODE_CPP
#define CALL_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

ExprNode *CallNode::resolve(in_func) {
	for (auto &argument : arguments) {
		argument = static_cast<HasClassIdNode *>(argument->resolve(in_data));
	}
	if (caller) {
		caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	} else if (!name.empty()) {
		auto it =
		    context.lexerStringMap.find(name.substr(0, name.length() - 2));
		if (it != context.lexerStringMap.end()) {
			LexerStringId nameId = it->second;
			switch (nameId) {
				case lexerIdInt: {
					if (arguments.size() != 1) {
						throwError("Invalid call: Int expects 1 "
						           "argument, but " +
						           std::to_string(arguments.size()) +
						           " were provided");
					}
					auto result = context.castPool.push(
					    arguments[0], DefaultClass::intClassId);
					arguments.clear();
					return result;
				}
				case lexerIdFloat: {
					if (arguments.size() != 1) {
						throwError("Invalid call: Float expects 1 "
						           "argument, but " +
						           std::to_string(arguments.size()) +
						           " were provided");
					}
					auto result = context.castPool.push(
					    arguments[0], DefaultClass::floatClassId);
					arguments.clear();
					return result;
				}
				case lexerIdBool: {
					if (arguments.size() != 1) {
						throwError("Invalid call: Bool expects 1 "
						           "argument, but " +
						           std::to_string(arguments.size()) +
						           " were provided");
					}
					auto result = context.castPool.push(
					    arguments[0], DefaultClass::boolClassId);
					arguments.clear();
					return result;
				}
				case lexerIdgetClassId: {
					if (arguments.size() != 1) {
						throwError("Invalid call: getClassId() expects 1 "
						           "argument, but " +
						           std::to_string(arguments.size()) +
						           " were provided");
					}
					auto result = context.constValuePool.push(
					    line, static_cast<int64_t>(arguments[0]->classId));
					arguments.clear();
					return result;
				}
			}
		}
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

	if (name == "[]")
		name = "get()";

	for (auto &argument : arguments) {
		argument->optimize(in_data);
		switch (argument->kind) {
			case NodeType::CLASS_ACCESS: {
				throwError("Cannot input class at function " + name);
			}
			case NodeType::CALL: {
				if (argument->classId == AutoLang::DefaultClass::voidClassId) {
					throwError("Cannot input Void value");
				}
				break;
			}
		}
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
		if (name.back() == ')') {
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
				caller = context.classAccessPool.push(line, it->second);
			}
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
		           currentFuncLog + (funcName.back() == ')' ? ")" : "]"));
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
	auto funcInfo = context.functionInfo[funcId];

	if (funcInfo->inferenceNode && !funcInfo->inferenceNode->loaded) {
		funcInfo->inferenceNode->resolve(in_data);
		funcInfo->inferenceNode->optimize(in_data);
		funcInfo->inferenceNode->loaded = true;
	}

	nullable = func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;

	if (first.errorNonNullIfMatch)
		throwError(std::string("Cannot input null in non null arguments ") +
		           funcName);
	if (!(func->functionFlags & FunctionFlags::FUNC_PUBLIC) &&
	    (!contextCallClassId || *contextCallClassId != funcInfo->clazz->id))
		throwError("Cannot access private function name '" + funcName + "'");
	// Add this
	if (!caller && !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		caller = context.varPool.push(
		    line, context.classInfo[callerCanCallId]->declarationThis, false,
		    false);
		caller->optimize(in_data);
	}
	if ((func->functionFlags & FunctionFlags::FUNC_IS_STATIC) && caller) {
		switch (caller->kind) {
			case NodeType::VAR: {
				bool callerClassId = caller->classId;
				ExprNode::deleteNode(caller);
				caller = context.classAccessPool.push(line, callerClassId);
				break;
			}
			case NodeType::GET_PROP: {
				auto newCaller = static_cast<GetPropNode *>(caller)->caller;
				static_cast<GetPropNode *>(caller)->caller = nullptr;
				ExprNode::deleteNode(caller);
				caller = newCaller;
				break;
			}
			case NodeType::CALL: {
				auto newCaller = static_cast<CallNode *>(caller)->caller;
				static_cast<CallNode *>(caller)->caller = nullptr;
				ExprNode::deleteNode(caller);
				caller = newCaller;
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
		auto funcInfo = context.functionInfo[match.id];
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
				if (funcArgClassId == DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				switch (inputClassId) {
					case DefaultClass::nullClassId: {
						++match.score;
						if (!match.errorNonNullIfMatch)
							match.errorNonNullIfMatch =
							    !funcInfo->nullableArgs[j + skip];
						continue;
					}
					case DefaultClass::intClassId: {
						if (funcArgClassId ==
						    AutoLang::DefaultClass::floatClassId) {
							++match.score;
							continue;
						}
						break;
					}
					default: {
						if (compile.classes[inputClassId]->inheritance.get(
						        funcArgClassId)) {
							++match.score;
							continue;
						}
						break;
					}
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

void CallNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	auto *func = compile.functions[funcId];
	auto *funcInfo = context.functionInfo[funcId];
	if (func->functionFlags & FunctionFlags::FUNC_WAIT_INPUT) {
		bytecodes.emplace_back(Opcode::WAIT_INPUT);
	}
	if (caller) {
		caller->putBytecodes(in_data, bytecodes);
		if (accessNullable) {
			assert(context.jumpIfNullNode != nullptr);
			bytecodes.emplace_back(context.jumpIfNullNode->returnNullIfNull
			                           ? Opcode::JUMP_AND_SET_IF_NULL
			                           : Opcode::JUMP_AND_DELETE_IF_NULL);
			jumpIfNullPos = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
		}
	}
	if (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR) {
		if (isSuper) {
			bytecodes.emplace_back(Opcode::LOAD_LOCAL);
			put_opcode_u32(bytecodes, 0);
		} else {
			if (contextCallClassId &&
			    compile.classes[*contextCallClassId]->classFlags &
			        ClassFlags::CLASS_NATIVE_DATA) {
				bytecodes.emplace_back(Opcode::CREATE_NATIVE_OBJECT);
				put_opcode_u32(bytecodes, classId);
			} else {
				bytecodes.emplace_back(Opcode::CREATE_OBJECT);
				put_opcode_u32(bytecodes, classId);
				put_opcode_u32(bytecodes,
				               compile.classes[classId]->memberId.size());
			}
		}
	}
	for (auto &argument : arguments) {
		argument->putBytecodes(in_data, bytecodes);
	}
	if (func->functionFlags & FunctionFlags::FUNC_IS_VIRTUAL) {
		bool returnVoid =
		    func->returnId == DefaultClass::voidClassId ||
		    (func->functionFlags & FunctionFlags::FUNC_WAIT_INPUT);
		bytecodes.emplace_back(returnVoid ? Opcode::CALL_VTABLE_VOID_FUNCTION
		                                  : Opcode::CALL_VTABLE_FUNCTION);
		put_opcode_u32(bytecodes, funcInfo->virtualPosition);
		put_opcode_u32(bytecodes, func->argSize);
		// std::cerr<<"At "<<func->name<<"\n";
		// std::cerr<<"Put "<<funcInfo->virtualPosition<<" &
		// "<<func->argSize<<"\n";
	} else {
		if (func->functionFlags & FunctionFlags::FUNC_IS_DATA_CONSTRUCTOR) {
			bytecodes.emplace_back(Opcode::CALL_DATA_CONTRUCTOR);
		} else {
			bool returnVoid =
			    func->returnId == DefaultClass::voidClassId ||
			    (func->functionFlags & FunctionFlags::FUNC_WAIT_INPUT);
			bytecodes.emplace_back(returnVoid ? Opcode::CALL_VOID_FUNCTION
			                                  : Opcode::CALL_FUNCTION);
		}
		put_opcode_u32(bytecodes, funcId);
	}
	// put_opcode_u32(bytecodes, func->args.size);
	// std::cout<<funcId<<'\n';
}

void CallNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto argument : arguments) {
		argument->rewrite(in_data, bytecodes);
	}
	if (context.jumpIfNullNode && caller) {
		caller->rewrite(in_data, bytecodes);
		if (accessNullable) {
			rewrite_opcode_u32(bytecodes, jumpIfNullPos,
			                   context.jumpIfNullNode->jumpIfNullPos);
		}
	}
}

ExprNode *CallNode::copy(in_func) {
	HasClassIdNode *newCaller = nullptr;
	if (caller) {
		newCaller = static_cast<HasClassIdNode *>(caller->copy(in_data));
	}
	std::vector<HasClassIdNode *> newArguments;
	newArguments.reserve(arguments.size());
	for (auto *argument : arguments) {
		newArguments.push_back(
		    static_cast<HasClassIdNode *>(argument->copy(in_data)));
	}
	auto newNode = context.callNodePool.push(
	    line, context.currentClassId, newCaller, name, std::move(newArguments),
	    justFindStatic, nullable, accessNullable);
	newNode->classId = classId;
	return newNode;
}

CallNode::~CallNode() {
	deleteNode(caller);
	for (auto *argument : arguments) {
		deleteNode(argument);
	}
}

} // namespace AutoLang

#endif