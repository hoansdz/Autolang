#ifndef CALL_NODE_CPP
#define CALL_NODE_CPP

#include "Node.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ImplicitConversion.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ClassInfo.hpp"
#include "frontend/parser/GenericData.hpp"
#include "shared/ClassFlags.hpp"
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

static bool matchAndBindType(in_func, ClassDeclaration *paramDecl, HasClassIdNode *argNode,
                             ClassDeclaration *argDecl,
                             HashMap<LexerStringId, ClassDeclaration *> &bindings,
                             GenericData *genericData) {
	if (!paramDecl) return true;

	bool isGenericParam = paramDecl->isGenericDeclaration ||
	                      (genericData && genericData->findDeclaration(paramDecl->baseClassLexerStringId) != nullptr);

	if (isGenericParam) {
		ClassDeclaration *concreteDecl = argDecl;
		if (!concreteDecl && argNode) {
			if (argNode->classDeclaration) {
				concreteDecl = argNode->classDeclaration;
			} else {
				concreteDecl = context.classDeclarationAllocator.push();
				concreteDecl->classId = argNode->classId;
				concreteDecl->nullable = argNode->isNullable();
				concreteDecl->baseClassLexerStringId =
				    context.createLexerStringIfNotExists(compile.classes[argNode->classId]->getName(compile));
			}
		}
		if (!concreteDecl) return false;

		if (genericData) {
			auto genDecl = genericData->findDeclaration(paramDecl->baseClassLexerStringId);
			if (genDecl && genDecl->condition.has_value()) {
				auto boundDecl = genDecl->condition->classDeclaration;
				if (boundDecl) {
					if (!boundDecl->classId.has_value()) {
						boundDecl->template load<false>(in_data);
					}
					if (boundDecl->classId.has_value() && concreteDecl->classId.has_value()) {
						ClassId boundId = *boundDecl->classId;
						ClassId actualId = *concreteDecl->classId;
						if (actualId != boundId && !compile.classes[actualId]->inheritance.get(boundId)) {
							return false;
						}
					}
				}
			}
		}

		auto it = bindings.find(paramDecl->baseClassLexerStringId);
		if (it != bindings.end()) {
			if (it->second->classId.has_value() && concreteDecl->classId.has_value()) {
				ClassId prevId = *it->second->classId;
				ClassId currId = *concreteDecl->classId;
				if (prevId == currId) {
					return true;
				}
				if (compile.classes[currId]->inheritance.get(prevId)) {
					return true;
				}
				if (compile.classes[prevId]->inheritance.get(currId)) {
					it->second = concreteDecl;
					return true;
				}
				return false;
			}
			return true;
		} else {
			bindings[paramDecl->baseClassLexerStringId] = concreteDecl;
			return true;
		}
	}

	if (!paramDecl->inputClassId.empty()) {
		const std::vector<ClassDeclaration *> *argArgs = nullptr;
		std::vector<ClassDeclaration *> fallbackArgs;

		if (argDecl && !argDecl->inputClassId.empty()) {
			argArgs = &argDecl->inputClassId;
		} else if (argNode && argNode->classDeclaration && !argNode->classDeclaration->inputClassId.empty()) {
			argArgs = &argNode->classDeclaration->inputClassId;
		} else {
			ClassId actualClassId = argDecl && argDecl->classId.has_value() ? *argDecl->classId : (argNode ? argNode->classId : DefaultClass::nullClassId);
			if (actualClassId != DefaultClass::nullClassId && actualClassId < context.classInfo.size()) {
				auto actualInfo = context.classInfo[actualClassId];
				if (actualInfo && !actualInfo->genericTypeId.empty()) {
					fallbackArgs.assign(actualInfo->genericTypeId.begin(), actualInfo->genericTypeId.end());
					argArgs = &fallbackArgs;
				}
			}
		}

		if (argArgs && argArgs->size() == paramDecl->inputClassId.size()) {
			for (size_t k = 0; k < paramDecl->inputClassId.size(); ++k) {
				if (!matchAndBindType(in_data, paramDecl->inputClassId[k], nullptr, (*argArgs)[k], bindings, genericData)) {
					return false;
				}
			}
			return true;
		}
	}

	return true;
}

