#ifndef CLASS_DECLARATION_HPP
#define CLASS_DECLARATION_HPP

#include "shared/Type.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include <vector>
#include <optional>

namespace AutoLang {

struct ParserContext;
struct CompiledProgram;

struct ClassDeclaration {
	uint32_t line;
	LexerStringId baseClassLexerStringId;
	bool nullable = false;
	bool isGenericDeclaration = false;
	bool mustInference = true;
	std::vector<ClassDeclaration *> inputClassId;
	std::optional<uint32_t> classId;
	bool isGenerics(in_func);
	template <bool changeGenericsClassId> 
	void load(in_func);
	template <bool addNullable = false>
	std::string getName(in_func);
	ClassDeclaration() {}
};

} // namespace AutoLang

#endif