#ifndef COMPILED_PROGRAM_CPP
#define COMPILED_PROGRAM_CPP

#include "CompiledProgram.hpp"
#include "Interpreter.hpp"

uint32_t CompiledProgram::registerFunction(
	AClass* clazz,
	bool isStatic,
	std::string name,
	std::vector<uint32_t> args,
	uint32_t returnId,
	AObject* (*native)(NativeFuncInput)
) {
	uint32_t id = functions.size();
	if (clazz != nullptr) {
		name = clazz->name+'.'+name;
		clazz->funcMap[name].push_back(id);
		if (native && !isStatic) {
			args.insert(args.begin(), clazz->id);
		}
	}
	functions.emplace_back(
		name,
		native,
		clazz ? isStatic : true,
		args,
		returnId
	);
	funcMap[name].push_back(id);
	return id;
}

uint32_t CompiledProgram::registerClass(
	std::string name
) {
	auto it = classMap.find(name);
	if (it != classMap.end()) 
		throw std::runtime_error("Class " + name + " has exist");
	uint32_t id = classes.size();
	classes.emplace_back(
		name,
		id
	);
	classMap[name] =  id;
	return id;
}

template<typename T>
std::string toStr(T* value) {
	return std::to_string((uintptr_t)value);
}

template<typename T>
std::string toStr(T value) {
	return std::to_string(value);
}

uint32_t CompiledProgram::registerConstPool(std::unordered_map<AString*, uint32_t, AString::Hash, AString::Equal>& map, AString* value) {
	auto it = map.find(value);
	if (it != map.end()) {
		return it->second;
	}
	map[value] = constPool.size();
	printDebug("Value : "+toStr(value)+" at "+std::to_string(constPool.size()));
	AObject* obj = manager.create(value);
	constPool.push_back(obj);
	obj->refCount = 2'000'000;
	return constPool.size() - 1;
}

template<typename T>
uint32_t CompiledProgram::registerConstPool(std::unordered_map<T, uint32_t>& map, T value) {
	auto it = map.find(value);
	if (it != map.end()) {
		return it->second;
	}
	map[value] = constPool.size();
	printDebug("Value : "+toStr(value)+" at "+std::to_string(constPool.size()));
	AObject* obj = manager.create(value);
	constPool.push_back(obj);
	obj->refCount = 2'000'000;
	return constPool.size() - 1;
}

void CompiledProgram::addTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t classId) {
	typeResult[{
		std::min(first, second), 
		std::max(first, second),
		op
	}] = classId;
}

bool CompiledProgram::getTypeResult(uint32_t first, uint32_t second, uint8_t op, uint32_t& result) {
	auto it = typeResult.find({
		std::min(first, second), 
		std::max(first, second),
		op
	});
	if (it == typeResult.end()) return false;
	result = it->second;
	return true;
}

CompiledProgram::~CompiledProgram() {
	// for (auto* obj : constPool) {
	// 	obj->free();
	// 	delete obj;
	// }
}

#endif