static ClassDeclaration *tryInferGenericArguments(in_func, GenericData *genericData,
                                                  Parameter *parameter,
                                                  const SmallVector<HasClassIdNode *, 4> &arguments,
                                                  size_t skip, LexerStringId baseNameId) {
	if (!genericData || !parameter) return nullptr;
	size_t numArgs = arguments.size();
	size_t numParams = parameter->parameters.size();
	if (numArgs + skip > numParams) return nullptr;
	if (numArgs + skip < parameter->defaultValuePos) return nullptr;

	HashMap<LexerStringId, ClassDeclaration *> bindings;
	for (size_t j = 0; j < numArgs; ++j) {
		auto paramNode = parameter->parameters[j + skip];
		auto argNode = arguments[j];
		if (!matchAndBindType(in_data, paramNode->classDeclaration, argNode, nullptr, bindings, genericData)) {
			return nullptr;
		}
	}

	for (auto *genDecl : genericData->genericDeclarations) {
		if (bindings.find(genDecl->nameId) == bindings.end()) {
			return nullptr;
		}
	}

	auto inferredClassDecl = context.classDeclarationAllocator.push();
	inferredClassDecl->baseClassLexerStringId = baseNameId;
	inferredClassDecl->isGeneric = true;
	inferredClassDecl->isGenericDeclaration = false;
	inferredClassDecl->inputClassId.reserve(genericData->genericDeclarations.size());
	for (auto *genDecl : genericData->genericDeclarations) {
		auto *boundType = bindings[genDecl->nameId];
		if (!boundType->classId.has_value()) {
			boundType->template load<false>(in_data);
			if (!boundType->classId.has_value()) {
				boundType->template load<true>(in_data);
			}
		}
		inferredClassDecl->inputClassId.push_back(boundType);
	}
	return inferredClassDecl;
}

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
					throwError(
					    "Invalid call: Int expects 1 "
					    "argument, but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: Pass exactly one argument to "
					    "convert to Int (e.g., Int(value)).");
				}
				auto result = context.castPool.push(arguments[0],
				                                    DefaultClass::intClassId);
				arguments.clear();
				return result;
			}
			case lexerIdFloat: {
				if (arguments.size() != 1) {
					throwError(
					    "Invalid call: Float expects 1 "
					    "argument, but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: Pass exactly one argument to "
					    "convert to Float (e.g., Float(value)).");
				}
				auto result = context.castPool.push(arguments[0],
				                                    DefaultClass::floatClassId);
				arguments.clear();
				return result;
			}
			case lexerIdBool: {
				if (arguments.size() != 1) {
					throwError(
					    "Invalid call: Bool expects 1 "
					    "argument, but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: Pass exactly one argument to "
					    "convert to Bool (e.g., Bool(value)).");
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
					           " were provided\nHint: Pass an object "
					           "expression to getClassId(obj).");
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
			case lexerIdarrayOf:
			case lexerIdlistOf:
			case lexerIdmutableListOf:
			case lexerIdarrayListOf:
			case lexerIdintArrayOf:
			case lexerIdfloatArrayOf:
			case lexerIddoubleArrayOf:
			case lexerIdbooleanArrayOf:
			case lexerIdstringArrayOf:
			case lexerIdlongArrayOf:
			case lexerIdbyteArrayOf: {
				if (funcObject)
					break;
				std::vector<HasClassIdNode *> vals;
				vals.reserve(arguments.size());
				for (auto *arg : arguments) {
					vals.push_back(arg);
				}
				auto arrayNode = context.createArrayPool.push(
				    line, nullptr, std::move(vals));
				arguments.clear();
				return arrayNode->resolve(in_data);
			}
			case lexerIdemptyArray:
			case lexerIdemptyList: {
				if (funcObject)
					break;
				if (!arguments.empty()) {
					throwError("Invalid call: " + context.lexerString[nameId] +
					           " expects 0 arguments, but " +
					           std::to_string(arguments.size()) +
					           " were provided\nHint: " +
					           context.lexerString[nameId] +
					           "() takes no arguments.");
				}
				auto arrayNode = context.createArrayPool.push(
				    line, nullptr, std::vector<HasClassIdNode *>());
				return arrayNode->resolve(in_data);
			}
			case lexerIdsetOf:
			case lexerIdmutableSetOf:
			case lexerIdhashSetOf:
			case lexerIdlinkedSetOf: {
				if (funcObject)
					break;
				std::vector<HasClassIdNode *> vals;
				vals.reserve(arguments.size());
				for (auto *arg : arguments) {
					vals.push_back(arg);
				}
				auto setNode =
				    context.createSetPool.push(line, nullptr, std::move(vals));
				arguments.clear();
				return setNode->resolve(in_data);
			}
			case lexerIdemptySet: {
				if (funcObject)
					break;
				if (!arguments.empty()) {
					throwError(
					    "Invalid call: emptySet expects 0 arguments, but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: emptySet() takes no arguments.");
				}
				auto setNode = context.createSetPool.push(
				    line, nullptr, std::vector<HasClassIdNode *>());
				return setNode->resolve(in_data);
			}
			case lexerIdemptyMap: {
				if (funcObject)
					break;
				if (!arguments.empty()) {
					throwError(
					    "Invalid call: emptyMap expects 0 arguments, but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: emptyMap() takes no arguments.");
				}
				auto mapNode = context.createMapPool.push(
				    line, nullptr,
				    std::vector<
				        std::pair<HasClassIdNode *, HasClassIdNode *>>());
				return mapNode->resolve(in_data);
			}
			case lexerIdpairOf: {
				if (funcObject)
					break;
				if (arguments.size() != 2) {
					throwError(
					    "Invalid call: pairOf expects 2 arguments (first, second), but " +
					    std::to_string(arguments.size()) +
					    " were provided\nHint: Use pairOf(first, second) or 'first to second'.");
				}
				auto pairNode = context.pairPool.push(line, arguments[0], arguments[1]);
				arguments.clear();
				return pairNode->resolve(in_data);
			}
			case lexerIdmapOf:
			case lexerIdmutableMapOf:
			case lexerIdhashMapOf:
			case lexerIdlinkedMapOf: {
				if (funcObject)
					break;
				if (arguments.empty()) {
					auto mapNode = context.createMapPool.push(
					    line, nullptr,
					    std::vector<
					        std::pair<HasClassIdNode *, HasClassIdNode *>>());
					return mapNode->resolve(in_data);
				}
				if (arguments.size() == 1 &&
				    arguments[0]->kind == NodeType::CREATE_MAP) {
					auto res = arguments[0];
					arguments.clear();
					return res;
				}
				bool allPairs = true;
				for (auto *arg : arguments) {
					if (arg->kind != NodeType::PAIR) {
						allPairs = false;
						break;
					}
				}
				if (allPairs) {
					std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> entries;
					entries.reserve(arguments.size());
					for (auto *arg : arguments) {
						auto *pairNode = static_cast<PairNode *>(arg);
						entries.emplace_back(pairNode->first, pairNode->second);
					}
					auto mapNode = context.createMapPool.push(
					    line, nullptr, std::move(entries));
					arguments.clear();
					return mapNode->resolve(in_data);
				}
				if (arguments.size() % 2 == 0) {
					std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>>
					    entries;
					entries.reserve(arguments.size() / 2);
					for (size_t k = 0; k < arguments.size(); k += 2) {
						entries.emplace_back(arguments[k], arguments[k + 1]);
					}
					auto mapNode = context.createMapPool.push(
					    line, nullptr, std::move(entries));
					arguments.clear();
					return mapNode->resolve(in_data);
				}
				throwError(
				    "Invalid call: " + context.lexerString[nameId] +
				    " expects Pair arguments (e.g., key to value), an even number of arguments (key, value pairs), or "
				    "a single Map, but " +
				    std::to_string(arguments.size()) +
				    " arguments were provided\nHint: Pass Pair arguments "
				    "(e.g., " +
				    context.lexerString[nameId] +
				    "(k1 to v1, k2 to v2)), key-value pairs (" +
				    context.lexerString[nameId] +
				    "(k1, v1, k2, v2)) or a map literal.");
			}
		}
	}
	return this;
}

