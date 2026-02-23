#ifndef COMPILED_PROGRAM_CPP
#define COMPILED_PROGRAM_CPP

#include "shared/CompiledProgram.hpp"
#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

namespace AutoLang {

void CompiledProgram::destroy() {
	manager.refresh();
	functionAllocator.destroy();
	functions.clear();
	funcMap.clear();
	classAllocator.destroy();
	classes.clear();
	classMap.clear();
}

void CompiledProgram::refresh() {
	manager.refresh();
}

template <bool isConstructor>
Offset CompiledProgram::registerFunction(AClass *clazz, std::string name,
                                         ClassId *args, uint32_t argSize,
                                         ClassId returnId,
                                         uint32_t functionFlags) {
	uint32_t id = functions.size();
	if (clazz != nullptr) {
		clazz->funcMap[name].push_back(id);
		name = clazz->name + '.' + name;
		// if constexpr (!isConstructor) {
		// 	if (native && !isStatic) //Auto insert "this" if native function in
		// class
		// 	{
		// 		args.insert(args.begin(), clazz->id);
		// 		nullableArgs.insert(nullableArgs.begin(), false);
		// 	}
		// } else { //Auto insert "this" if function is constructor (User can
		// create non native function) 	args.insert(args.begin(), clazz->id);
		// 	nullableArgs.insert(nullableArgs.begin(), false);
		// }
	}
	// printDebug(name);
	// printDebug("BEGIN SIZE FUNC: "+std::to_string(functions.size()) + " " +
	// std::to_string(funcMap.size()));
	auto *func = functionAllocator.push();
	func->id = id;
	func->name = std::move(name);
	func->args = args;
	func->argSize = argSize;
	func->returnId = returnId;
	func->functionFlags = functionFlags;
	functions.push_back(func);
	funcMap[func->name].push_back(id);
	// printDebug("END SIZE FUNC: "+std::to_string(functions.size()) + " " +
	// std::to_string(funcMap.size()));
	return id;
}

ClassId CompiledProgram::registerClass(std::string name, uint32_t classFlags) {
	auto it = classMap.find(name);
	if (it != classMap.end())
		throw std::runtime_error("Class " + name + " has exist");
	Offset id = classes.size();
	AClass *clazz = classAllocator.push();
	clazz->name = std::move(name);
	clazz->id = id;
	clazz->classFlags = classFlags;
	classes.push_back(clazz);
	classMap[clazz->name] = id;
	return id;
}

template <typename T> std::string toStr(T *value) {
	return std::to_string((uintptr_t)value);
}

template <typename T> std::string toStr(T value) {
	return std::to_string(value);
}

Offset CompiledProgram::registerConstPool(
    HashMap<AString *, uint32_t, AString::Hash, AString::Equal> &map,
    AString *value) {
	auto it = map.find(value);
	if (it != map.end()) {
		delete value;
		return it->second;
	}
	uint32_t id = constPool.size();
	map[value] = id;
	// printDebug("Value : "+toStr(value)+" at
	// "+std::to_string(constPool.size()));
	constPool.push_back(value);
	AObject* obj = &constPool.back();
	obj->refCount = AutoLang::DefaultClass::refCountForGlobal;
	obj->flags = AObject::Flags::OBJ_IS_CONST;
	return id;
}

template <typename T>
Offset CompiledProgram::registerConstPool(HashMap<T, uint32_t> &map, T value) {
	auto it = map.find(value);
	if (it != map.end()) {
		return it->second;
	}
	uint32_t id = constPool.size();
	map[value] = id;
	// printDebug("Value : "+toStr(value)+" at
	// "+std::to_string(constPool.size()));
	constPool.push_back(value);
	AObject* obj = &constPool.back();
	obj->refCount = AutoLang::DefaultClass::refCountForGlobal;
	obj->flags = AObject::Flags::OBJ_IS_CONST;
	return id;
}

CompiledProgram::~CompiledProgram() {
	manager.destroy();
	classAllocator.destroy();
	functionAllocator.destroy();
}

} // namespace AutoLang

#endif