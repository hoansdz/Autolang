#ifndef CLASS_DECLARATION_CPP
#define CLASS_DECLARATION_CPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "ParserContext.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/FunctionInfo.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/CompiledProgram.hpp"
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

ClassDeclaration::ClassDeclaration() { mode = ParserContext::mode; }

void ClassDeclaration::throwError(std::string message) {
	ParserContext::mode = mode;
	throw ParserError(line, message);
}

bool ClassDeclaration::isSame(ClassDeclaration *classDeclaration) {
	if (classId != classDeclaration->classId ||
	    inputClassId.size() != classDeclaration->inputClassId.size()) {
		return false;
	}
	for (int i = 0; i < inputClassId.size(); ++i) {
		if (inputClassId[i]->classId !=
		    classDeclaration->inputClassId[i]->classId) {
			return false;
		}
	}
	return true;
}

bool ClassDeclaration::isMatch(ClassDeclaration *classDeclaration) {
	if (inputClassId.size() != classDeclaration->inputClassId.size()) {
		return false;
	}
	return true;
	// if (classId != classDeclaration->classId ||
	//     inputClassId.size() != classDeclaration->inputClassId.size()) {
	// 	return false;
	// }
	// for (int i = 0; i < inputClassId.size(); ++i) {
	// 	if (!inputClassId[i] || !classDeclaration->inputClassId[i]) {
	// 		continue;
	// 	}
	// 	if (inputClassId[i]->classId !=
	// 	    classDeclaration->inputClassId[i]->classId) {
	// 		return false;
	// 	}
	// 	if (inputClassId[i]->classId == DefaultClass::functionClassId &&
	// 	    !inputClassId[i]->isMatch(classDeclaration->inputClassId[i])) {
	// 		return false;
	// 	}
	// }
	// return true;
}

ClassDeclaration *ClassDeclaration::copy(in_func) {
	if (baseClassLexerStringId == lexerIdVoid) {
		classId = DefaultClass::voidClassId;
		return this;
	}
	if (!classId) {
		load<true>(in_data);
	}
	if (!classId) {
		std::cerr << getName(in_data) << "\n";
		throwError(
		    "Cannot copy class declaration because class not exists\nHint: "
		    "Ensure target class is declared before copying its declaration");
	}
	if (!isGeneric) {
		return this;
	}
	auto newClassDeclaration = context.classDeclarationAllocator.push();
	newClassDeclaration->baseClassLexerStringId = baseClassLexerStringId;
	newClassDeclaration->isGeneric = isGeneric;
	newClassDeclaration->mode = mode;
	newClassDeclaration->line = line;
	newClassDeclaration->nullable = nullable;
	newClassDeclaration->isGenericDeclaration = isGenericDeclaration;
	newClassDeclaration->mustInference = mustInference;
	if (isGeneric || classId == DefaultClass::functionClassId) {
		newClassDeclaration->inputClassId.reserve(inputClassId.size());
		for (auto *inputClass : inputClassId) {
			newClassDeclaration->inputClassId.push_back(
			    inputClass->copy(in_data));
		}
	}
	if (isGeneric && !isGenericDeclaration && classId != DefaultClass::functionClassId) {
		newClassDeclaration->classId = std::nullopt;
	} else {
		newClassDeclaration->classId = classId;
	}
	newClassDeclaration->isFunction = isFunction;
	return newClassDeclaration;
}

