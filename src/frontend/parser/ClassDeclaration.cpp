#ifndef CLASS_DECLARATION_CPP
#define CLASS_DECLARATION_CPP

#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

template <bool changeGenericsClassId> void ClassDeclaration::load(in_func) {
	if (classId) {
		return;
	}
	if (inputClassId.empty()) {
		if (isGenericDeclaration)
			return;
		auto it = context.defaultClassMap.find(baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
			throw ParserError(
			    line, "Cannot find class name '" +
			              context.lexerString[baseClassLexerStringId] + "'");
		}
		classId = it->second;
		return;
	}
	{
		if (isGenericDeclaration) {
			throw ParserError(line,
			                  "Type parameter '" +
			                      context.lexerString[baseClassLexerStringId] +
			                      "' cannot have type arguments");
		}
		auto it = context.defaultClassMap.find(baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
			throw ParserError(
			    line, "Cannot find class name '" +
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
		if (!change) return;
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
		if (!change) return;
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
	classId = loadGenerics(in_data, name, this);
}

bool ClassDeclaration::isGenerics(in_func) {
	if (isGenericDeclaration)
		return true;
	for (auto classDeclaration : inputClassId) {
		if (classDeclaration->isGenericDeclaration)
			return true;
	}
	return false;
}

template <bool addNullable> std::string ClassDeclaration::getName(in_func) {
	if (classId) {
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