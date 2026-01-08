#ifndef COMPILED_PROGRAM_HPP
#define COMPILED_PROGRAM_HPP

#include <vector>
#include <tuple>
#include <cstdint>
#include "ankerl/unordered_dense.h"
#include "AObject.hpp"
#include "StackAllocator.hpp"

#define NativeFuncInput ObjectManager &, StackAllocator &, size_t
#define NativeFuncInData ObjectManager &manager, StackAllocator &stackAllocator, size_t size

struct AClass;
struct Function;

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
	uint32_t mainFunctionId;
	std::vector<std::string> warnings;
	ankerl::unordered_dense::map<std::tuple<uint32_t, uint32_t, uint8_t>, uint32_t, PairHash> typeResult;
	std::vector<Function> functions;
	ankerl::unordered_dense::map<std::string, std::vector<uint32_t>> funcMap;
	std::vector<AClass> classes;
	ankerl::unordered_dense::map<std::string, uint32_t> classMap;
	std::vector<AObject *> constPool;
	ankerl::unordered_dense::map<int64_t, uint32_t> constIntMap;
	ankerl::unordered_dense::map<double, uint32_t> constFloatMap;
	ankerl::unordered_dense::map<AString *, uint32_t, AString::Hash, AString::Equal> constStringMap;
	uint32_t registerFunction( // Return value
		AClass *clazz,
		bool isStatic,
		std::string name,
		std::vector<uint32_t> args,
		std::vector<bool> nullableArgs,
		uint32_t returnId,
		bool returnNullable,
		AObject *(*native)(NativeFuncInput));
	inline uint32_t registerFunction( // No return value
		AClass *clazz,
		bool isStatic,
		std::string name,
		std::vector<uint32_t> args,
		std::vector<bool> nullableArgs,
		AObject *(*native)(NativeFuncInput))
	{
		return registerFunction(clazz, isStatic, name, args, nullableArgs, AutoLang::DefaultClass::nullClassId, false, native);
	}
	uint32_t registerClass(
		std::string name);

	uint32_t registerConstPool(ankerl::unordered_dense::map<AString *, uint32_t, AString::Hash, AString::Equal> &map, AString *value);
	template <typename T>
	uint32_t registerConstPool(ankerl::unordered_dense::map<T, uint32_t> &map, T value);

	inline static std::tuple<uint32_t, uint32_t, uint8_t> makeTuple(uint32_t first, uint32_t second, uint8_t op)
	{
		return std::make_tuple(
			std::min(first, second),
			std::max(first, second),
			op);
	}
	void addTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t classId);
	bool getTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t &result);
	~CompiledProgram();
};

#endif