ExprNode *CallNode::optimize(in_func) {
	if (optimized) {
		return this;
	}
	optimized = true;
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	std::string funcName;
	ClassId callerCanCallId; // never be used if is static
	uint8_t count = 0;
	static std::vector<FunctionId> *funcVec[2];

	if (nameId == lexerIdLRBRACKET)
		nameId = lexerIdget;

	bool mustInferenceGenericType = false;

	for (int i = 0; i < arguments.size(); ++i) {
		auto argument = arguments[i];
		switch (argument->kind) {
			case NodeType::CLASS_ACCESS: {
				throwError("Cannot input class at parameter " +
				           std::to_string(i + 1) +
				           "\nHint: Parameter expects an instance value, not a "
				           "class type.");
			}
			case NodeType::CALL: {
				argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
				arguments[i] = argument;
				if (argument->classId == Autolang::DefaultClass::voidClassId) {
					throwError("Cannot input Void value at parameter " +
					           std::to_string(i + 1) +
					           "\nHint: Parameter expects a value-returning "
					           "expression, not a function that returns Void.");
				}
				break;
			}
			case NodeType::CREATE_ARRAY:
			case NodeType::CREATE_MAP:
			case NodeType::CREATE_SET: {
				if (argument->classDeclaration) {
					argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
					arguments[i] = argument;
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
				argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
				arguments[i] = argument;
				break;
			}
		}
	}

	std::string name = context.lexerString[nameId];

	if (caller) {
		// Caller.funcName() => Class.funcName()
		caller = static_cast<HasClassIdNode *>(caller->optimize(in_data));
		if (caller->isNullable()) {
			if (!accessNullable) {
				throwError("You can't use '.' with nullable value, you must "
				           "use '?.'\nHint: Use safe navigation operator '?.' "
				           "when accessing members of a nullable object.");
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
			default:
				break;
		}

		auto callerClassInfo = context.classInfo[caller->classId];
		{
			auto it = callerClassInfo->allFunction.find(nameId);
			if (inputGenericArguments) {
				LexerStringId baseNameId = inputGenericArguments->baseClassLexerStringId;
				auto git = callerClassInfo->genericFunctionMap.find(baseNameId);
				if (git != callerClassInfo->genericFunctionMap.end() ||
				    it == callerClassInfo->allFunction.end()) {
					loadMemberFunctionGenerics(
					    in_data, caller->classId, name, inputGenericArguments,
					    baseNameId);
					it = callerClassInfo->allFunction.find(nameId);
				}
			} else {
				std::vector<CreateFuncNode *> *candidates = nullptr;
				auto git = callerClassInfo->genericFunctionMap.find(nameId);
				if (git != callerClassInfo->genericFunctionMap.end()) {
					candidates = &git->second;
				} else {
					auto callerClass = compile.classes[caller->classId];
					if (callerClass && callerClass->genericBaseClassId != 0) {
						auto baseClassInfo = context.classInfo[callerClass->genericBaseClassId];
						auto baseGit = baseClassInfo->genericFunctionMap.find(nameId);
						if (baseGit != baseClassInfo->genericFunctionMap.end()) {
							candidates = &baseGit->second;
						}
					}
				}
				if (candidates && !candidates->empty()) {
					bool shouldInfer = (it == callerClassInfo->allFunction.end());
					if (!shouldInfer) {
						shouldInfer = true;
						for (auto fid : it->second) {
							auto fInfo = context.functionInfo[fid];
							if (!fInfo->genericData) {
								auto fn = compile.functions[fid];
								size_t skip = (fn->functionFlags & FunctionFlags::FUNC_IS_STATIC) ? 0 : 1;
								if (arguments.size() + skip >= fInfo->parameter->defaultValuePos &&
								    arguments.size() + skip <= fInfo->parameter->parameters.size()) {
									bool argsMatch = true;
									for (size_t j = 0; j < arguments.size(); ++j) {
										uint32_t expected = fn->args[j + skip];
										uint32_t actual = arguments[j]->classId;
										if (expected != actual && expected != DefaultClass::anyClassId &&
										    !compile.classes[actual]->inheritance.get(expected)) {
											argsMatch = false;
											break;
										}
									}
									if (argsMatch) {
										shouldInfer = false;
										break;
									}
								}
							}
						}
					}
					if (shouldInfer) {
						for (auto *candidateNode : *candidates) {
							auto candidateInfo = context.functionInfo[candidateNode->id];
							if (candidateInfo->genericData) {
								size_t skip = (candidateNode->functionFlags & FunctionFlags::FUNC_IS_STATIC) ? 0 : 1;
								auto inferredDecl = tryInferGenericArguments(
								    in_data, candidateInfo->genericData,
								    candidateNode->parameter, arguments, skip, nameId);
								if (inferredDecl) {
									std::string specializedName = inferredDecl->getName(in_data);
									LexerStringId specializedNameId = context.createLexerStringIfNotExists(specializedName);
									loadMemberFunctionGenerics(
									    in_data, caller->classId, specializedName, inferredDecl, nameId);
									inputGenericArguments = inferredDecl;
									name = specializedName;
									nameId = specializedNameId;
									it = callerClassInfo->allFunction.find(specializedNameId);
									break;
								}
							}
						}
					}
				}
			}
			if (it != callerClassInfo->allFunction.end()) {
				funcVec[count++] = &it->second;
				callerCanCallId = caller->classId;
			} else {
				auto member = callerClassInfo->findAllMember(
				    in_data, line, nameId, justFindStatic);
				if (member) {
					if (arguments.empty() &&
					    member->classId != DefaultClass::functionClassId) {
						auto getPropNode = context.getPropPool.push(
						    line, member, contextCallClassId, caller, nameId,
						    false, nullable, accessNullable);
						return getPropNode->optimize(in_data);
					}
					funcObject = context.getPropPool.push(
					    line, member, caller->classId,
					    context.varPool.push(line,
					                         callerClassInfo->declarationThis,
					                         false, false),
					    nameId, true, true, false);
					matchFunction(in_data, mustInferenceGenericType);
					return this;
				}
			}
			funcName = name;
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
			return this;
		}

		{
			auto it = context.defaultClassMap.find(nameId);
			if (it == context.defaultClassMap.end()) {
				funcName = name;
				if (contextCallClassId) {
					auto callerClassInfo =
					    context.classInfo[*contextCallClassId];
					auto it = callerClassInfo->allFunction.find(nameId);
					if (inputGenericArguments) {
						LexerStringId baseNameId = inputGenericArguments->baseClassLexerStringId;
						auto git = callerClassInfo->genericFunctionMap.find(baseNameId);
						if (git != callerClassInfo->genericFunctionMap.end() ||
						    it == callerClassInfo->allFunction.end()) {
							loadMemberFunctionGenerics(
							    in_data, *contextCallClassId, name,
							    inputGenericArguments,
							    baseNameId);
							it = callerClassInfo->allFunction.find(nameId);
						}
					} else {
						auto git = callerClassInfo->genericFunctionMap.find(nameId);
						if (git != callerClassInfo->genericFunctionMap.end() && !git->second.empty()) {
							bool shouldInfer = (it == callerClassInfo->allFunction.end());
							if (!shouldInfer) {
								shouldInfer = true;
								for (auto fid : it->second) {
									if (!context.functionInfo[fid]->genericData) {
										shouldInfer = false;
										break;
									}
								}
							}
							if (shouldInfer) {
								for (auto *candidateNode : git->second) {
									auto candidateInfo = context.functionInfo[candidateNode->id];
									if (candidateInfo->genericData) {
										size_t skip = (candidateNode->functionFlags & FunctionFlags::FUNC_IS_STATIC) ? 0 : 1;
										auto inferredDecl = tryInferGenericArguments(
										    in_data, candidateInfo->genericData,
										    candidateNode->parameter, arguments, skip, nameId);
										if (inferredDecl) {
											std::string specializedName = inferredDecl->getName(in_data);
											LexerStringId specializedNameId = context.createLexerStringIfNotExists(specializedName);
											loadMemberFunctionGenerics(
											    in_data, *contextCallClassId, specializedName, inferredDecl, nameId);
											inputGenericArguments = inferredDecl;
											name = specializedName;
											nameId = specializedNameId;
											it = callerClassInfo->allFunction.find(specializedNameId);
											break;
										}
									}
								}
							}
						}
					}
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
					funcName = compile.classes[it->second]->getName(compile) +
					           '.' + name;
					caller = context.classAccessPool.push(line, it->second);
				} else {
					if (!inputGenericArguments) {
						ClassDeclaration *inferredDecl = nullptr;
						if (classInfo->primaryConstructor) {
							inferredDecl = tryInferGenericArguments(
							    in_data, classInfo->genericData,
							    classInfo->primaryConstructor->parameter,
							    arguments, 1, nameId);
						}
						if (!inferredDecl) {
							for (auto *ctor : classInfo->secondaryConstructor) {
								inferredDecl = tryInferGenericArguments(
								    in_data, classInfo->genericData,
								    ctor->parameter, arguments, 1, nameId);
								if (inferredDecl) break;
							}
						}
						if (inferredDecl) {
							std::string specializedName = inferredDecl->getName(in_data);
							ClassId specializedClassId = loadClassGenerics<false>(
							    in_data, specializedName, inferredDecl);
							inputGenericArguments = inferredDecl;
							caller = context.classAccessPool.push(line, specializedClassId);
							funcName = compile.classes[specializedClassId]->getName(compile) +
							           '.' + compile.classes[specializedClassId]->getName(compile);
						} else {
							funcName = name;
						}
					} else {
						funcName = name;
					}
				}
			}
		}

		{
			auto git = context.genericFunctionMap.find(nameId);
			if (!inputGenericArguments && !caller && git != context.genericFunctionMap.end() && !git->second.empty()) {
				auto fit = compile.funcMap.find(funcName);
				bool shouldInfer = (fit == compile.funcMap.end());
				if (!shouldInfer) {
					shouldInfer = true;
					for (auto fid : fit->second) {
						auto fInfo = context.functionInfo[fid];
						if (!fInfo->genericData) {
							auto fn = compile.functions[fid];
							size_t skip = (fn->functionFlags & FunctionFlags::FUNC_IS_STATIC) ? 0 : 1;
							if (arguments.size() + skip >= fInfo->parameter->defaultValuePos &&
							    arguments.size() + skip <= fInfo->parameter->parameters.size()) {
								bool argsMatch = true;
								for (size_t j = 0; j < arguments.size(); ++j) {
									uint32_t expected = fn->args[j + skip];
									uint32_t actual = arguments[j]->classId;
									if (expected != actual && expected != DefaultClass::anyClassId &&
									    !compile.classes[actual]->inheritance.get(expected)) {
										argsMatch = false;
										break;
									}
								}
								if (argsMatch) {
									shouldInfer = false;
									break;
								}
							}
						}
					}
				}
				if (shouldInfer) {
					for (auto *candidateNode : git->second) {
						auto candidateInfo = context.functionInfo[candidateNode->id];
						if (candidateInfo->genericData) {
							size_t skip = (candidateNode->functionFlags & FunctionFlags::FUNC_IS_STATIC) ? 0 :
							              (candidateNode->contextCallClassId.has_value() ? 1 : 0);
							auto inferredDecl = tryInferGenericArguments(
							    in_data, candidateInfo->genericData,
							    candidateNode->parameter, arguments, skip, nameId);
							if (inferredDecl) {
								std::string specializedName = inferredDecl->getName(in_data);
								LexerStringId specializedNameId = context.createLexerStringIfNotExists(specializedName);
								loadFunctionGenerics(in_data, specializedName, inferredDecl);
								inputGenericArguments = inferredDecl;
								name = specializedName;
								nameId = specializedNameId;
								funcName = specializedName;
								break;
							}
						}
					}
				}
			}
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
				if (arguments.empty() &&
				    member->classId != DefaultClass::functionClassId) {
					if (member->isGlobal) {
						auto classAccess = context.classAccessPool.push(
						    line, *contextCallClassId);
						auto getPropNode = context.getPropPool.push(
						    line, member, contextCallClassId, classAccess,
						    nameId, false, nullable, false);
						return getPropNode->optimize(in_data);
					} else {
						auto thisVar = context.varPool.push(
						    line, classInfo->declarationThis, false, false);
						auto getPropNode = context.getPropPool.push(
						    line, member, contextCallClassId, thisVar, nameId,
						    false, nullable, false);
						return getPropNode->optimize(in_data);
					}
				}
				funcObject = context.getPropPool.push(
				    line, member, contextCallClassId,
				    context.varPool.push(line, classInfo->declarationThis,
				                         false, false),
				    nameId, true, true, false);
				matchFunction(in_data, mustInferenceGenericType);
				return this;
			}
		}
	}

	// Find
	// if (allowPrefix) {
	// 	auto it = compile.funcMap.find(clazz->getName(compile) + '.' +
	// funcName); 	if (it != compile.funcMap.end()) { 		funcVec[count++] =
	// &it->second;
	// 	}
	// }

	bool ambitiousCall = false;
	// uint8_t foundIndex;
	bool found = false;
	MatchOverload first;
	if (count > 0) {
		MatchOverload second;
		int i = 0;
		int j = 0;
		// Find first function
		for (; j < count; ++j) {
			if (!match(in_data, first, *funcVec[j], i,
			           mustInferenceGenericType)) {
				i = 0;
				continue;
			}
			found = true;
			// foundIndex = j;
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
				// foundIndex = j;
				ambitiousCall = false;
				first = second;
			}
			i = 0;
		}
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
			currentFuncLog +=
			    compile.classes[argument->classId]->getName(compile);
		}
		currentFuncLog += ")";

		std::string found;
		bool isFirst1 = true;
		for (int j = 0; j < count; ++j) {
			auto &vecs = *funcVec[j];
			if (vecs.empty()) {
				printDebug("Empty");
			}
			for (auto v : vecs) {
				auto func = compile.functions[v];
				auto funcInfo = context.functionInfo[v];
				// if (func->functionFlags & FunctionFlags::FUNC_UNUSABLE) {
				// 	continue;
				// }
				if (isFirst1) {
					isFirst1 = false;
				} else {
					found += "\n";
				}
				found += funcInfo->toString(in_data);
			}
		}

		std::string targetName = context.lexerString[nameId];
		std::string bestSuggestion;
		double bestScore = 0.0;
		auto checkSuggestion = [&](const std::string &candidate) {
			double score = rapidfuzz::fuzz::ratio(targetName, candidate);
			if (score > bestScore && score >= 60.0) {
				bestScore = score;
				bestSuggestion = candidate;
			}
		};

		if (caller) {
			auto callerClassInfo = context.classInfo[caller->classId];
			if (callerClassInfo) {
				for (const auto &[fNameId, _] : callerClassInfo->allFunction) {
					checkSuggestion(context.lexerString[fNameId]);
				}
				for (auto *decl : callerClassInfo->member) {
					if (decl)
						checkSuggestion(decl->name);
				}
				for (const auto [_, declarationNode] :
				     callerClassInfo->staticMember) {
					if (declarationNode)
						checkSuggestion(declarationNode->name);
				}
			}
		} else {
			for (const auto &pair : compile.funcMap) {
				checkSuggestion(pair.first);
			}
			if (contextCallClassId) {
				auto callerClassInfo = context.classInfo[*contextCallClassId];
				if (callerClassInfo) {
					for (const auto &[fNameId, _] :
					     callerClassInfo->allFunction) {
						checkSuggestion(context.lexerString[fNameId]);
					}
					for (auto *decl : callerClassInfo->member) {
						if (decl)
							checkSuggestion(decl->name);
					}
					for (const auto &pair : callerClassInfo->staticMember) {
						if (pair.second)
							checkSuggestion(pair.second->name);
					}
				}
			}
		}

		std::string errorMsg;
		if (found.empty()) {
			if (caller) {
				errorMsg = "Cannot find function name '" + targetName +
				           "' in class '" +
				           compile.classes[caller->classId]->getName(compile) +
				           "'";
			} else {
				errorMsg = "Cannot find function name '" + targetName + "'";
			}
		} else {
			errorMsg = "Cannot find matching function overload for '" +
			           currentFuncLog + "'";
			if (caller) {
				errorMsg += " in class '" +
				            compile.classes[caller->classId]->getName(compile) +
				            "'";
			}
		}

		if (!bestSuggestion.empty() && bestSuggestion != targetName) {
			errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
		}

		if (!found.empty()) {
			errorMsg += "\nAvailable overloads:\n" + found;
			errorMsg += "\nHint: Verify argument types and count match one of "
			            "the available function overloads.";
		} else if (caller) {
			auto callerClassInfo = context.classInfo[caller->classId];
			std::string availFuncs;
			bool hasAvail = false;
			if (callerClassInfo) {
				for (const auto &[fNameId, fIds] :
				     callerClassInfo->allFunction) {
					for (auto fId : fIds) {
						auto fInfo = context.functionInfo[fId];
						if (fInfo) {
							if (hasAvail)
								availFuncs += "\n";
							availFuncs += fInfo->toString(in_data);
							hasAvail = true;
						}
					}
				}
			}
			if (hasAvail) {
				errorMsg += "\nAvailable functions in '" +
				            compile.classes[caller->classId]->getName(compile) +
				            "':\n" + availFuncs;
			} else {
				errorMsg += "\n(No functions declared in class '" +
				            compile.classes[caller->classId]->getName(compile) +
				            "')";
			}
			errorMsg += "\nHint: Check function name spelling or verify "
			            "whether it is declared in class '" +
			            compile.classes[caller->classId]->getName(compile) +
			            "'.";
		} else {
			errorMsg += "\nHint: Verify the function name is spelled correctly "
			            "and declared or imported in current scope.";
		}

		throwError(errorMsg);
	}
	if (ambitiousCall) {
		std::string message = "Ambiguous Call : " + funcName;
		for (int j = 0; j < count; ++j) {
			auto &vecs = *funcVec[j];
			for (auto v : vecs) {
				auto func = compile.functions[v];
				auto funcInfo = context.functionInfo[v];
				if (func->functionFlags & FunctionFlags::FUNC_UNUSABLE) {
					continue;
				}
				message += "\n  Founded " + funcInfo->toString(in_data);
			}
		}

		throwError(message +
		           "\nHint: Provide explicit type casts for arguments to "
		           "disambiguate the function overload.");
	}
	funcId = first.id;
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	classId = first.func->returnId;
	if (func->returnId == DefaultClass::functionClassId) {
		classDeclaration = funcInfo->returnClass;
	}
	{
		bool hasNamed = false;
		for (auto name : argumentNames) {
			if (name != 0) {
				hasNamed = true;
				break;
			}
		}
		if (hasNamed) {
			int skip = !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
			size_t totalParams = funcInfo->parameter->parameters.size();
			size_t userParamCount = totalParams - skip;
			std::vector<HasClassIdNode *> orderedArgs(userParamCount, nullptr);

			size_t posIdx = 0;
			while (posIdx < arguments.size() &&
			       (argumentNames.empty() || argumentNames[posIdx] == 0)) {
				orderedArgs[posIdx] = arguments[posIdx];
				posIdx++;
			}

			for (size_t a = posIdx; a < arguments.size(); ++a) {
				LexerStringId argNameId = argumentNames[a];
				const auto &argNameStr = context.lexerString[argNameId];
				int foundP = -1;
				for (size_t p = 0; p < userParamCount; ++p) {
					auto paramDecl = funcInfo->parameter->parameters[skip + p];
					if (paramDecl->baseName == argNameId ||
					    paramDecl->name == argNameStr) {
						foundP = static_cast<int>(p);
						break;
					}
				}
				if (foundP != -1) {
					orderedArgs[foundP] = arguments[a];
				}
			}

			for (size_t p = 0; p < userParamCount; ++p) {
				if (orderedArgs[p] == nullptr) {
					size_t targetIndex = skip + p;
					orderedArgs[p] = funcInfo->parameter->parameterDefaultValues
					    [targetIndex - funcInfo->parameter->defaultValuePos];
				}
			}

			arguments.clear();
			arguments.reserve(orderedArgs.size());
			for (auto *arg : orderedArgs) {
				arguments.push_back(arg);
			}
			argumentNames.clear();
		} else {
			int i = arguments.size() +
			        !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
			for (; i < funcInfo->parameter->parameters.size(); ++i) {
				arguments.push_back(funcInfo->parameter->parameterDefaultValues
				                        [i - funcInfo->parameter->defaultValuePos]);
			}
			argumentNames.clear();
		}
	}

	// if (mustInferenceGenericType) {
	{
		int i = func->functionFlags & FunctionFlags::FUNC_IS_STATIC ? 0 : 1;
		for (auto &argument : arguments) {
			auto funcExpectClass = funcInfo->parameter->parameters[i];
			auto funcExpectClassId = func->args[i++];
			auto funcExpectClassInfo = context.classInfo[funcExpectClassId];
			auto genericBaseClassId =
			    compile.classes[funcExpectClassId]->genericBaseClassId;
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
							if (genericBaseClassId !=
							    DefaultClass::arrayClassId) {
								auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
								if (converted) {
									argument = converted;
									first.errorNonNullIfMatchCount--;
									break;
								}
								goto notFound;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
						case NodeType::CREATE_MAP: {
							if (genericBaseClassId !=
							    DefaultClass::mapClassId) {
								auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
								if (converted) {
									argument = converted;
									first.errorNonNullIfMatchCount--;
									break;
								}
								goto notFound;
							}
							argument->classId = funcExpectClassId;
							argument->optimize(in_data);
							first.errorNonNullIfMatchCount--;
							break;
						}
						case NodeType::CREATE_SET: {
							if (genericBaseClassId !=
							    DefaultClass::setClassId) {
								if (genericBaseClassId ==
								    DefaultClass::mapClassId) {
									argument = context.createMapPool.push(
									    argument->line, nullptr,
									    std::vector<
									        std::pair<HasClassIdNode *,
									                  HasClassIdNode *>>{});
								} else {
									auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
									if (converted) {
										argument = converted;
										first.errorNonNullIfMatchCount--;
										break;
									}
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
						default:
							break;
					}
					break;
				}
				default:
					break;
			}
			if (argument->classId != funcExpectClassId &&
			    !compile.classes[argument->classId]->inheritance.get(funcExpectClassId)) {
				auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
				if (converted) {
					argument = converted;
				}
			}
		}
	}

	if (funcInfo->genericData) {
		throwError(
		    "Function " + funcName + " expects " +
		    std::to_string(funcInfo->genericData->genericDeclarations.size()) +
		    " type argument but 0 were given\nHint: Provide type arguments "
		    "explicitly (e.g., func<Type>(...)).");
	}

	if (funcInfo->inferenceNode && !funcInfo->inferenceNode->loaded) {
		funcInfo->inferenceNode->resolve(in_data);
		funcInfo->inferenceNode =
		    static_cast<ReturnNode *>(funcInfo->inferenceNode->optimize(in_data));
		funcInfo->inferenceNode->loaded = true;
	}
	
	if (nullable) {
		nullable = func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;
	}

	if (first.errorNonNullIfMatchCount) {
		throwError("Cannot pass null to non-null parameter in function '" +
		           funcName +
		           "'\nHint: Provide a non-null argument or use '?\?' fallback "
		           "operator.");
	}
	if (!(func->functionFlags & FunctionFlags::FUNC_PUBLIC) &&
	    (!contextCallClassId || *contextCallClassId != funcInfo->clazz->id))
		throwError("Cannot access private function name '" + funcName +
		           "'\nHint: Mark function as 'public' or call it within its "
		           "defining class.");
	// Add this
	if (!caller && !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		caller = context.varPool.push(
		    line, context.classInfo[callerCanCallId]->declarationThis, false,
		    false);
		caller = static_cast<HasClassIdNode *>(caller->optimize(in_data));
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
		throwError("Function '" + func->getName(compile) +
		           "' is not a static function\nHint: Call this function on an "
		           "instance of the class, or mark the function as 'static'.");
	return this;
}

void CallNode::matchFunction(in_func, ClassDeclaration *detach,
                             ClassDeclaration *value) {
	detach->template load<true>(in_data);
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
				           value->getName(in_data) +
				           "'\nHint: Ensure closure parameters and return "
				           "types match the target signature.");
			}
		}
		return;
	} else {
		throwError("Type mismatch: expected '" + detach->getName(in_data) +
		           "' but found '" + value->getName(in_data) +
		           "'\nHint: Ensure closure parameter count matches the target "
		           "function signature.");
	}
}