template <bool changeGenericsClassId, bool canBeFunction, bool mustBeFunction>
void ClassDeclaration::onLoadTypealias(in_func, TypealiasData *typealias) {
	if (typealias->genericData) {
		if (typealias->genericData->genericDeclarations.size() !=
		    inputClassId.size()) {
			throwError(
			    "Typealias " + context.lexerString[baseClassLexerStringId] +
			    " expects " +
			    std::to_string(
			        typealias->genericData->genericDeclarations.size()) +
			    " type argument but " + std::to_string(inputClassId.size()) +
			    " were given\nHint: Check number of type "
			    "arguments "
			    "passed to the generic typealias");
		}
	} else {
		if (inputClassId.size()) {
			throwError("Typealias " +
			           context.lexerString[baseClassLexerStringId] +
			           " expects no type argument but " +
			           std::to_string(inputClassId.size()) +
			           " were given\nHint: "
			           "Remove type arguments from typealias");
		}
	}
	switch (typealias->state) {
		case TypealiasState::TAS_UNVISITED: {
			typealias->state = TAS_VISITING;
			++context.typealiasDepth;
			// if (!typealias->classDeclaration->classId) {
			if (typealias->genericData) {
				for (size_t i = 0; i < inputClassId.size(); ++i) {
					auto &genericDeclaration =
					    typealias->genericData->genericDeclarations[i];
					auto *inputClass = inputClassId[i];
					ClassId inputClassId = *inputClass->classId;
					// Change generics type
					genericDeclaration->classId = inputClassId;
					genericDeclaration->nullable = inputClass->nullable;
					ClassDeclaration *newClassDeclaration;
					if (inputClass->isGeneric) {
						newClassDeclaration =
						    context.classDeclarationAllocator.push();
						newClassDeclaration->classId = inputClassId;
						newClassDeclaration->nullable = inputClass->nullable;
						newClassDeclaration->line = genericDeclaration->line;
						if (inputClassId == DefaultClass::functionClassId) {
							newClassDeclaration->inputClassId.reserve(
							    inputClass->inputClassId.size());
							for (auto classDeclaration :
							     inputClass->inputClassId) {
								newClassDeclaration->inputClassId.push_back(
								    classDeclaration->copy(in_data));
							}
						}
					} else {
						newClassDeclaration = inputClass;
					}

					if (genericDeclaration->condition) {
						auto &condition = *genericDeclaration->condition;
						// if (condition.condition ==
						// GenericDeclarationCondition::MUST_EXTENDS) {
						if (!condition.classDeclaration->classId) {
							condition.classDeclaration->template load<true>(in_data);
							if (!condition.classDeclaration->classId) {
								condition.classDeclaration->throwError(
								    "Unresolved " +
								    condition.classDeclaration->getName(
								        in_data) +
								    "\nHint: Ensure type constraint class "
								    "is defined or imported");
							}
						} else if (condition.classDeclaration->classId ==
						           DefaultClass::functionClassId) {
							condition.classDeclaration->template load<true>(in_data);
						}
						context.checkValidateExtends[genericDeclaration]
						    .push_back(newClassDeclaration);
						// }
					}
					for (auto *classDeclaration :
					     genericDeclaration->allClassDeclarations) {
						classDeclaration->classId = inputClassId;
						classDeclaration->inputClassId =
						    newClassDeclaration->inputClassId;
						if (classDeclaration->mustInference) {
							classDeclaration->nullable = inputClass->nullable;
							// classDeclaration->mustInference = false;
						}
						classDeclaration->baseClassLexerStringId =
						    inputClass->baseClassLexerStringId;
					}
				}
			}
			typealias->classDeclaration->template load<true>(in_data);
			// }
			if (context.typealiasTraceIndex) {
				if (--context.typealiasTraceIndex) {
					context.typealiasStackTrace[context.typealiasTraceIndex] =
					    baseClassLexerStringId;
					typealias->state = TAS_UNVISITED;
					return;
				} else {
					context.typealiasStackTrace[0] = baseClassLexerStringId;
					std::string errorMsg = "Circular typealias detected: ";
					bool first = true;
					for (auto nameId : context.typealiasStackTrace) {
						if (!first)
							errorMsg += " -> ";
						errorMsg += context.lexerString[nameId];
						first = false;
					}
					context.typealiasDepth = 0;
					typealias->state = TAS_UNVISITED;
					throwError(errorMsg);
				}
			} else {
				--context.typealiasDepth;
				typealias->state = TAS_VISITED;
			}
			break;
		}
		case TypealiasState::TAS_VISITING: {
			context.typealiasTraceIndex = context.typealiasDepth++;
			context.typealiasStackTrace.resize(context.typealiasDepth);
			context.typealiasStackTrace[context.typealiasTraceIndex] =
			    baseClassLexerStringId;
			return;
		}
		default:
			break;
	}
	auto *classDeclaration = typealias->classDeclaration->copy(in_data);
	if (!classDeclaration->classId) {
		classDeclaration->template load<true>(in_data);
	}
	classId = classDeclaration->classId;
	baseClassLexerStringId = classDeclaration->baseClassLexerStringId;
	inputClassId = classDeclaration->inputClassId;
	if (classDeclaration->nullable) {
		nullable = true;
	}
	if (typealias->genericData || classDeclaration->isGeneric) {
		typealias->state = TAS_UNVISITED;
		typealias->classDeclaration->classId = std::nullopt;
	}
}

