#ifndef COMPILED_PROGRAM_HPP
#define COMPILED_PROGRAM_HPP

#include "third_party/ankerl/unordered_dense.h"
#include "shared/AClass.hpp"
#include "shared/AObject.hpp"
#include "shared/AString.hpp"
#include "shared/ChunkArena.hpp"
#include "shared/FixedPool.hpp"
#include "shared/Function.hpp"
#include "shared/ObjectManager.hpp"
#include "shared/StackAllocator.hpp"
#include "shared/StringArena.hpp"
#include <cstdint>
#include <tuple>
#include <vector>

namespace Autolang {

struct PairHash {
	template <typename T1, typename T2, typename T3>
	size_t operator()(const std::tuple<T1, T2, T3> &tuple) const {
		return std::hash<T1>{}(std::get<0>(tuple)) ^
		       (std::hash<T2>{}(std::get<1>(tuple)) << 1) ^
		       (std::hash<T3>{}(std::get<2>(tuple)) << 2);
	}
};

struct CompiledProgram {
	// Use when finished resize vector
	Function *main;
	CompiledProgram();
	ObjectManager manager;
	FunctionId mainFunctionId;
	std::vector<uint8_t> allBytecodes;
	std::vector<uint32_t> allGenericType;
	std::vector<uint32_t> allMemberId;
	std::vector<uint32_t> allCatchPosition;
	std::vector<bool> allMemberNullable;
	std::vector<bool> allGenericTypeNullable;
	StringArena stringArena;
	ChunkArena<Function, 64> functionAllocator;
	std::vector<Function *> functions;
	HashMap<std::string, std::vector<FunctionId>> funcMap;
	ChunkArena<AClass, 64> classAllocator;
	std::vector<AClass *> classes;
	HashMap<std::string, ClassId> classMap;
	ChunkArena<AObject, 32> constObjectAllocator;
	std::vector<AObject *> constPool;
	void refresh();
	void destroy();
	template <bool isConstructor = false>
	FunctionId registerFunction( // Return value
	    AClass *clazz, std::string name, ClassId *args, uint32_t argSize,
	    ClassId returnId, uint32_t functionFlags);
	inline FunctionId registerFunction( // No return value
	    AClass *clazz, std::string name, ClassId *args, uint32_t argSize,
	    uint32_t functionFlags) {
		return registerFunction(clazz, std::move(name), args, argSize,
		                        Autolang::DefaultClass::voidClassId,
		                        functionFlags);
	}
	ClassId registerClass(std::string name, uint32_t classFlags);

	Offset registerConstPool(
	    HashMap<AString *, uint32_t, AString::Hash, AString::Equal> &map,
	    AString *value);
	Offset registerEnumConstPool(ClassId classId); //Enum
	template <typename T>
	Offset registerConstPool(HashMap<T, uint32_t> &map, T value);
	~CompiledProgram();
};

} // namespace Autolang

#endif