void CallNode::matchFunction(in_func, bool mustInferenceGenericType) {
	funcObject = static_cast<HasClassIdNode *>(funcObject->optimize(in_data));

	if (funcObject->classId != DefaultClass::functionClassId) {
		throwError(
		    "Cannot call non-function object\nHint: Only instances of Function "
		    "type or callable objects can be called as functions.");
	}

	if (!funcObject->classDeclaration) {
		throwError("Bug: Class not ensure is Function\nHint: Internal compiler "
		           "error - function object lacks Function class declaration.");
	}

	if (funcObject->isNullable()) {
		throwError(
		    "Cannot call nullable function object\nHint: Perform a null check "
		    "or use safe navigation '?.' before calling a nullable function.");
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
		           " were given\nHint: Check the number of arguments passed to "
		           "the function object.");
	}
	if (justFindStatic) {
		throwError("Cannot call non-static function from static context\nHint: "
		           "Instantiate the class first or make the function static.");
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
						argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
						break;
					}
					case NodeType::CREATE_CLOSURE: {
						auto node = static_cast<CreateClosureNode *>(argument);
						// if (node->mustInfer) {
						node->inferFrom(in_data, funcExpectClass);
						argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
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
					auto genericBaseClassId =
					    compile.classes[funcExpectClassId]->genericBaseClassId;
					switch (argument->kind) {
						case NodeType::CREATE_ARRAY: {
							if (genericBaseClassId !=
							    DefaultClass::arrayClassId) {
								auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
								if (converted) {
									argument = converted;
									break;
								}
								goto err;
							}
							argument->classId = funcExpectClassId;
							argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
							break;
						}
						case NodeType::CREATE_MAP: {
							if (genericBaseClassId !=
							    DefaultClass::mapClassId) {
								auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
								if (converted) {
									argument = converted;
									break;
								}
								goto err;
							}
							argument->classId = funcExpectClassId;
							argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
							break;
						}
						case NodeType::CREATE_SET: {
							if (genericBaseClassId !=
							    DefaultClass::setClassId) {
								if (genericBaseClassId ==
								    DefaultClass::mapClassId) {
									argument = context.createMapPool.push(
									    argument->line, nullptr,
									    std::vector<std::pair<HasClassIdNode *,
									                          HasClassIdNode *>>{});
								} else {
									auto converted = tryImplicitConversion(in_data, funcExpectClassId, argument, line);
									if (converted) {
										argument = converted;
										break;
									}
									goto err;
								}
							}
							argument->classId = funcExpectClassId;
							argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
							break;
						}
						case NodeType::WHEN:
						case NodeType::IF: {
							auto n = static_cast<NullableNode *>(argument);
							argument->classId = funcExpectClassId;
							n->nullable = funcExpectClass->nullable;
							argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
							break;
						}
						default:
							break;
					}
				}
				continue;
			}
			// Never functionClassId
			case DefaultClass::intClassId: {
				if (funcExpectClassId == Autolang::DefaultClass::floatClassId) {
					argument = static_cast<HasClassIdNode *>(
					    context.castPool
					        .push(argument, DefaultClass::floatClassId)
					        ->resolve(in_data));
					argument = static_cast<HasClassIdNode *>(argument->optimize(in_data));
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
		default:
			break;
	}
	auto argumentClassId = argument->classId;
	throwError("Object " + context.lexerString[nameId] + ": At argument " +
	           std::to_string(j) + " expected " +
	           compile.classes[*inputClass[j + 1]->classId]->getName(compile) +
	           " but " + compile.classes[argumentClassId]->getName(compile) +
	           " found\nHint: Ensure argument type matches the expected "
	           "parameter type.");
}

