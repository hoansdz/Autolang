#ifndef COMPILED_PROGRAM_CPP
#define COMPILED_PROGRAM_CPP

#include "shared/CompiledProgram.hpp"
#include "backend/vm/AVM.hpp"
#include "frontend/lexer/Lexer.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

namespace Autolang {

CompiledProgram::CompiledProgram() {}

void CompiledProgram::destroy() {
	manager.refresh();
	functionAllocator.destroy();
	functions.clear();
	funcMap.clear();
	classAllocator.destroy();
	classes.clear();
	classMap.clear();
	constPool.clear();
	constObjectAllocator.destroy();
	constPool.push_back(DefaultClass::nullObject);
	constPool.push_back(DefaultClass::trueObject);
	constPool.push_back(DefaultClass::falseObject);
	allBytecodes.clear();
	allGenericType.clear();
	allMemberId.clear();
	allMemberNullable.clear();
	allGenericTypeNullable.clear();
	allOpcodeLines.clear();
	allMainFunctionOpcodeLines.clear();
	stringArena.reset();
}

template <bool isConstructor>
FunctionId CompiledProgram::registerFunction(const char *path, AClass *clazz,
                                             std::string name, ClassId *args,
                                             uint32_t argSize, ClassId returnId,
                                             uint32_t functionFlags) {
	uint32_t id = functions.size();
	if (clazz != nullptr) {
		clazz->funcMap[name].push_back(id);
		name = clazz->getName(*this) + '.' + name;
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
	auto &funcs = funcMap[name];
	funcs.push_back(id);
	auto *func = functionAllocator.push();
	func->id = id;
	if (funcs.size() == 1) {
		func->nameStringOffset = stringArena.push_back(name);
	} else {
		func->nameStringOffset = functions[funcs[0]]->nameStringOffset;
	}
	func->args = args;
	func->argSize = argSize;
	func->path = path;
	func->returnId = returnId;
	func->functionFlags = functionFlags;
	functions.push_back(func);

	// std::cerr << "Created " << func->getName(compile) << " " << id << "\n";
	// printDebug("END SIZE FUNC: "+std::to_string(functions.size()) + " " +
	// std::to_string(funcMap.size()));
	return id;
}

ClassId CompiledProgram::registerClass(std::string name, uint32_t classFlags) {
	auto it = classMap.find(name);
	if (it != classMap.end())
		throw std::runtime_error("Class " + name + " already exists");
	ClassId id = classes.size();
	AClass *clazz = classAllocator.push();
	// std::cerr << "Created class name: " << name << " " << id << "\n";
	clazz->nameStringOffset = stringArena.push_back(name);
	clazz->id = id;
	clazz->classFlags = classFlags;
	clazz->genericBaseClassId = 0;
	clazz->parentId = 0;
	classes.push_back(clazz);
	classMap[name] = id;
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
	constPool.push_back(constObjectAllocator.push(value));
	AObject *obj = constPool.back();
	obj->refCount = Autolang::DefaultClass::refCountForGlobal;
	obj->flags = AObject::Flags::OBJ_IS_CONST;
	return id;
}

Offset CompiledProgram::registerEnumConstPool(ClassId classId) {
	uint32_t id = constPool.size();
	constPool.push_back(constObjectAllocator.push(classId));
	AObject *obj = constPool.back();
	obj->refCount = Autolang::DefaultClass::refCountForGlobal;
	obj->flags = AObject::Flags::OBJ_IS_CONST & AObject::Flags::OBJ_IS_NO_DATA;
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
	constPool.push_back(constObjectAllocator.push(value));
	AObject *obj = constPool.back();
	obj->refCount = Autolang::DefaultClass::refCountForGlobal;
	obj->flags = AObject::Flags::OBJ_IS_CONST;
	return id;
}

CompiledProgram::~CompiledProgram() {
	manager.destroy();
	classAllocator.destroy();
	functionAllocator.destroy();
	for (auto obj : constPool) {
		if (obj->type == DefaultClass::stringClassId) {
			delete obj->str;
		}
	}
	constObjectAllocator.destroy();
}

} // namespace Autolang

#endif