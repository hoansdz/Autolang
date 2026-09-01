#ifndef CLASS_DECLARATION_HPP
#define CLASS_DECLARATION_HPP

#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/Type.hpp"
#include <optional>
#include <vector>
#include "shared/DefaultClass.hpp"

namespace Autolang {

struct ParserContext;
struct CompiledProgram;
struct LibraryData;
struct TypealiasData;

struct ClassDeclaration {
	LibraryData *mode;
	uint32_t line;
	LexerStringId baseClassLexerStringId;
	bool nullable = false;
	// class A<T> => T is generic declaration
	bool isGenericDeclaration = false;
	bool mustInference = true;
	// class A<T> => A<T> has generic declaration
	bool isGeneric = false;
	std::vector<ClassDeclaration *> inputClassId;
	std::optional<uint32_t> classId;
	inline bool isGenerics(in_func) { return isGeneric; }
	template <bool changeGenericsClassId, bool canBeFunction = false>
	void load(in_func);
	template <bool changeGenericsClassId, bool canBeFunction = false>
	void onLoadTypealias(in_func, TypealiasData *typealias);
	int64_t loadHash() {
		uint64_t hash = 14695981039346656037ull;
		auto mix = [](uint64_t h, uint64_t val) {
			for (int i = 0; i < 8; ++i) {
				h ^= (val & 0xFF);
				h *= 1099511628211ull;
				val >>= 8;
			}
			return h;
		};
		hash = mix(hash, *classId);
		for (auto classDeclaration : inputClassId) {
			if (classDeclaration->classId == DefaultClass::functionClassId) {
				hash = mix(hash, classDeclaration->loadHash());
			} else {
				hash = mix(hash, *classDeclaration->classId);
			}
		}
		return static_cast<int64_t>(hash);
	}
	template <bool addNullable = false> std::string getName(in_func);
	ClassDeclaration *copy(in_func);
	bool isSame(ClassDeclaration *classDeclaration);
	bool isMatch(ClassDeclaration *classDeclaration);
	ClassDeclaration();
	[[noreturn]] void throwError(std::string message);
};

} // namespace Autolang

#endif