bool CallNode::match(in_func, MatchOverload &match,
                     std::vector<FunctionId> &functions, int &i,
                     bool mustInferenceGenericType) {
	match.score = 0;
	bool hasNamed = false;
	for (auto name : argumentNames) {
		if (name != 0) {
			hasNamed = true;
			break;
		}
	}
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
		if (match.func->functionFlags & FunctionFlags::FUNC_UNUSABLE &&
		    funcInfo->tokenIndex > tokenIndex) {
			continue;
		}
		if (funcInfo->genericData) {
			continue;
		}

		if (!hasNamed) {
			size_t argumentSize = arguments.size() + skip;
			if (argumentSize < funcInfo->parameter->defaultValuePos ||
			    argumentSize > funcInfo->parameter->parameters.size())
				continue;
			match.errorNonNullIfMatchCount = 0;
			// std::cerr << match.func->getName(compile) << " " << arguments.size()
			// << " "
			//           << argumentSize << " " << skip << " "
			//           << funcInfo->parameter->parameters.size() << " "
			//           << funcInfo->parameter->defaultValuePos << "\n";
			for (int j = 0; j < arguments.size(); ++j) {
				uint32_t inputClassId = arguments[j]->classId;
				uint32_t funcExpectClassId = match.func->args[j + skip];
				// printDebug(compile.classes[inputClassId]->getName(compile) + "
				// and " + compile.classes[funcExpectClassId]->getName(compile));

				if (funcExpectClassId == inputClassId) {
					if (funcExpectClassId == DefaultClass::functionClassId) {
						if (arguments[j]->kind != NodeType::FUNCTION_ACCESS) {
							// Function access expected context to know what
							// function auto funcInfo =
							// context.functionInfo[match.id]; std::cerr << j << " "
							//           << funcInfo->parameter->parameters[j +
							//           skip]
							//                  ->classDeclaration
							//           << " " << arguments[j]->getNodeType() <<
							//           "\n";
							if (!funcInfo->parameter->parameters[j + skip]
							         ->classDeclaration->isMatch(
							             arguments[j]->classDeclaration)) {
								goto finished;
							}
						} else {
						}
					}
					match.score += 2;
					continue;
				}
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
							auto genericBaseClassId =
							    compile.classes[funcExpectClassId]
							        ->genericBaseClassId;
							switch (argument->kind) {
								case NodeType::CREATE_ARRAY: {
									if (genericBaseClassId !=
									    DefaultClass::arrayClassId) {
										if (canImplicitConvert(in_data, funcExpectClassId, argument)) {
											break;
										}
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_MAP: {
									if (genericBaseClassId !=
									    DefaultClass::mapClassId) {
										if (canImplicitConvert(in_data, funcExpectClassId, argument)) {
											break;
										}
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_SET: {
									if (genericBaseClassId !=
									    DefaultClass::setClassId) {
										if (genericBaseClassId ==
										    DefaultClass::mapClassId) {
											break;
										}
										if (canImplicitConvert(in_data, funcExpectClassId, argument)) {
											break;
										}
										goto finished;
									}
									break;
								}
								default:
									break;
							}
						}

						++match.score;
						match.errorNonNullIfMatchCount +=
						    !funcInfo->parameter->parameters[j + skip]->nullable;
						continue;
					}
					case DefaultClass::intClassId: {
						if (funcExpectClassId ==
						    Autolang::DefaultClass::floatClassId) {
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
				if (canImplicitConvert(in_data, funcExpectClassId, arguments[j])) {
					++match.score;
					continue;
				}
				goto finished;
			}
			// Matched
			++i;
			return true;
		} else {
			size_t totalParams = funcInfo->parameter->parameters.size();
			size_t userParamCount = totalParams - skip;
			if (arguments.size() > userParamCount)
				continue;

			std::vector<HasClassIdNode *> paramAssigned(userParamCount, nullptr);
			bool matchFailed = false;

			size_t posIdx = 0;
			while (posIdx < arguments.size() &&
			       (argumentNames.empty() || argumentNames[posIdx] == 0)) {
				paramAssigned[posIdx] = arguments[posIdx];
				posIdx++;
			}

			for (size_t a = posIdx; a < arguments.size(); ++a) {
				LexerStringId argNameId = argumentNames[a];
				const auto &argNameStr = context.lexerString[argNameId];
				int foundP = -1;
				for (size_t p = 0; p < userParamCount; ++p) {
					auto paramDecl = funcInfo->parameter->parameters[skip + p];
					if (paramDecl->baseName == argNameId ||
					    paramDecl->name == argNameStr) {
						foundP = static_cast<int>(p);
						break;
					}
				}
				if (foundP == -1 || paramAssigned[foundP] != nullptr) {
					matchFailed = true;
					break;
				}
				paramAssigned[foundP] = arguments[a];
			}
			if (matchFailed)
				goto finished;

			for (size_t p = 0; p < userParamCount; ++p) {
				if (paramAssigned[p] == nullptr) {
					size_t targetIndex = skip + p;
					if (targetIndex < funcInfo->parameter->defaultValuePos) {
						matchFailed = true;
						break;
					}
				}
			}
			if (matchFailed)
				goto finished;

			match.errorNonNullIfMatchCount = 0;
			for (size_t p = 0; p < userParamCount; ++p) {
				if (paramAssigned[p] == nullptr)
					continue;
				auto argNode = paramAssigned[p];
				uint32_t inputClassId = argNode->classId;
				uint32_t funcExpectClassId = match.func->args[p + skip];

				if (funcExpectClassId == inputClassId) {
					if (funcExpectClassId == DefaultClass::functionClassId) {
						if (argNode->kind != NodeType::FUNCTION_ACCESS) {
							if (!funcInfo->parameter->parameters[p + skip]
							         ->classDeclaration->isMatch(
							             argNode->classDeclaration)) {
								goto finished;
							}
						}
					}
					match.score += 2;
					continue;
				}
				if (funcExpectClassId == DefaultClass::anyClassId) {
					++match.score;
					continue;
				}
				switch (inputClassId) {
					case DefaultClass::nullClassId: {
						if (mustInferenceGenericType) {
							auto funcExpectClassInfo =
							    context.classInfo[funcExpectClassId];
							auto genericBaseClassId =
							    compile.classes[funcExpectClassId]
							        ->genericBaseClassId;
							switch (argNode->kind) {
								case NodeType::CREATE_ARRAY: {
									if (genericBaseClassId !=
									    DefaultClass::arrayClassId) {
										if (canImplicitConvert(in_data, funcExpectClassId, argNode)) {
											break;
										}
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_MAP: {
									if (genericBaseClassId !=
									    DefaultClass::mapClassId) {
										if (canImplicitConvert(in_data, funcExpectClassId, argNode)) {
											break;
										}
										goto finished;
									}
									break;
								}
								case NodeType::CREATE_SET: {
									if (genericBaseClassId !=
									    DefaultClass::setClassId) {
										if (genericBaseClassId ==
										    DefaultClass::mapClassId) {
											break;
										}
										if (canImplicitConvert(in_data, funcExpectClassId, argNode)) {
											break;
										}
										goto finished;
									}
									break;
								}
								default:
									break;
							}
						}

						++match.score;
						match.errorNonNullIfMatchCount +=
						    !funcInfo->parameter->parameters[p + skip]->nullable;
						continue;
					}
					case DefaultClass::intClassId: {
						if (funcExpectClassId ==
						    Autolang::DefaultClass::floatClassId) {
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
				if (canImplicitConvert(in_data, funcExpectClassId, argNode)) {
					++match.score;
					continue;
				}
				goto finished;
			}
			// Matched
			++i;
			return true;
		}
	finished:;
	}
	return false;
}

void CallNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
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
				               compile.classes[classId]->memberMap.size());
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
		// std::cerr<<"At "<<func->getName(compile)<<"\n";
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
	if (isForceNonNull) {
		bytecodes.push_back(Opcode::CHECK_FORCE_NON_NULL);
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
	    line, tokenIndex, context.currentClassId, newCaller, nameId,
	    std::move(newArguments), justFindStatic, nullable, accessNullable);
	newNode->argumentNames = argumentNames;
	newNode->classId = classId;
	newNode->classDeclaration = classDeclaration;
	newNode->isForceNonNull = isForceNonNull;
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

} // namespace Autolang

#endif