template <bool changeGenericsClassId, bool canBeFunction, bool isLazy,
          bool mustBeFunction>
void ClassDeclaration::load(in_func) {
	if (!classId && baseClassLexerStringId == lexerIdFunction) {
		classId = DefaultClass::functionClassId;
	}
	if (classId) {
		if (classId == DefaultClass::functionClassId) {
			for (size_t i = 0; i < inputClassId.size(); ++i) {
				auto *classDeclaration = inputClassId[i];
				if (classDeclaration->classId) {
					if (classDeclaration->classId ==
					    DefaultClass::functionClassId) {
						classDeclaration->template load<changeGenericsClassId>(in_data);
						continue;
					}
				} else {
					if (classDeclaration->isGeneric) {
						classDeclaration->template load<false>(in_data);
						classDeclaration->classId = std::nullopt;
					} else {
						classDeclaration->template load<true>(in_data);
					}
					continue;
				}
			}
		}
		return;
	}
	if (inputClassId.empty()) {
		if (isGenericDeclaration)
			return;
		if constexpr (canBeFunction || mustBeFunction) {
			auto it = compile.funcMap.find(
			    context.lexerString[baseClassLexerStringId]);
			if (it != compile.funcMap.end()) {
				// Generics no overload
				auto funcId = it->second[0];
				auto func = compile.functions[funcId];
				auto funcInfo = context.functionInfo[funcId];
				if (funcInfo->genericData) {
					throwError(
					    "'" + context.lexerString[baseClassLexerStringId] +
					    "' expects " +
					    std::to_string(
					        funcInfo->genericData->genericDeclarations.size()) +
					    " type argument but 0 were given\nHint: Provide "
					    "required generic type arguments '<...>' for the "
					    "function");
				}
				if (inputClassId.size() !=
				    funcInfo->genericData->genericDeclarations.size()) {
					throwError(
					    "Function '" +
					    context.lexerString[baseClassLexerStringId] +
					    "' expects " +
					    std::to_string(funcInfo->genericTypeId.size()) +
					    " type argument but " +
					    std::to_string(inputClassId.size()) +
					    " were given\nHint: Check number of type arguments "
					    "passed to the generic function");
				}
				isFunction = true;
				return;
			} else if constexpr (mustBeFunction) {
				std::string targetName =
				    context.lexerString[baseClassLexerStringId];
				std::string bestSuggestion;
				double bestScore = 0.0;
				auto checkSuggestion = [&](const std::string &candidate) {
					double score =
					    rapidfuzz::fuzz::ratio(targetName, candidate);
					if (score > bestScore && score >= 60.0) {
						bestScore = score;
						bestSuggestion = candidate;
					}
				};
				for (const auto &[name, _] : compile.funcMap) {
					checkSuggestion(name);
				}
				std::string errorMsg =
				    "Cannot find function '" + targetName + "'";
				if (!bestSuggestion.empty() && bestSuggestion != targetName) {
					errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
				}
				errorMsg += "\nHint: Ensure the function name is spelled "
				            "correctly and declared or imported.";
				throwError(errorMsg);
			}
		}
		{
			if (baseClassLexerStringId == lexerIdVoid) {
				classId = DefaultClass::voidClassId;
				return;
			}
			auto it = context.defaultClassMap.find(baseClassLexerStringId);
			if (it == context.defaultClassMap.end()) {
				auto typealiasResult =
				    context.typealiasMap.find(baseClassLexerStringId);
				if (typealiasResult != context.typealiasMap.end()) {
					auto typealias = typealiasResult->second;
					onLoadTypealias<changeGenericsClassId, canBeFunction,
					                mustBeFunction>(in_data, typealias);
					return;
				}
				std::string targetName =
				    context.lexerString[baseClassLexerStringId];
				std::string bestSuggestion;
				double bestScore = 0.0;
				auto checkSuggestion = [&](const std::string &candidate) {
					double score =
					    rapidfuzz::fuzz::ratio(targetName, candidate);
					if (score > bestScore && score >= 60.0) {
						bestScore = score;
						bestSuggestion = candidate;
					}
				};
				for (const auto [name, classId] : context.defaultClassMap) {
					checkSuggestion(context.lexerString[name]);
				}
				for (const auto &clazz : compile.classes) {
					if (clazz) {
						checkSuggestion(clazz->getName(compile));
					}
				}

				std::string errorMsg =
				    "Cannot find class name '" + targetName + "'";
				if (!bestSuggestion.empty() && bestSuggestion != targetName) {
					errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
				}
				errorMsg += "\nHint: Verify that the class name is spelled "
				            "correctly and defined or imported.";
				throwError(errorMsg);
			}
			classId = it->second;
			auto classInfo = context.classInfo[*classId];
			if (classInfo->genericData) {
				throwError(
				    "'" + context.lexerString[baseClassLexerStringId] +
				    "' expects " +
				    std::to_string(
				        classInfo->genericData->genericDeclarations.size()) +
				    " type argument but 0 were given\nHint: Provide "
				    "required "
				    "generic type arguments '<...>' for the class");
			}
			if (inputClassId.size() != classInfo->genericTypeId.size()) {
				throwError("'" + context.lexerString[baseClassLexerStringId] +
				           "' expects " +
				           std::to_string(classInfo->genericTypeId.size()) +
				           " type argument but " +
				           std::to_string(inputClassId.size()) +
				           " were given\nHint: Check number of type arguments "
				           "passed to the generic class");
			}
			return;
		}
	}
	{
		if (isGenericDeclaration) {
			throwError(
			    "Type parameter '" +
			    context.lexerString[baseClassLexerStringId] +
			    "' cannot have type arguments\nHint: Generic type "
			    "parameters "
			    "(like T, U) cannot accept further type arguments '<...>'");
		}
	}

	if constexpr (canBeFunction || mustBeFunction) {
		auto it =
		    compile.funcMap.find(context.lexerString[baseClassLexerStringId]);
		// std::cerr << context.lexerString[baseClassLexerStringId] << "\n";
		if (it != compile.funcMap.end()) {
			for (FunctionId funcId : it->second) {
				auto func = compile.functions[funcId];
				auto funcInfo = context.functionInfo[funcId];
				if (!funcInfo->genericData) {
					throwError(
					    "'" + context.lexerString[baseClassLexerStringId] +
					    "' isn't generic function\nHint: Do not pass type "
					    "arguments '<...>' to a non-generic function");
				}
				if (inputClassId.size() !=
				    funcInfo->genericData->genericDeclarations.size()) {
					continue;
				}
				for (size_t i = 0; i < inputClassId.size(); ++i) {
					auto *classDeclaration = inputClassId[i];
					if (!classDeclaration->classId) {
						classDeclaration->template load<changeGenericsClassId>(in_data);
						if (!classDeclaration->classId) {
							throwError(
							    "Unresolved class " +
							    classDeclaration->getName(in_data) +
							    "\nHint: Ensure type parameter or class is "
							    "defined or imported");
						}
					} else if (classDeclaration->classId ==
					           DefaultClass::functionClassId) {
						classDeclaration->template load<changeGenericsClassId>(in_data);
					}
				}
				std::string name = getName(in_data);
				// if (!isGenerics(in_data)) {
				loadFunctionGenerics(in_data, name, this);
				// }
				isFunction = true;
				return;
			}
			auto funcInfo = context.functionInfo[it->second[0]];
			if (inputClassId.size() !=
			    funcInfo->genericData->genericDeclarations.size()) {
				throwError(
				    "Function '" + context.lexerString[baseClassLexerStringId] +
				    "' expects " +
				    std::to_string(
				        funcInfo->genericData->genericDeclarations.size()) +
				    " type argument but " +
				    std::to_string(inputClassId.size()) +
				    " were given\nHint: Match number of type arguments "
				    "with "
				    "function generic parameters");
			}
		} else if constexpr (mustBeFunction) {
			std::string targetName =
			    context.lexerString[baseClassLexerStringId];
			std::string bestSuggestion;
			double bestScore = 0.0;
			auto checkSuggestion = [&](const std::string &candidate) {
				double score =
				    rapidfuzz::fuzz::ratio(targetName, candidate);
				if (score > bestScore && score >= 60.0) {
					bestScore = score;
					bestSuggestion = candidate;
				}
			};
			for (const auto &[name, _] : compile.funcMap) {
				checkSuggestion(name);
			}
			std::string errorMsg =
			    "Cannot find function '" + targetName + "'";
			if (!bestSuggestion.empty() && bestSuggestion != targetName) {
				errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
			}
			errorMsg += "\nHint: Ensure the function name is spelled "
			            "correctly or defined before use.";
			throwError(errorMsg);
		}
	}
	if (isFunction) {
		return;
	}

	TypealiasData *typealias = nullptr;

	{
		auto it = context.defaultClassMap.find(baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
			auto typealiasResult =
			    context.typealiasMap.find(baseClassLexerStringId);
			if (typealiasResult != context.typealiasMap.end()) {
				typealias = typealiasResult->second;
				goto continueLoad;
			}
			std::string targetName =
			    context.lexerString[baseClassLexerStringId];
			std::string bestSuggestion;
			double bestScore = 0.0;
			auto checkSuggestion = [&](const std::string &candidate) {
				double score = rapidfuzz::fuzz::ratio(targetName, candidate);
				if (score > bestScore && score >= 60.0) {
					bestScore = score;
					bestSuggestion = candidate;
				}
			};
			for (const auto &pair : context.defaultClassMap) {
				checkSuggestion(context.lexerString[pair.first]);
			}
			for (const auto &clazz : compile.classes) {
				if (clazz) {
					checkSuggestion(clazz->getName(compile));
				}
			}

			std::string errorMsg =
			    "Cannot find class name '" + targetName + "'";
			if (!bestSuggestion.empty() && bestSuggestion != targetName) {
				errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
			}
			errorMsg +=
			    "\nHint: Ensure target class name is defined or imported.";
			throwError(errorMsg);
		}
	}

	continueLoad:;

	std::string name;
	if constexpr (!changeGenericsClassId) {
		bool mustInfer = true;
		std::unique_ptr<bool[]> marked(new bool[inputClassId.size()]());
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			auto *classDeclaration = inputClassId[i];
			if (!classDeclaration->classId) {
				if (classDeclaration->isGeneric) {
					classDeclaration->template load<changeGenericsClassId>(in_data);
					marked[i] = true;
					mustInfer = false;
				} else {
					classDeclaration->template load<true>(in_data);
				}
			} else if (classDeclaration->classId ==
			           DefaultClass::functionClassId) {
				if (classDeclaration->isGeneric) {
					classDeclaration->template load<changeGenericsClassId>(in_data);
				} else {
					classDeclaration->template load<true>(in_data);
				}
			}
		}
		name = getName(in_data);
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			if (marked[i]) {
				inputClassId[i]->classId = std::nullopt;
			}
		}
		if (!mustInfer)
			return;
	} else {
		bool mustInfer = true;
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			auto *classDeclaration = inputClassId[i];
			if (!classDeclaration->classId) {
				classDeclaration->template load<true>(in_data);
				if (!classDeclaration->classId) {
					mustInfer = false;
				}
			} else if (classDeclaration->classId ==
			           DefaultClass::functionClassId) {
				classDeclaration->template load<true>(in_data);
			}
		}
		if (!mustInfer)
			return;
		name = getName(in_data);
	}
	if (typealias) {
		onLoadTypealias<changeGenericsClassId, canBeFunction, mustBeFunction>(
		    in_data, typealias);
		return;
	}
	{
		auto it = compile.classMap.find(name);
		if (it != compile.classMap.end()) {
			classId = it->second;
			return;
		}
	}
	// context.genericClassMustBeLoaded[baseClassLexerStringId].push_back(this);
	classId = Autolang::loadClassGenerics<isLazy>(in_data, name, this);
	auto classInfo = context.classInfo[*classId];
	if (inputClassId.size() != classInfo->genericTypeId.size()) {
		// int* x = nullptr; *x = 5;
		throwError(
		    "'" + context.lexerString[baseClassLexerStringId] + "' expects " +
		    std::to_string(classInfo->genericTypeId.size()) +
		    " type argument but " + std::to_string(inputClassId.size()) +
		    " were given\nHint: Match number of type arguments with class "
		    "generic parameters");
	}
}

