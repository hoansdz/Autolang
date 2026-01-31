#ifndef COMPILED_PROGRAM_HPP
#define COMPILED_PROGRAM_HPP

#include <vector>
#include <tuple>
#include <cstdint>
#include "ankerl/unordered_dense.h"
#include "shared/AObject.hpp"
#include "shared/AString.hpp"
#include "shared/AClass.hpp"
#include "shared/Function.hpp"
#include "shared/StackAllocator.hpp"
#include "shared/ObjectManager.hpp"

struct PairHash
{
	template <typename T1, typename T2, typename T3>
	size_t operator()(const std::tuple<T1, T2, T3> &tuple) const
	{
		return std::hash<T1>{}(std::get<0>(tuple)) ^
			   (std::hash<T2>{}(std::get<1>(tuple)) << 1) ^
			   (std::hash<T3>{}(std::get<2>(tuple)) << 2);
	}
};

struct CompiledProgram
{
	// Use when finished resize vector
	Function *main;
	CompiledProgram() {}
	ObjectManager manager;
	Offset mainFunctionId;
	AreaAllocator<Function, 64> functionAllocator;
	std::vector<Function*> functions;
	HashMap<std::string, std::vector<Offset>> funcMap;
	AreaAllocator<AClass, 64> classAllocator;
	std::vector<AClass*> classes;
	HashMap<std::string, Offset> classMap;
	std::vector<AObject *> constPool;
	HashMap<int64_t, Offset> constIntMap;
	HashMap<double, Offset> constFloatMap;
	HashMap<AString *, Offset, AString::Hash, AString::Equal> constStringMap;
	template <bool isConstructor = false>
	Offset registerFunction( // Return value
		AClass *clazz,
		bool isStatic,
		std::string name,
		ClassId* args,
		std::vector<bool> nullableArgs,
		ClassId returnId,
		bool returnNullable,
		AObject *(*native)(NativeFuncInput));
	inline Offset registerFunction( // No return value
		AClass *clazz,
		bool isStatic,
		std::string name,
		ClassId* args,
		std::vector<bool> nullableArgs,
		AObject *(*native)(NativeFuncInput))
	{
		return registerFunction(clazz, isStatic, name, args, nullableArgs, AutoLang::DefaultClass::nullClassId, false, native);
	}
	ClassId registerClass(
		std::string name);

	Offset registerConstPool(HashMap<AString *, uint32_t, AString::Hash, AString::Equal> &map, AString *value);
	template <typename T>
	Offset registerConstPool(HashMap<T, uint32_t> &map, T value);
	~CompiledProgram();
};

#endif