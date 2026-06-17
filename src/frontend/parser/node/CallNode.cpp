#ifndef CALL_NODE_CPP
#define CALL_NODE_CPP

#include "Node.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

ExprNode *CallNode::resolve(in_func) {
	for (auto &argument : arguments) {
		argument = static_cast<HasClassIdNode *>(argument->resolve(in_data));
	}
	if (funcObject) {
		funcObject =
		    static_cast<HasClassIdNode *>(funcObject->resolve(in_data));
	}
	if (caller) {
		caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	} else {
		switch (nameId) {
			case lexerIdInt: {
				if (arguments.size() != 1) {
					throwError("Invalid call: Int expects 1 "
					           "argument, but " +
					           std::to_string(arguments.size()) +
					           " were provided");
				}
				auto result = context.castPool.push(arguments[0],
				                                    DefaultClass::intClassId);
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
				auto result = context.castPool.push(arguments[0],
				                                    DefaultClass::floatClassId);
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
				auto result = context.castPool.push(arguments[0],
				                                    DefaultClass::boolClassId);
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
				// case lexerId__CLASS__:
				// case lexerId__FILE__:
				// case lexerId__FUNC__:
				// case lexerId__LINE__: {
				// 	throwError("Invalid call: " + context.lexerString[nameId] +
				// 	           " is magic const ");
				// }
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
	static std::vector<FunctionId> *funcVec[2];

	if (nameId == lexerIdLRBRACKET)
		nameId = lexerIdget;

	const auto &name = context.lexerString[nameId];
	bool mustInferenceGenericType = false;

	for (int i = 0; i < arguments.size(); ++i) {
		auto argument = arguments[i];
		switch (argument->kind) {
			case NodeType::CLASS_ACCESS: {
				throwError("Cannot input class at parameter " +
				           std::to_string(i + 1));
			}
			case NodeType::CALL: {
				argument->optimize(in_data);
				if (argument->classId == AutoLang::DefaultClass::voidClassId) {
					throwError("Cannot input Void value at parameter " +
					           std::to_string(i + 1));
				}
				break;
			}
			case NodeType::CREATE_ARRAY:
			case NodeType::CREATE_MAP:
			case NodeType::CREATE_SET: {
				if (argument->classDeclaration) {
					argument->optimize(in_data);
				} else {
					mustInferenceGenericType = true;
				}
				break;
			}
			case NodeType::CREATE_CLOSURE:
			case NodeType::FUNCTION_ACCESS: {
				break;
			}
			default: {
				argument->optimize(in_data);
				break;
			}
		}
	}

	if (caller) {
		// Caller.funcName() => Class.funcName()
		caller->optimize(in_data);
		if (caller->isNullable()) {
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

		auto callerClassInfo = context.classInfo[caller->classId];
		{
			auto it = callerClassInfo->allFunction.find(nameId);
			if (it != callerClassInfo->allFunction.end()) {
				funcVec[count++] = &it->second;
				callerCanCallId = caller->classId;
			} else {
				auto member = callerClassInfo->findAllMember(
				    in_data, line, nameId, justFindStatic);
				if (member) {
					funcObject = context.getPropPool.push(
					    line, member, caller->classId,
					    context.varPool.push(line,
					                         callerClassInfo->declarationThis,
					                         false, false),
					    nameId, true, true, false);
					matchFunction(in_data, mustInferenceGenericType);
					return;
				}
				funcName = name;
			}
		}

		// {
		// 	auto it = compile.funcMap.find(funcName);
		// 	if (it != compile.funcMap.end()) {
		// 		funcVec[count++] = &it->second;
		// 	}
		// }

	} else {
		// Check if constructor
		if (funcObject) {
			matchFunction(in_data, mustInferenceGenericType);
			return;
		}

		{
			auto it = context.defaultClassMap.find(nameId);
			if (it == context.defaultClassMap.end()) {
				funcName = name;
				if (contextCallClassId) {
					auto callerClassInfo =
					    context.classInfo[*contextCallClassId];
					auto it = callerClassInfo->allFunction.find(nameId);
					if (it != callerClassInfo->allFunction.end()) {
						funcVec[count++] = &it->second;
						callerCanCallId = *contextCallClassId;
					}
				}
				// allowPrefix = clazz != nullptr;
			} else {
				// Return Id in putbytecode
				auto classInfo = context.classInfo[it->second];
				if (!classInfo->genericData) {
					funcName = compile.classes[it->second]->name + '.' + name;
					caller = context.classAccessPool.push(line, it->second);
				} else {
					funcName = name;
				}
			}
		}

		{
			auto it = compile.funcMap.find(funcName);
			if (it != compile.funcMap.end()) {
				funcVec[count++] = &it->second;
			}
		}

		if (count == 0 && contextCallClassId) {
			auto classInfo = context.classInfo[*contextCallClassId];
			auto member =
			    classInfo->findAllMember(in_data, line, nameId, justFindStatic);
			if (member) {
				funcObject = context.getPropPool.push(
				    line, member, contextCallClassId,
				    context.varPool.push(line, classInfo->declarationThis,
				                         false, false),
				    nameId, true, true, false);
				matchFunction(in_data, mustInferenceGenericType);
				return;
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
		if (!match(in_data, first, *funcVec[j], i, mustInferenceGenericType)) {
			i = 0;
			continue;
		}
		found = true;
		foundIndex = j;
		break;
	} // Find function
	for (; j < count; ++j) {
		std::vector<uint32_t> *vec = funcVec[j];
		while (match(in_data, second, *vec, i, mustInferenceGenericType)) {
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
	notFound:;
		std::string currentFuncLog = funcName + "(";
		bool isFirst = true;
		for (auto argument : arguments) {
			if (isFirst)
				isFirst = false;
			else
				currentFuncLog += ", ";
			currentFuncLog += compile.classes[argument->classId]->name;
		}
		std::string found;
		bool isFirst1 = true;
		for (int j = 0; j < count; ++j) {
			auto &vecs = *funcVec[j];
			if (vecs.empty()) {
				found = "EMPTY";
				printDebug("Empty");
			}
			for (auto v : vecs) {
				if (isFirst1) {
					isFirst1 = false;
				} else {
					found += "\n";
				}
				found += compile.functions[v]->toString(compile);
			}
		}
		throwError(std::string("Cannot find function name: " +
		                       context.lexerString[nameId] +
		                       " has arguments : ") +
		           currentFuncLog + ") \nFound: " + found);
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
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	classId = first.func->returnId;
	if (func->returnId == DefaultClass::functionClassId) {
		classDeclaration = funcInfo->returnClass;
	}
	{
		int i = arguments.size() +
		        !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
		for (; i < funcInfo->parameter->parameters.size(); ++i) {
			arguments.push_back(funcInfo->parameter->parameterDefaultValues
			                        [i - funcInfo->parameter->defaultValuePos]);
		}
	}

	// if (mustInferenceGenericType) {
	{
		int i = func->functionFlags & FunctionFlags::FUNC_IS_STATIC ? 0 : 1;
		for (auto &argument : arguments) {
			auto funcExpectClass = funcInfo->parameter->parameters[i];
			auto funcExpectClassId = func->args[i++];
			auto funcExpectClassInfo = context.classInfo[funcExpectClassId];
			if (argument->isNullable() && !funcExpectClass->nullable) {
				if (mode->flags & LibraryFlags::ALLOW_NON_NULL_ASSERTION) {
					throwError(
					    "Error: Nullability mismatch at parameter " +
					    std::to_string(i) +
					    ": "
					    "expected non-null, but argument could be null"
					    "\nHint: Use '!' to assert or '?\?' to fallback.");
				}
				throwError("Error: Nullability mismatch at parameter " +
				           std::to_string(i) +
				           ": "
				           "expected non-null, but argument could be null"
				           "\nHint: Use '?\?' to provide a fallback value.");
			}
			switch (argument->classId) {
				case DefaultClass::intClassId: {
					if (funcExpectClassId == DefaultClass::floatClassId) {
						argument = static_cast<HasClassIdNode *>(
						    context.castPool
						        .push(argument, DefaultClass::floatClassId)
						        ->resolve(in_data));
						argument->optimize(in_data);
					}
					break;
				}
				case DefaultClass::functionClassId: {
					switch (argument->kind) {
						case NodeType::FUNCTION_ACCESS: {
							argument->classDeclaration =
							    funcExpectClass->classDeclaration;
							argument->optimize(in_data);
							break;
						}
						case NodeType::CREATE_CLOSURE: {
							auto node =
							    static_cast<CreateClosureNode *>(argument);
							// if (node->mustInfer) {
							node->inferFrom(in_data,
							                funcExpectClass->classDeclaration);
							argument->optimize(in_data);
							// 	break;
							// }
							// argument->optimize(in_data);
							// matchFunction(in_data,
							//               funcInputClass->classDeclaration,
							//               argument->classDeclaration);
							break;
						}
						default: {
							matchFunction(in_data,
							              funcExpectClass->classDeclaration,
							              argument->classDeclaration);
						}
					}
					break;
				}
				case DefaultClass::nullClassId: {
					switch (argument->kind) {
						case NodeType::CREATE_ARRAY: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::arrayClassId) {
								goto notFound;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
						case NodeType::CREATE_MAP: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::mapClassId) {
								goto notFound;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
						case NodeType::CREATE_SET: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::setClassId) {
								if (funcExpectClassInfo->baseClassId ==
								    DefaultClass::mapClassId) {
									argument = context.createMapPool.push(
									    argument->line, nullptr,
									    std::vector<
									        std::pair<HasClassIdNode *,
									                  HasClassIdNode *>>{});
								} else {
									goto notFound;
								}
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
						case NodeType::WHEN:
						case NodeType::IF: {
							auto n = static_cast<NullableNode *>(argument);
							argument->classId = funcExpectClassId;
							n->nullable = funcExpectClass->nullable;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
					}
					break;
				}
			}
		}
	}

	if (funcInfo->genericData) {
		throwError(
		    "Function " + funcName + " expects " +
		    std::to_string(funcInfo->genericData->genericDeclarations.size()) +
		    " type argument but 0 were given");
	}

	if (funcInfo->inferenceNode && !funcInfo->inferenceNode->loaded) {
		funcInfo->inferenceNode->resolve(in_data);
		funcInfo->inferenceNode->optimize(in_data);
		funcInfo->inferenceNode->loaded = true;
	}

	if (nullable) {
		nullable = func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;
	}

	if (first.errorNonNullIfMatchCount) {
		throwError(std::string(
		               "Cannot input null in non null arguments of function ") +
		           funcName);
	}
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

void CallNode::matchFunction(in_func, ClassDeclaration *detach,
                             ClassDeclaration *value) {
	detach->load<true>(in_data);
	size_t size = detach->inputClassId.size();
	if (size == detach->inputClassId.size()) {
		for (int i = 0; i < size; ++i) {
			// if (!detach->inputClassId[i]->classId) {
			// 	std::cerr << "WTF1\n";
			// }
			// if (!value->inputClassId[i]->classId) {
			// 	std::cerr << "WTF2\n";
			// }
			if (detach->inputClassId[i]->classId !=
			    value->inputClassId[i]->classId) {

				throwError("Type mismatch: expected '" +
				           detach->getName(in_data) + "' but found '" +
				           value->getName(in_data));
			}
		}
		return;
	} else {
		throwError("Type mismatch: expected '" + detach->getName(in_data) +
		           "' but found '" + value->getName(in_data));
	}
}

void CallNode::matchFunction(in_func, bool mustInferenceGenericType) {
	funcObject->optimize(in_data);

	if (funcObject->classId != DefaultClass::functionClassId) {
		throwError("Cannot call non-function object");
	}

	if (!funcObject->classDeclaration) {
		throwError("Bug: Class not ensure is Function");
	}

	if (funcObject->isNullable()) {
		throwError("Cannot call nullable function object");
	}

	auto &inputClass = funcObject->classDeclaration->inputClassId;

	classId = *inputClass[0]->classId;
	if (classId == DefaultClass::functionClassId) {
		classDeclaration = inputClass[0];
	}
	nullable = funcObject->classDeclaration->nullable;

	if (inputClass.size() - 1 != arguments.size()) {
		throwError("Object " + context.lexerString[nameId] + ": " +
		           funcObject->classDeclaration->getName(in_data) +
		           " expects " + std::to_string(inputClass.size() - 1) +
		           " argument but " + std::to_string(arguments.size()) +
		           " were given");
	}
	if (justFindStatic) {
		throwError("Just find static");
	}
	int j = 0;
	for (; j < arguments.size(); ++j) {
		auto &argument = arguments[j];
		uint32_t inputClassId = argument->classId;
		auto funcExpectClass = inputClass[j + 1];
		uint32_t funcExpectClassId = *funcExpectClass->classId;
		if (argument->isNullable() && !funcExpectClass->nullable) {
			if (mode->flags & LibraryFlags::ALLOW_NON_NULL_ASSERTION) {
				throwError("Error: Nullability mismatch at parameter " +
				           std::to_string(j + 1) +
				           ": "
				           "expected non-null, but argument could be null"
				           "\nHint: Use '!' to assert or '?\?' to fallback.");
			}
			throwError("Error: Nullability mismatch at parameter " +
			           std::to_string(j + 1) +
			           ": "
			           "expected non-null, but argument could be null"
			           "\nHint: Use '?\?' to provide a fallback value.");
		}
		if (funcExpectClassId == inputClassId) {
			if (funcExpectClassId == DefaultClass::functionClassId) {
				switch (argument->kind) {
					case NodeType::FUNCTION_ACCESS: {
						argument->classDeclaration = inputClass[j + 1];
						argument->optimize(in_data);
						break;
					}
					case NodeType::CREATE_CLOSURE: {
						auto node = static_cast<CreateClosureNode *>(argument);
						// if (node->mustInfer) {
						node->inferFrom(in_data, funcExpectClass);
						argument->optimize(in_data);
						// 	break;
						// }
						// argument->optimize(in_data);
						// matchFunction(in_data, inputClass[j + 1],
						//               argument->classDeclaration);
						break;
					}
					default: {
						matchFunction(in_data, funcExpectClass,
						              argument->classDeclaration);
						break;
					}
				}
				break;
			}
			continue;
		}
		if (funcExpectClassId == DefaultClass::anyClassId) {
			continue;
		}
		switch (inputClassId) {
			case DefaultClass::nullClassId: {
				if (mustInferenceGenericType) {
					auto &argument = arguments[j];
					auto funcExpectClassInfo =
					    context.classInfo[funcExpectClassId];
					switch (argument->kind) {
						case NodeType::CREATE_ARRAY: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::arrayClassId) {
								goto err;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							break;
						}
						case NodeType::CREATE_MAP: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::mapClassId) {
								goto err;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							break;
						}
						case NodeType::CREATE_SET: {
							if (funcExpectClassInfo->baseClassId !=
							    DefaultClass::setClassId) {
								if (funcExpectClassInfo->baseClassId !=
								    DefaultClass::mapClassId) {
									goto err;
								}
								argument = context.createMapPool.push(
								    argument->line, nullptr,
								    std::vector<std::pair<HasClassIdNode *,
								                          HasClassIdNode *>>{});
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							break;
						}
						case NodeType::WHEN:
						case NodeType::IF: {
							auto n = static_cast<NullableNode *>(argument);
							argument->classId = funcExpectClassId;
							n->nullable = funcExpectClass->nullable;
							argument->optimize(in_data);
							break;
						}
					}
				}
				continue;
			}
			// Never functionClassId
			case DefaultClass::intClassId: {
				if (funcExpectClassId == AutoLang::DefaultClass::floatClassId) {
					argument = static_cast<HasClassIdNode *>(
					    context.castPool
					        .push(argument, DefaultClass::floatClassId)
					        ->resolve(in_data));
					argument->optimize(in_data);
					continue;
				}
				break;
			}
			default: {
				if (compile.classes[inputClassId]->inheritance.get(
				        funcExpectClassId)) {
					continue;
				}
				break;
			}
		}
	}

	return;

err:;
	auto argument = arguments[j];
	switch (argument->kind) {
		case NodeType::CREATE_ARRAY: {
			argument->classId = DefaultClass::arrayClassId;
			break;
		}
		case NodeType::CREATE_SET: {
			argument->classId = DefaultClass::setClassId;
			break;
		}
		case NodeType::CREATE_MAP: {
			argument->classId = DefaultClass::mapClassId;
			break;
		}
	}
	auto argumentClassId = argument->classId;
	throwError("Object " + context.lexerString[nameId] + ": At argument " +
	           std::to_string(j) + " expected " +
	           compile.classes[*inputClass[j + 1]->classId]->name + " but " +
	           compile.classes[argumentClassId]->name + " found");
}

bool CallNode::match(in_func, MatchOverload &match,
                     std::vector<FunctionId> &functions, int &i,
                     bool mustInferenceGenericType) {
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
		size_t argumentSize = arguments.size() + skip;
		if (argumentSize < funcInfo->parameter->defaultValuePos ||
		    argumentSize > funcInfo->parameter->parameters.size())
			continue;
		match.errorNonNullIfMatchCount = 0;
		// std::cerr << match.func->name << " " << arguments.size() << " "
		//           << argumentSize << " " << skip << " "
		//           << funcInfo->parameter->parameters.size() << " "
		//           << funcInfo->parameter->defaultValuePos << "\n";
		for (int j = 0; j < arguments.size(); ++j) {
			uint32_t inputClassId = arguments[j]->classId;
			uint32_t funcExpectClassId = match.func->args[j + skip];
			// printDebug(compile.classes[inputClassId]->name + " and " +
			// compile.classes[funcExpectClassId]->name);
			if (funcExpectClassId != inputClassId) {
				if (funcExpectClassId == DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				switch (inputClassId) {
					case DefaultClass::nullClassId: {
						if (mustInferenceGenericType) {
							auto argument = arguments[j];
							auto funcExpectClassInfo =
							    context.classInfo[funcExpectClassId];
							switch (argument->kind) {
								case NodeType::CREATE_ARRAY: {
									if (funcExpectClassInfo->baseClassId !=
									    DefaultClass::arrayClassId) {
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_MAP: {
									if (funcExpectClassInfo->baseClassId !=
									    DefaultClass::mapClassId) {
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_SET: {
									if (funcExpectClassInfo->baseClassId !=
									    DefaultClass::setClassId) {
										if (funcExpectClassInfo->baseClassId ==
										    DefaultClass::mapClassId) {
											break;
										}
										goto finished;
									}
									break;
								}
							}
						}

						++match.score;
						match.errorNonNullIfMatchCount +=
						    !funcInfo->parameter->parameters[j + skip]
						         ->nullable;
						continue;
					}
					case DefaultClass::intClassId: {
						if (funcExpectClassId ==
						    AutoLang::DefaultClass::floatClassId) {
							++match.score;
							continue;
						}
						break;
					}
					default: {
						if (compile.classes[inputClassId]->inheritance.get(
						        funcExpectClassId)) {
							++match.score;
							continue;
						}
						break;
					}
				}
				goto finished;
			}
			match.score += 2;
		}
		// Matched
		++i;
		return true;
	finished:;
	}
	return false;
}

void CallNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (funcObject) {
		for (auto argument : arguments) {
			argument->putBytecodes(in_data, bytecodes);
		}
		funcObject->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::CALL_FUNCTION_OBJECT);
		return;
	}

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
			jumpIfNullPos = bytecodes.size() - context.currentBytecodePos;
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
			// if (func->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			// 	bytecodes.emplace_back(returnVoid
			// 	                           ? Opcode::CALL_VOID_NATIVE_FUNCTION
			// 	                           : Opcode::CALL_NATIVE_FUNCTION);
			// } else {
			// 	bytecodes.emplace_back(returnVoid ? Opcode::CALL_VOID_FUNCTION
			// 	                                  : Opcode::CALL_FUNCTION);
			// }
		}
		put_opcode_u32(bytecodes, funcId);
	}
	// put_opcode_u32(bytecodes, func->args.size);
	// std::cerr<<funcId<<'\n';
}

void CallNode::rewrite(in_func, uint8_t *bytecodes) {
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
	    line, context.currentClassId, newCaller, nameId,
	    std::move(newArguments), justFindStatic, nullable, accessNullable);
	newNode->classId = classId;
	newNode->classDeclaration = classDeclaration;
	if (funcObject) {
		newNode->funcObject =
		    static_cast<HasClassIdNode *>(funcObject->copy(in_data));
	}
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