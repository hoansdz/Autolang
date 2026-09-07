#ifndef IMPLICIT_CONVERSION_HPP
#define IMPLICIT_CONVERSION_HPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/Node.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/FunctionFlags.hpp"

namespace Autolang {

inline bool canImplicitConvert(in_func, ClassId targetClassId, HasClassIdNode *value) {
	if (!value || targetClassId == DefaultClass::nullClassId ||
	    targetClassId == DefaultClass::voidClassId ||
	    targetClassId >= compile.classes.size() || !compile.classes[targetClassId]) {
		return false;
	}
	if (value->classId == targetClassId) {
		return true;
	}
	if (value->classId != DefaultClass::nullClassId &&
	    compile.classes[value->classId]->inheritance.get(targetClassId)) {
		return true;
	}

	auto targetClass = compile.classes[targetClassId];
	auto itName = context.lexerStringMap.find(targetClass->getName(compile));
	if (itName == context.lexerStringMap.end()) {
		return false;
	}
	LexerStringId classNameId = itName->second;
	auto targetClassInfo = context.classInfo[targetClassId];
	if (!targetClassInfo) {
		return false;
	}

	auto itFuncs = targetClassInfo->allFunction.find(classNameId);
	if (itFuncs == targetClassInfo->allFunction.end()) {
		return false;
	}

	for (auto funcId : itFuncs->second) {
		auto func = compile.functions[funcId];
		if (!(func->functionFlags & FunctionFlags::FUNC_IS_IMPLICIT)) {
			continue;
		}
		if (func->functionFlags & FunctionFlags::FUNC_UNUSABLE) {
			continue;
		}
		auto funcInfo = context.functionInfo[funcId];
		if (!funcInfo || !funcInfo->parameter) {
			continue;
		}

		// 1-arg match
		if (func->argSize >= 2) {
			size_t requiredUserParams = (funcInfo->parameter->defaultValuePos > 1)
			                                ? (funcInfo->parameter->defaultValuePos - 1)
			                                : 0;
			if (requiredUserParams <= 1) {
				ClassId expectedClassId = func->args[1];
				auto paramDecl = funcInfo->parameter->parameters[1];
				if (value->isNullable() && !paramDecl->nullable) {
					continue;
				}
				if (expectedClassId == value->classId ||
				    expectedClassId == DefaultClass::anyClassId) {
					return true;
				}
				if (expectedClassId == DefaultClass::floatClassId &&
				    value->classId == DefaultClass::intClassId) {
					return true;
				}
				if (value->classId != DefaultClass::nullClassId &&
				    compile.classes[value->classId]->inheritance.get(expectedClassId)) {
					return true;
				}
				if (value->classId == DefaultClass::nullClassId) {
					auto expectedGenericBase =
					    compile.classes[expectedClassId]->genericBaseClassId;
					if (value->kind == NodeType::CREATE_MAP &&
					    expectedGenericBase == DefaultClass::mapClassId) {
						return true;
					}
					if (value->kind == NodeType::CREATE_ARRAY &&
					    expectedGenericBase == DefaultClass::arrayClassId) {
						return true;
					}
					if (value->kind == NodeType::CREATE_SET) {
						auto setNode = static_cast<CreateSetNode *>(value);
						if (setNode->values.empty()) {
							if (expectedGenericBase == DefaultClass::mapClassId ||
							    expectedGenericBase == DefaultClass::setClassId) {
								return true;
							}
						} else if (expectedGenericBase == DefaultClass::setClassId) {
							return true;
						}
					}
				}
			}
		}

		// 0-arg match
		if (func->argSize == 1) {
			if (value->kind == NodeType::CREATE_SET &&
			    static_cast<CreateSetNode *>(value)->values.empty()) {
				return true;
			}
			if (value->kind == NodeType::CREATE_MAP &&
			    static_cast<CreateMapNode *>(value)->values.empty()) {
				return true;
			}
			if (value->kind == NodeType::CREATE_ARRAY &&
			    static_cast<CreateArrayNode *>(value)->values.empty()) {
				return true;
			}
		}
	}

	return false;
}

inline HasClassIdNode *tryImplicitConversion(in_func, ClassId targetClassId,
                                             HasClassIdNode *value, uint32_t line,
                                             std::optional<ClassId> contextCallClassId = std::nullopt) {
	if (!value || targetClassId == DefaultClass::nullClassId ||
	    targetClassId == DefaultClass::voidClassId ||
	    targetClassId >= compile.classes.size() || !compile.classes[targetClassId]) {
		return nullptr;
	}
	if (value->classId == targetClassId) {
		return value;
	}
	if (value->classId != DefaultClass::nullClassId &&
	    compile.classes[value->classId]->inheritance.get(targetClassId)) {
		return value;
	}

	auto targetClass = compile.classes[targetClassId];
	auto itName = context.lexerStringMap.find(targetClass->getName(compile));
	if (itName == context.lexerStringMap.end()) {
		return nullptr;
	}
	LexerStringId classNameId = itName->second;
	auto targetClassInfo = context.classInfo[targetClassId];
	if (!targetClassInfo) {
		return nullptr;
	}

	auto itFuncs = targetClassInfo->allFunction.find(classNameId);
	if (itFuncs == targetClassInfo->allFunction.end()) {
		return nullptr;
	}

	FunctionId bestFuncId = UINT32_MAX;
	bool isZeroArg = false;
	ClassId bestExpectedParamClassId = 0;

	for (auto funcId : itFuncs->second) {
		auto func = compile.functions[funcId];
		if (!(func->functionFlags & FunctionFlags::FUNC_IS_IMPLICIT)) {
			continue;
		}
		if (func->functionFlags & FunctionFlags::FUNC_UNUSABLE) {
			continue;
		}
		auto funcInfo = context.functionInfo[funcId];
		if (!funcInfo || !funcInfo->parameter) {
			continue;
		}

		// 1-arg match
		if (func->argSize >= 2) {
			size_t requiredUserParams = (funcInfo->parameter->defaultValuePos > 1)
			                                ? (funcInfo->parameter->defaultValuePos - 1)
			                                : 0;
			if (requiredUserParams <= 1) {
				ClassId expectedClassId = func->args[1];
				auto paramDecl = funcInfo->parameter->parameters[1];
				if (value->isNullable() && !paramDecl->nullable) {
					continue;
				}
				bool matched = false;
				if (expectedClassId == value->classId ||
				    expectedClassId == DefaultClass::anyClassId) {
					matched = true;
				} else if (expectedClassId == DefaultClass::floatClassId &&
				           value->classId == DefaultClass::intClassId) {
					matched = true;
				} else if (value->classId != DefaultClass::nullClassId &&
				           compile.classes[value->classId]->inheritance.get(expectedClassId)) {
					matched = true;
				} else if (value->classId == DefaultClass::nullClassId) {
					auto expectedGenericBase =
					    compile.classes[expectedClassId]->genericBaseClassId;
					if (value->kind == NodeType::CREATE_MAP &&
					    expectedGenericBase == DefaultClass::mapClassId) {
						matched = true;
					} else if (value->kind == NodeType::CREATE_ARRAY &&
					           expectedGenericBase == DefaultClass::arrayClassId) {
						matched = true;
					} else if (value->kind == NodeType::CREATE_SET) {
						auto setNode = static_cast<CreateSetNode *>(value);
						if (setNode->values.empty()) {
							if (expectedGenericBase == DefaultClass::mapClassId ||
							    expectedGenericBase == DefaultClass::setClassId) {
								matched = true;
							}
						} else if (expectedGenericBase == DefaultClass::setClassId) {
							matched = true;
						}
					}
				}

				if (matched) {
					bestFuncId = funcId;
					isZeroArg = false;
					bestExpectedParamClassId = expectedClassId;
					break;
				}
			}
		}

		// 0-arg match
		if (func->argSize == 1 && bestFuncId == UINT32_MAX) {
			bool isEmptyColl = false;
			if (value->kind == NodeType::CREATE_SET &&
			    static_cast<CreateSetNode *>(value)->values.empty()) {
				isEmptyColl = true;
			} else if (value->kind == NodeType::CREATE_MAP &&
			           static_cast<CreateMapNode *>(value)->values.empty()) {
				isEmptyColl = true;
			} else if (value->kind == NodeType::CREATE_ARRAY &&
			           static_cast<CreateArrayNode *>(value)->values.empty()) {
				isEmptyColl = true;
			}
			if (isEmptyColl) {
				bestFuncId = funcId;
				isZeroArg = true;
			}
		}
	}

	if (bestFuncId == UINT32_MAX) {
		return nullptr;
	}

	if (isZeroArg) {
		auto callNode = context.callNodePool.push(
		    line, 0, contextCallClassId, nullptr, classNameId,
		    std::vector<HasClassIdNode *>{}, false, false, false);
		callNode->resolve(in_data);
		auto opt = callNode->optimize(in_data);
		return static_cast<HasClassIdNode *>(opt);
	}

	HasClassIdNode *argValue = value;
	if (argValue->classId == DefaultClass::nullClassId) {
		auto expectedGenericBase =
		    compile.classes[bestExpectedParamClassId]->genericBaseClassId;
		if (argValue->kind == NodeType::CREATE_SET) {
			auto setNode = static_cast<CreateSetNode *>(argValue);
			if (setNode->values.empty() &&
			    expectedGenericBase == DefaultClass::mapClassId) {
				auto newMapNode = context.createMapPool.push(
				    argValue->line, nullptr,
				    std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>>{});
				newMapNode->classId = bestExpectedParamClassId;
				argValue = static_cast<HasClassIdNode *>(newMapNode->optimize(in_data));
			} else {
				argValue->classId = bestExpectedParamClassId;
				argValue = static_cast<HasClassIdNode *>(argValue->optimize(in_data));
			}
		} else {
			argValue->classId = bestExpectedParamClassId;
			argValue = static_cast<HasClassIdNode *>(argValue->optimize(in_data));
		}
	} else if (argValue->classId == DefaultClass::intClassId &&
	           bestExpectedParamClassId == DefaultClass::floatClassId) {
		argValue = static_cast<HasClassIdNode *>(
		    context.castPool.push(argValue, DefaultClass::floatClassId)->optimize(in_data));
	}

	auto callNode = context.callNodePool.push(
	    line, 0, contextCallClassId, nullptr, classNameId,
	    std::vector<HasClassIdNode *>{argValue}, false, false, false);
	callNode->resolve(in_data);
	auto opt = callNode->optimize(in_data);
	return static_cast<HasClassIdNode *>(opt);
}

} // namespace Autolang

#endif
