#ifndef CLASS_DECLARATION_HPP
#define CLASS_DECLARATION_HPP

#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/Type.hpp"
#include <optional>
#include <vector>


namespace AutoLang {

struct ParserContext;
struct CompiledProgram;
struct LibraryData;

struct ClassDeclaration {
	LibraryData *mode;
	uint32_t line;
	LexerStringId baseClassLexerStringId;
	bool nullable = false;
	//class A<T> => T is generic declaration
	bool isGenericDeclaration = false;
	bool mustInference = true;
	//class A<T> => A<T> has generic declaration
	bool isGeneric = false;
	std::vector<ClassDeclaration *> inputClassId;
	std::optional<uint32_t> classId;
	inline bool isGenerics(in_func) { return isGeneric; }
	template <bool changeGenericsClassId, bool canBeFunction = false>
	void load(in_func);
	template <bool addNullable = false> std::string getName(in_func);
	ClassDeclaration *copy(in_func);
	bool isSame(ClassDeclaration* classDeclaration);
	ClassDeclaration();
	[[noreturn]] inline void throwError(std::string message);
};

} // namespace AutoLang

#endif