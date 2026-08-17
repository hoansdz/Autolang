#ifndef CLASS_DECLARATION_CPP
#define CLASS_DECLARATION_CPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"
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
	if (!classId) {
		std::cerr << getName(in_data) << "\n";
		int *a = nullptr;
		*a = 5;
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
	newClassDeclaration->classId = classId;
	return newClassDeclaration;
}

template <bool changeGenericsClassId, bool canBeFunction>
void ClassDeclaration::load(in_func) {
	if (classId) {
		if (classId == DefaultClass::functionClassId) {
			for (size_t i = 0; i < inputClassId.size(); ++i) {
				auto *classDeclaration = inputClassId[i];
				if (classDeclaration->classId) {
					if (classDeclaration->classId ==
					    DefaultClass::functionClassId) {
						classDeclaration->load<changeGenericsClassId>(in_data);
						continue;
					}
				} else {
					if (classDeclaration->isGeneric) {
						classDeclaration->load<false>(in_data);
						classDeclaration->classId = std::nullopt;
					} else {
						classDeclaration->load<true>(in_data);
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
		if constexpr (canBeFunction) {
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
				return;
			}
		}
		{
			auto it = context.defaultClassMap.find(baseClassLexerStringId);
			if (it == context.defaultClassMap.end()) {
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
				    " type argument but 0 were given\nHint: Provide required "
				    "generic type arguments '<...>' for the class");
			}
			if (inputClassId.size() != classInfo->genericTypeId.size()) {
				throwError(
				    "'" + context.lexerString[baseClassLexerStringId] +
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
			    "' cannot have type arguments\nHint: Generic type parameters "
			    "(like T, U) cannot accept further type arguments '<...>'");
		}
	}

	if constexpr (canBeFunction) {
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
						classDeclaration->load<changeGenericsClassId>(in_data);
						if (!classDeclaration->classId) {
							throwError(
							    "Unresolved class " +
							    classDeclaration->getName(in_data) +
							    "\nHint: Ensure type parameter or class is "
							    "defined or imported");
						}
					} else if (classDeclaration->classId ==
					           DefaultClass::functionClassId) {
						classDeclaration->load<changeGenericsClassId>(in_data);
					}
				}
				std::string name = getName(in_data);
				// if (!isGenerics(in_data)) {
				loadFunctionGenerics(in_data, name, this);
				// }
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
				    " were given\nHint: Match number of type arguments with "
				    "function generic parameters");
			}
		}
	}

	{
		auto it = context.defaultClassMap.find(baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
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

	std::string name;
	if constexpr (!changeGenericsClassId) {
		bool mustInfer = true;
		std::unique_ptr<bool[]> marked(new bool[inputClassId.size()]());
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			auto *classDeclaration = inputClassId[i];
			if (!classDeclaration->classId) {
				if (classDeclaration->isGeneric) {
					classDeclaration->load<changeGenericsClassId>(in_data);
					marked[i] = true;
					mustInfer = false;
				} else {
					classDeclaration->load<true>(in_data);
				}
			} else if (classDeclaration->classId ==
			           DefaultClass::functionClassId) {
				if (classDeclaration->isGeneric) {
					classDeclaration->load<changeGenericsClassId>(in_data);
				} else {
					classDeclaration->load<true>(in_data);
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
				classDeclaration->load<true>(in_data);
				if (!classDeclaration->classId) {
					mustInfer = false;
				}
			} else if (classDeclaration->classId ==
			           DefaultClass::functionClassId) {
				classDeclaration->load<true>(in_data);
			}
		}
		if (!mustInfer)
			return;
		name = getName(in_data);
	}
	{
		auto it = compile.classMap.find(name);
		if (it != compile.classMap.end()) {
			classId = it->second;
			return;
		}
	}
	// context.genericClassMustBeLoaded[baseClassLexerStringId].push_back(this);
	classId = loadClassGenerics(in_data, name, this);
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

} // namespace Autolang

#endif