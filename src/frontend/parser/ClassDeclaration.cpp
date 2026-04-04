#ifndef CLASS_DECLARATION_CPP
#define CLASS_DECLARATION_CPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ClassDeclaration::ClassDeclaration() { mode = ParserContext::mode; }

void ClassDeclaration::throwError(std::string message) {
	ParserContext::mode = mode;
	throw ParserError(line, message);
}

ClassDeclaration *ClassDeclaration::copy(in_func) {
	if (!classId) {
		int *a = nullptr;
		*a = 5;
		throwError("Cannot copy class declaration because class not exists");
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
				classDeclaration->load<true>(in_data);
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
					    " type argument but 0 were given");
				}
				if (inputClassId.size() !=
				    funcInfo->genericData->genericDeclarations.size()) {
					throwError(
					    "'" + context.lexerString[baseClassLexerStringId] +
					    "' expects " +
					    std::to_string(funcInfo->genericTypeId.size()) +
					    " type argument but " +
					    std::to_string(inputClassId.size()) + " were given");
				}
				return;
			}
		}
		{
			auto it = context.defaultClassMap.find(baseClassLexerStringId);
			if (it == context.defaultClassMap.end()) {
				throwError("Cannot find class name1 '" +
				           context.lexerString[baseClassLexerStringId] + "'");
			}
			classId = it->second;
			auto classInfo = context.classInfo[*classId];
			if (classInfo->genericData) {
				throwError(
				    "'" + context.lexerString[baseClassLexerStringId] +
				    "' expects " +
				    std::to_string(
				        classInfo->genericData->genericDeclarations.size()) +
				    " type argument but 0 were given");
			}
			if (inputClassId.size() != classInfo->genericTypeId.size()) {
				throwError("'" + context.lexerString[baseClassLexerStringId] +
				           "' expects " +
				           std::to_string(classInfo->genericTypeId.size()) +
				           " type argument but " +
				           std::to_string(inputClassId.size()) + " were given");
			}
			return;
		}
	}
	{
		if (isGenericDeclaration) {
			throwError("Type parameter '" +
			           context.lexerString[baseClassLexerStringId] +
			           "' cannot have type arguments");
		}
	}

	if constexpr (canBeFunction) {
		auto it =
		    compile.funcMap.find(context.lexerString[baseClassLexerStringId]);
		// std::cerr << context.lexerString[baseClassLexerStringId] << "\n";
		if (it != compile.funcMap.end()) {
			// Generics no overload
			auto funcId = it->second[0];
			auto func = compile.functions[funcId];
			auto funcInfo = context.functionInfo[funcId];
			if (!funcInfo->genericData) {
				throwError("'" + context.lexerString[baseClassLexerStringId] +
				           "' isn't generic function");
			}
			if (inputClassId.size() !=
			    funcInfo->genericData->genericDeclarations.size()) {
				// int* x = nullptr; *x = 5;
				throwError("'" + context.lexerString[baseClassLexerStringId] +
				           "' expects " +
				           std::to_string(funcInfo->genericTypeId.size()) +
				           " type argument but " +
				           std::to_string(inputClassId.size()) + " were given");
			}
			for (size_t i = 0; i < inputClassId.size(); ++i) {
				auto *classDeclaration = inputClassId[i];
				if (!classDeclaration->classId) {
					classDeclaration->load<changeGenericsClassId>(in_data);
					if (!classDeclaration->classId) {
						throwError("Unresolved class " +
						           classDeclaration->getName(in_data));
					}
				}
			}
			std::string name = getName(in_data);
			if (!isGenerics(in_data)) {
				loadFunctionGenerics(in_data, name, this);
			}
			return;
		}
	}

	{
		auto it = context.defaultClassMap.find(baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
			throwError("Cannot find class name '" +
			           context.lexerString[baseClassLexerStringId] + "'");
		}
	}

	std::string name;
	if constexpr (!changeGenericsClassId) {
		bool change = true;
		std::unique_ptr<bool[]> marked(new bool[inputClassId.size()]());
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			auto *classDeclaration = inputClassId[i];
			if (!classDeclaration->classId) {
				classDeclaration->load<changeGenericsClassId>(in_data);
				marked[i] = true;
				change = false;
			}
		}
		name = getName(in_data);
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			if (marked[i]) {
				inputClassId[i]->classId = std::nullopt;
			}
		}
		if (!change)
			return;
	} else {
		bool change = true;
		for (size_t i = 0; i < inputClassId.size(); ++i) {
			auto *classDeclaration = inputClassId[i];
			if (!classDeclaration->classId) {
				classDeclaration->load<changeGenericsClassId>(in_data);
				if (!classDeclaration->classId) {
					change = false;
				}
			}
		}
		if (!change)
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
		throwError("'" + context.lexerString[baseClassLexerStringId] +
		           "' expects " +
		           std::to_string(classInfo->genericTypeId.size()) +
		           " type argument but " + std::to_string(inputClassId.size()) +
		           " were given");
	}
}

template <bool addNullable> std::string ClassDeclaration::getName(in_func) {
	if (classId) {
		if (classId == DefaultClass::functionClassId) {
			std::string result = "(";
			bool isFirst = true;
			for (int i = 1; i < inputClassId.size(); ++i) {
				if (isFirst) {
					isFirst = false;
				} else {
					result += ", ";
				}
				result += inputClassId[i]->getName(in_data);
			}
			result += ")->";
			result += inputClassId[0]->getName(in_data);
			if (nullable)
				result += "?";
			return result;
		}
		if constexpr (!addNullable) {
			return compile.classes[*classId]->name;
		}
		if (nullable) {
			return compile.classes[*classId]->name + "?";
		}
		return compile.classes[*classId]->name;
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

} // namespace AutoLang

#endif