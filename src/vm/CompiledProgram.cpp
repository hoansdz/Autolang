#ifndef COMPILED_PROGRAM_CPP
#define COMPILED_PROGRAM_CPP

#include <iostream>
#include <cstdint>
#include <cassert>
#include "CompiledProgram.hpp"
#include "Interpreter.hpp"
#include "Lexer.hpp"

template <bool isConstructor>
uint32_t CompiledProgram::registerFunction(
	AClass *clazz,
	bool isStatic,
	std::string name,
	std::vector<uint32_t> args,
	std::vector<bool> nullableArgs,
	uint32_t returnId,
	bool returnNullable,
	AObject *(*native)(NativeFuncInput))
{
	if (args.size() != nullableArgs.size()) {
		printDebug(name);
		assert (args.size() != nullableArgs.size() && "Args and nullable args is not same , check ");
	}
	uint32_t id = functions.size();
	if (clazz != nullptr)
	{
		name = clazz->name + '.' + name;
		clazz->funcMap[name].push_back(id);
		if constexpr (!isConstructor) {
			if (native && !isStatic) //Auto insert "this" if native function in class
			{
				args.insert(args.begin(), clazz->id);
				nullableArgs.insert(nullableArgs.begin(), false);
			}
		} else { //Auto insert "this" if function is constructor (User can create non native function)
			args.insert(args.begin(), clazz->id);
			nullableArgs.insert(nullableArgs.begin(), false);
		}
	}
	// printDebug(name);
	// printDebug("BEGIN SIZE FUNC: "+std::to_string(functions.size()) + " " + std::to_string(funcMap.size()));
	functions.emplace_back(
		id,
		name,
		native,
		clazz ? isStatic : true,
		args,
		nullableArgs,
		returnId,
		returnNullable
	);
	funcMap[name].push_back(id);
	// printDebug("END SIZE FUNC: "+std::to_string(functions.size()) + " " + std::to_string(funcMap.size()));
	return id;
}

uint32_t CompiledProgram::registerClass(
	std::string name)
{
	auto it = classMap.find(name);
	if (it != classMap.end())
		throw std::runtime_error("Class " + name + " has exist");
	uint32_t id = classes.size();
	classes.emplace_back(
		name,
		id);
	classMap[name] = id;
	return id;
}

template <typename T>
std::string toStr(T *value)
{
	return std::to_string((uintptr_t)value);
}

template <typename T>
std::string toStr(T value)
{
	return std::to_string(value);
}

uint32_t CompiledProgram::registerConstPool(ankerl::unordered_dense::map<AString *, uint32_t, AString::Hash, AString::Equal> &map, AString *value)
{
	auto it = map.find(value);
	if (it != map.end())
	{
		delete value;
		return it->second;
	}
	map[value] = constPool.size();
	// printDebug("Value : "+toStr(value)+" at "+std::to_string(constPool.size()));
	AObject *obj = manager.create(value);
	constPool.push_back(obj);
	obj->refCount = AutoLang::DefaultClass::refCountForGlobal;
	return constPool.size() - 1;
}

template <typename T>
uint32_t CompiledProgram::registerConstPool(ankerl::unordered_dense::map<T, uint32_t> &map, T value)
{
	auto it = map.find(value);
	if (it != map.end())
	{
		return it->second;
	}
	map[value] = constPool.size();
	// printDebug("Value : "+toStr(value)+" at "+std::to_string(constPool.size()));
	AObject *obj = manager.create(value);
	constPool.push_back(obj);
	obj->refCount = AutoLang::DefaultClass::refCountForGlobal;
	return constPool.size() - 1;
}

CompiledProgram::~CompiledProgram()
{
	// for (auto [str, _] : constStringMap) {
	// 	delete str;
	// }
	for (AObject* obj : constPool) {
		if (obj->refCount <= AutoLang::DefaultClass::refCountForGlobal) {
			obj->refCount = 0;
			obj->free();
			reinterpret_cast<AreaChunkSlot*>(obj)->isFree =true;
			continue;
		}
		obj->refCount = obj->refCount - AutoLang::DefaultClass::refCountForGlobal;
	}
	manager.destroy();
}

#endif