template <bool addNullable> std::string ClassDeclaration::getName(in_func) {
	if (classId) {
		if (classId == DefaultClass::functionClassId) {
			std::string result = "(";
			bool isFirst = true;
			for (int i = 1; i < inputClassId.size(); ++i) {
				auto inputClass = inputClassId[i];
				if (isFirst) {
					isFirst = false;
				} else {
					result += ", ";
				}
				if (!inputClass) {
					result += "Null";
					continue;
				}
				result += inputClass->getName(in_data);
			}
			result += ")->";
			if (inputClassId[0]) {
				result += inputClassId[0]->getName(in_data);
			} else {
				result += "Null";
			}
			if (nullable)
				result += "?";
			return result;
		}
		if constexpr (!addNullable) {
			return compile.classes[*classId]->getName(compile);
		}
		if (nullable) {
			return compile.classes[*classId]->getName(compile) + "?";
		}
		return compile.classes[*classId]->getName(compile);
	}
	if (inputClassId.empty()) {
		if constexpr (!addNullable) {
			return context.lexerString[baseClassLexerStringId];
		}
		if (nullable) {
			return context.lexerString[baseClassLexerStringId] + "?";
		}
		return context.lexerString[baseClassLexerStringId];
	}
	std::string name = context.lexerString[baseClassLexerStringId] + "<";
	bool isFirst = true;
	for (auto classDeclaration : inputClassId) {
		if (!isFirst) {
			name += ",";
		} else {
			isFirst = false;
		}
		name += classDeclaration->getName<true>(in_data);
	}
	if constexpr (!addNullable) {
		return name + ">";
	}
	if (nullable) {
		name += ">?";
	} else {
		name += ">";
	}
	return name;
}

template void ClassDeclaration::load<false, false>(in_func);
template void ClassDeclaration::load<true, false>(in_func);
template void ClassDeclaration::load<false, true>(in_func);
template void ClassDeclaration::load<true, true>(in_func);
template void ClassDeclaration::load<false, false, true>(in_func);
template void ClassDeclaration::load<true, false, true>(in_func);
template void ClassDeclaration::load<false, true, true>(in_func);
template void ClassDeclaration::load<true, true, true>(in_func);

template void ClassDeclaration::load<false, false, false, true>(in_func);
template void ClassDeclaration::load<true, false, false, true>(in_func);
template void ClassDeclaration::load<false, true, false, true>(in_func);
template void ClassDeclaration::load<true, true, false, true>(in_func);
template void ClassDeclaration::load<false, false, true, true>(in_func);
template void ClassDeclaration::load<true, false, true, true>(in_func);
template void ClassDeclaration::load<false, true, true, true>(in_func);
template void ClassDeclaration::load<true, true, true, true>(in_func);

template std::string ClassDeclaration::getName<false>(in_func);
template std::string ClassDeclaration::getName<true>(in_func);

} // namespace Autolang

#endif