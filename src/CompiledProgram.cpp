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

uint32_t CompiledProgram::registerConstPool(
	AObject* obj
) {
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID: {
			auto it = constIntMap.find(obj->i);
			if (it == constIntMap.end()) {
				constIntMap[obj->i] = constPool.size();
				break;
			}
			return it->second;
		}
		case AutoLang::DefaultClass::FLOATCLASSID: {
			auto it = constFloatMap.find(obj->f);
			if (it == constFloatMap.end()) {
				constFloatMap[obj->f] = constPool.size();
				break;
			}
			return it->second;
		}
		default: {
			if (obj->type == AutoLang::DefaultClass::stringClassId) {
				auto it = constStringMap.find(static_cast<AString*>(obj->ref));
				if (it == constStringMap.end()) {
					constStringMap[static_cast<AString*>(obj->ref)] = constPool.size();
					break;
				}
				return it->second;
			}
		}
	}
	constPool.push_back(obj);
	obj->refCount = 9999;
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