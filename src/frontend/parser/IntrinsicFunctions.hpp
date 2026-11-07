#ifndef INTRINSIC_FUNCTIONS_HPP
#define INTRINSIC_FUNCTIONS_HPP

#include "frontend/parser/ParserContext.hpp"
#include <array>
#include <cstdint>
#include <string_view>

namespace Autolang {

struct IntrinsicFunctionEntry {
	LexerStringId lexerId;
	std::string_view name;
	std::string_view returnClass;
	bool nullable;
	bool elementFromArguments;
	int minArgs;
	int maxArgs;
};

inline constexpr std::array<IntrinsicFunctionEntry, 39> INTRINSIC_FUNCTION_TABLE = {{
	{lexerIdrun, "run", "", false, false, 1, 1},
	{lexerIdwith, "with", "", false, false, 2, 2},
	{lexerIdInt, "Int", "Int", false, false, 1, 1},
	{lexerIdChar, "Char", "Char", false, false, 1, 1},
	{lexerIdFloat, "Float", "Float", false, false, 1, 1},
	{lexerIdBool, "Bool", "Bool", false, false, 1, 1},
	{lexerIdgetClassId, "getClassId", "Int", false, false, 1, 1},
	{lexerIdArray, "Array", "Array", false, true, 0, -1},
	{lexerIdarrayOf, "arrayOf", "Array", false, true, 0, -1},
	{lexerIdlistOf, "listOf", "Array", false, true, 0, -1},
	{lexerIdmutableListOf, "mutableListOf", "Array", false, true, 0, -1},
	{lexerIdarrayListOf, "arrayListOf", "Array", false, true, 0, -1},
	{lexerIdintArrayOf, "intArrayOf", "Array", false, true, 0, -1},
	{lexerIdfloatArrayOf, "floatArrayOf", "Array", false, true, 0, -1},
	{lexerIddoubleArrayOf, "doubleArrayOf", "Array", false, true, 0, -1},
	{lexerIdbooleanArrayOf, "booleanArrayOf", "Array", false, true, 0, -1},
	{lexerIdstringArrayOf, "stringArrayOf", "Array", false, true, 0, -1},
	{lexerIdlongArrayOf, "longArrayOf", "Array", false, true, 0, -1},
	{lexerIdbyteArrayOf, "byteArrayOf", "Array", false, true, 0, -1},
	{lexerIdlistOfNotNull, "listOfNotNull", "Array", false, true, 0, -1},
	{lexerIdarrayOfNotNull, "arrayOfNotNull", "Array", false, true, 0, -1},
	{lexerIdmutableListOfNotNull, "mutableListOfNotNull", "Array", false, true, 0, -1},
	{lexerIdemptyArray, "emptyArray", "Array", false, false, 0, 0},
	{lexerIdemptyList, "emptyList", "Array", false, false, 0, 0},
	{lexerIdSet, "Set", "Set", false, true, 0, -1},
	{lexerIdsetOf, "setOf", "Set", false, true, 0, -1},
	{lexerIdmutableSetOf, "mutableSetOf", "Set", false, true, 0, -1},
	{lexerIdhashSetOf, "hashSetOf", "Set", false, true, 0, -1},
	{lexerIdlinkedSetOf, "linkedSetOf", "Set", false, true, 0, -1},
	{lexerIdsetOfNotNull, "setOfNotNull", "Set", false, true, 0, -1},
	{lexerIdmutableSetOfNotNull, "mutableSetOfNotNull", "Set", false, true, 0, -1},
	{lexerIdemptySet, "emptySet", "Set", false, false, 0, 0},
	{lexerIdMap, "Map", "Map", false, true, 0, -1},
	{lexerIdmapOf, "mapOf", "Map", false, true, 0, -1},
	{lexerIdmutableMapOf, "mutableMapOf", "Map", false, true, 0, -1},
	{lexerIdhashMapOf, "hashMapOf", "Map", false, true, 0, -1},
	{lexerIdlinkedMapOf, "linkedMapOf", "Map", false, true, 0, -1},
	{lexerIdemptyMap, "emptyMap", "Map", false, false, 0, 0},
	{lexerIdpairOf, "pairOf", "Pair", false, true, 2, 2},
}};

inline const IntrinsicFunctionEntry *findIntrinsicByLexerId(LexerStringId id) {
	for (const auto &entry : INTRINSIC_FUNCTION_TABLE) {
		if (entry.lexerId == id) {
			return &entry;
		}
	}
	return nullptr;
}

inline const IntrinsicFunctionEntry *findIntrinsicByName(std::string_view name) {
	for (const auto &entry : INTRINSIC_FUNCTION_TABLE) {
		if (entry.name == name) {
			return &entry;
		}
	}
	return nullptr;
}

} // namespace Autolang

#endif
