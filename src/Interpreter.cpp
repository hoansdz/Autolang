#ifndef INTERPRETER_CPP
#define INTERPRETER_CPP

#include <chrono>
#include <sstream>
#include <iostream>
#include "Debugger.hpp"
#include "Interpreter.hpp"
#include "DefaultOperator.hpp"

template <typename T>
AVM::AVM(T& lineData, bool allowDebug) : allowDebug(allowDebug)
{
	auto startCompiler = std::chrono::high_resolution_clock::now();
	AutoLang::DefaultClass::init(data);
	AutoLang::DefaultFunction::init(data);
	data.main = &data.functions[
		data.registerFunction(nullptr, false, ".main", {}, AutoLang::DefaultClass::nullClassId, nullptr)
	];
	
	{
		using namespace AutoLang::DefaultClass;
		using TT = AutoLang::Lexer::TokenType; // Giả sử bạn đặt TokenType trong Lexer namespace
		data.typeResult = {
			// int, float, bool
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::PLUS), INTCLASSID},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::MINUS), INTCLASSID},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::STAR), INTCLASSID},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::SLASH), INTCLASSID},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::PERCENT), INTCLASSID},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::LT), boolClassId},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::GT), boolClassId},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::LTE), boolClassId},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::GTE), boolClassId},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::EQEQ), boolClassId},
			{CompiledProgram::makeTuple(INTCLASSID, INTCLASSID, (uint8_t)TT::NOTEQ), boolClassId},
		
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::PLUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::MINUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::STAR), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::SLASH), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::PERCENT), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::LT), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::GT), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::LTE), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::GTE), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::EQEQ), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, INTCLASSID, (uint8_t)TT::NOTEQ), boolClassId},
		
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::PLUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::MINUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::STAR), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::SLASH), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::PERCENT), FLOATCLASSID},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::LT), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::GT), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::LTE), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::GTE), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::EQEQ), boolClassId},
			{CompiledProgram::makeTuple(FLOATCLASSID, FLOATCLASSID, (uint8_t)TT::NOTEQ), boolClassId},
		
			{CompiledProgram::makeTuple(boolClassId, INTCLASSID, (uint8_t)TT::PLUS), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, INTCLASSID, (uint8_t)TT::MINUS), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, INTCLASSID, (uint8_t)TT::STAR), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, INTCLASSID, (uint8_t)TT::SLASH), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, INTCLASSID, (uint8_t)TT::PERCENT), INTCLASSID},
		
			{CompiledProgram::makeTuple(boolClassId, FLOATCLASSID, (uint8_t)TT::PLUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(boolClassId, FLOATCLASSID, (uint8_t)TT::MINUS), FLOATCLASSID},
			{CompiledProgram::makeTuple(boolClassId, FLOATCLASSID, (uint8_t)TT::STAR), FLOATCLASSID},
			{CompiledProgram::makeTuple(boolClassId, FLOATCLASSID, (uint8_t)TT::SLASH), FLOATCLASSID},
			{CompiledProgram::makeTuple(boolClassId, FLOATCLASSID, (uint8_t)TT::PERCENT), FLOATCLASSID},
		
			// string + int/float
			{CompiledProgram::makeTuple(stringClassId, INTCLASSID, (uint8_t)TT::PLUS), stringClassId},
			{CompiledProgram::makeTuple(stringClassId, FLOATCLASSID, (uint8_t)TT::PLUS), stringClassId},
			{CompiledProgram::makeTuple(stringClassId, stringClassId, (uint8_t)TT::PLUS), stringClassId},
			
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::PLUS), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::MINUS), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::STAR), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::SLASH), INTCLASSID},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::AND_AND), boolClassId},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::OR_OR), boolClassId},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::EQEQ), boolClassId},
			{CompiledProgram::makeTuple(boolClassId, boolClassId, (uint8_t)TT::NOTEQ), boolClassId},
			
		};
	}
	AutoLang::build(data, lineData);
	initGlobalVariables();
	log();
	//log(&data.functions[data.funcMap["m()"][0]]);
	run();
	auto end = std::chrono::high_resolution_clock::now();
	auto durationCompiler = std::chrono::duration_cast<std::chrono::milliseconds>(end - startCompiler);
	std::cout<<"Total time : "<<durationCompiler.count()<<" ms"<<std::endl;
	while (allowDebug) {
		std::string command;
		std::getline(std::cin, command);
		std::istringstream iss(command);
		std::string word;
		if (iss >> word) {
			if (word == "log") {
				if (iss >> word) {
					std::string name = std::move(word);
					auto& vec = data.funcMap[name];
					if (vec.size() == 0) {
						std::cout<<"Cannot find "<<name;
						continue;
					}
					if (vec.size() == 1) {
						log(&data.functions[vec[0]]);
						std::cout<<std::endl;
						continue;
					}
					continue;
				} else {
					std::cout<<"Please log function"<<std::endl;
					continue;
				}
			} else
			if (word == "e") {
				return; 
			}
		} else {
			std::cout<<"wtf"<<std::endl;
		}
	}
}

void AVM::run() {
	std::cerr<<"-------------------"<<std::endl;
	std::cerr<<"Runtime"<<std::endl;
	auto start = std::chrono::high_resolution_clock::now();
	//[0, 5] is temp area
	stackAllocator.top = 5;
	run(data.main, stackAllocator.top, 0);
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout<<std::endl<<"Total runtime : "<<duration.count()<<" ms"<<std::endl;
}

AObject* AVM::run(Function* currentFunction, const size_t currentTop, size_t maxThisAreaSize) {
	auto* bytecodes = currentFunction->bytecodes.data();
	size_t i = 0;
	const size_t size = currentFunction->bytecodes.size();
	//std::cerr<<'\n';
	while (i < size) {
		//std::cerr<<i<<std::endl;
		switch (bytecodes[i++]) {
			case AutoLang::Opcode::CALL_FUNCTION:{
				//std::cerr<<"loading: function"<<std::endl;;
				Function* func = &data.functions[get_u32(bytecodes, i)];
				stackAllocator.top += currentFunction->maxDeclaration;
				uint32_t top = stackAllocator.top;
				//std::cerr<<"loading: ensure\n";
				stackAllocator.ensure(func->maxDeclaration);
				//std::cerr<<"loading: argument "<<stackAllocator.top<<" "<<func->args.size();
				for (size_t size = func->args.size(); size-- > 0;) {
					auto object = stack.pop();
					if (object != nullptr) object->retain();
					stackAllocator[size] = object;
				}
				//std::cerr<<"loading: value\n";
				//Prior
				
				AObject* value;
				if (func->bytecodes.size() != 0) {
					value = run(func, top, currentFunction->maxDeclaration);
				}
				
				if (func->native) {
					value = func->native(data.manager, stackAllocator, func->args.size());
				}
				
				if (value != nullptr) {
					//value->retain();
					stack.push(value);
				}
				/*std::cerr<<"Start "<<func->maxDeclaration<<std::endl;
				for (int i = 0; i < func->maxDeclaration; ++i) {
					AObject* obj = stackAllocator[i];
					if (obj == nullptr) continue;
					std::cerr<<"Before: "<<obj->refCount<<std::endl;
				}*/
				stackAllocator.clear(data.manager, top, top + func->maxDeclaration - 1);
				stackAllocator.freeTo(currentTop);
				break;
			}
			case AutoLang::Opcode::LOAD_CONST:{
				stack.push(getConstObject(get_u32(bytecodes, i)));
				break;
			}
			case AutoLang::Opcode::LOAD_CONST_PRIMARY:{
				stack.push(data.constPool[get_u32(bytecodes, i)]);
				break;
			}
			case AutoLang::Opcode::POP:{
				data.manager.release(stack.pop());
				break;
			}
			case AutoLang::Opcode::RETURN_LOCAL: {
				AObject** last = &stackAllocator[get_u32(bytecodes, i)];
				AObject* obj = *last;
				if (obj != nullptr) {
					--obj->refCount;
				}
				stack.push(obj);
				*last = nullptr;
				return obj;
			}
			case AutoLang::Opcode::CREATE_OBJECT:
				stack.push(new AObject(get_u32(bytecodes, i), get_u32(bytecodes, i)));
				break;
			case AutoLang::Opcode::LOAD_GLOBAL:{
				stack.push(globalVariables[get_u32(bytecodes, i)]);
				break;
			}
			case AutoLang::Opcode::STORE_GLOBAL:{
				setGlobalVariables(get_u32(bytecodes, i), stack.pop());
				break;
			}
			case AutoLang::Opcode::LOAD_LOCAL:{
				stack.push(stackAllocator[get_u32(bytecodes, i)]);
				break;
			}
			case AutoLang::Opcode::STORE_LOCAL:{
				stackAllocator.set(data.manager, get_u32(bytecodes, i), stack.pop());
				break;
			}
			case AutoLang::Opcode::LOAD_MEMBER:{
				stack.push(stack.pop()->member[get_u32(bytecodes, i)]);
				break;
			}
			case AutoLang::Opcode::STORE_MEMBER:{
				AObject** last = &stack.pop()->member[get_u32(bytecodes, i)];
				if (*last != nullptr) {
					data.manager.release(*last);
				}
				*last = stack.pop();
				(*last)->retain();
				break;
			}
			case AutoLang::Opcode::RETURN:{
				return nullptr;
			}
			case AutoLang::Opcode::RETURN_VALUE:{
				return stack.pop();
			}
			case AutoLang::Opcode::JUMP_IF_FALSE: {
				uint32_t pos = get_u32(bytecodes, i);
				if (!stack.pop()->b)
					i = pos;
				break;
			}
			case AutoLang::Opcode::JUMP: {
				i = get_u32(bytecodes, i);
				break;
			}
			case AutoLang::Opcode::TO_INT: {
				operate<AutoLang::DefaultFunction::to_int, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::TO_FLOAT: {
				operate<AutoLang::DefaultFunction::to_float, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::TO_STRING: {
				operate<AutoLang::DefaultFunction::to_string, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::PLUS_PLUS: {
				operate<AutoLang::DefaultFunction::plus_plus, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::MINUS_MINUS: {
				operate<AutoLang::DefaultFunction::minus_minus, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::PLUS: {
				operate<AutoLang::DefaultFunction::plus, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::MINUS: {
				operate<AutoLang::DefaultFunction::minus, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::MUL: {
				operate<AutoLang::DefaultFunction::mul, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::DIVIDE: {
				operate<AutoLang::DefaultFunction::divide, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::PLUS_EQUAL:
				operateWithoutPush<AutoLang::DefaultFunction::plus_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::MINUS_EQUAL:
				operateWithoutPush<AutoLang::DefaultFunction::minus_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::MUL_EQUAL:
				operateWithoutPush<AutoLang::DefaultFunction::mul_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::DIVIDE_EQUAL:
				operateWithoutPush<AutoLang::DefaultFunction::divide_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::MOD: {
				operate<AutoLang::DefaultFunction::mod, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::NEGATIVE: {
				operate<AutoLang::DefaultFunction::negative, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::NOT: {
				operate<AutoLang::DefaultFunction::op_not, 1>(currentTop);
				break;
			}
			case AutoLang::Opcode::AND_AND: {
				operate<AutoLang::DefaultFunction::op_and_and, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::OR_OR: {
				operate<AutoLang::DefaultFunction::op_or_or, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::EQUAL_VALUE:
				operate<AutoLang::DefaultFunction::op_eqeq, 2>(currentTop);
				break;
			case AutoLang::Opcode::NOTEQ_VALUE:
				operate<AutoLang::DefaultFunction::op_not_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::EQUAL_POINTER: {
				operate<AutoLang::DefaultFunction::op_eq_pointer, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::NOTEQ_POINTER: {
				operate<AutoLang::DefaultFunction::op_not_eq_pointer, 2>(currentTop);
				break;
			}
			case AutoLang::Opcode::LESS_THAN_EQ:
				operate<AutoLang::DefaultFunction::op_less_than_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::LESS_THAN:
				operate<AutoLang::DefaultFunction::op_less_than, 2>(currentTop);
				break;
			case AutoLang::Opcode::GREATER_THAN_EQ:
				operate<AutoLang::DefaultFunction::op_greater_than_eq, 2>(currentTop);
				break;
			case AutoLang::Opcode::GREATER_THAN:
				operate<AutoLang::DefaultFunction::op_greater_than, 2>(currentTop);
				break;
			default:
				break;
		}
	}
	return nullptr;
}

AObject* AVM::getConstObject(uint32_t id) {
	AObject* obj = data.constPool[id];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return data.manager.create(obj->i);
		case AutoLang::DefaultClass::FLOATCLASSID:
			return data.manager.create(obj->f);
		default:
			if (obj->type != AutoLang::DefaultClass::stringClassId)
				return obj;
			return data.manager.create(static_cast<AString*>(obj->ref));
	}
}

void AVM::initGlobalVariables() {
	globalVariables = new AObject*[data.main->maxDeclaration];
	for (uint32_t i=0; i<data.main->maxDeclaration; ++i) {
		globalVariables[i] = nullptr;
	}
}

void AVM::setGlobalVariables(uint32_t i, AObject* object) {
	AObject** last = &globalVariables[i];
	if (*last != nullptr) {
		data.manager.release(*last);
	}
	*last = object;
	object->retain();
}

template <AObject* (*native)(NativeFuncInput), size_t size>
void AVM::operate(size_t currentTop) {
	stackAllocator.top = 0;
	inputArgument<size>();
	stack.push(native(data.manager, stackAllocator, size));
	stackAllocator.top = currentTop;
	stackAllocator.clearTemp<size>(data.manager);
}

template <AObject* (*native)(NativeFuncInput), size_t size>
void AVM::operateWithoutPush(size_t currentTop) {
	stackAllocator.top = 0;
	inputArgument<size>();
	native(data.manager, stackAllocator, size);
	stackAllocator.top = currentTop;
	stackAllocator.clearTemp<size>(data.manager);
}

uint32_t AVM::get_u32(uint8_t* code, size_t& ip) {
	uint32_t val;
	std::memcpy(&val, code + ip, 4);
	ip += 4;
	return val;
}

void AVM::log() {
	log(data.main);
	std::cerr<<"-------------------"<<std::endl;
	std::cerr<<"ConstPool: "<<data.constPool.size()<<" elements"<<std::endl;
	stackAllocator.top = 0;
	for (int i=0; i<data.constPool.size(); ++i) {
		stackAllocator[0] = data.constPool[i];
		std::cerr<<'['<<i<<"] ";
		AutoLang::DefaultFunction::println(data.manager, stackAllocator, 1);
	}
	std::cerr<<"-------------------"<<std::endl;
	std::cerr<<"Function: "<<data.functions.size()<<" elements"<<std::endl;
	for (auto pair:data.funcMap) {
		for (auto& funcId:pair.second) {
			bool isFirst = true;
			Function* func = &data.functions[funcId];
			std::cerr<<"["<<funcId<<"] [Declaration: "<<func->maxDeclaration<<"] "<<pair.first<<": (";
			for (uint32_t classId:func->args) {
				if (isFirst) {
					isFirst = false;
				} else {
					std::cerr<<", ";
				}
				std::cerr<<data.classes[classId].name;
			}
			std::cerr<<")->";
			std::cerr<<data.classes[func->returnId].name<<std::endl;
			//if (!func->native) log(func);
		}
	}
	/*uint32_t totalSize = bytecodes.size() +
		sizeof(AVM) +
		data.functions.size() * sizeof(Function) +
		data.classes.size() * sizeof(ClassInfo) +
		data.constPool.size() * sizeof(AObject*)  +
		data.constPool.size() * sizeof(AObject)  +
		estimateUnorderedMapSize(data.funcMap) +
		estimateUnorderedMapSize(data.classMap) +
		data.main->maxDeclaration * sizeof(AObject*) 
		;
	std::cerr<<"TotalSize: "<<static_cast<double>(totalSize) / 1024<<" kb\n";*/
}

void AVM::log(Function* currentFunction) {
	auto* bytecodes = currentFunction->bytecodes.data();
	auto size = currentFunction->bytecodes.size();
	std::cerr<<"Size: "<<size<<" bytes"<<std::endl;
	size_t i = 0;
	while (i<size) {
		std::cerr<<"["<<i<<"] ";
		uint8_t b = bytecodes[i++];
		switch (b) {
			case AutoLang::Opcode::CALL_FUNCTION:
				std::cerr<<"CALL_FUNCTION	 "<<data.functions[get_u32(bytecodes, i)].name<<std::endl;
				break;
			case AutoLang::Opcode::LOAD_CONST:
				std::cerr<<"LOAD_CONST	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::LOAD_CONST_PRIMARY:
				std::cerr<<"CONST_PRIMARY	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::POP:
				std::cerr<<"POP	 "<<std::endl;
				break;
			case AutoLang::Opcode::RETURN_LOCAL:
				std::cerr<<"RETURN_LOCAL	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::CREATE_OBJECT:
				std::cerr<<"CREATE_OBJECT	 "<<data.classes[get_u32(bytecodes, i)].name<<"     "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::LOAD_GLOBAL:
				std::cerr<<"LOAD_GLOBAL	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::STORE_GLOBAL:
				std::cerr<<"STORE_GLOBAL	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::LOAD_LOCAL:
				std::cerr<<"LOAD_LOCAL	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::STORE_LOCAL:
				std::cerr<<"STORE_LOCAL	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::LOAD_MEMBER:
				std::cerr<<"LOAD_MEMBER	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::STORE_MEMBER:
				std::cerr<<"STORE_MEMBER	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::RETURN:
				std::cerr<<"RETURN	 "<<std::endl;
				break;
			case AutoLang::Opcode::RETURN_VALUE:
				std::cerr<<"RETURN_VALUE	 "<<std::endl;
				break;
			case AutoLang::Opcode::JUMP_IF_FALSE:
				std::cerr<<"JUMP_IF_FALSE	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::JUMP:
				std::cerr<<"JUMP	 "<<get_u32(bytecodes, i)<<std::endl;
				break;
			case AutoLang::Opcode::TO_INT:
				std::cerr<<"TO_INT	 "<<std::endl;
				break;
			case AutoLang::Opcode::TO_FLOAT:
				std::cerr<<"TO_FLOAT	 "<<std::endl;
				break;
			case AutoLang::Opcode::TO_STRING:
				std::cerr<<"TO_STRING	 "<<std::endl;
				break;
			case AutoLang::Opcode::AND_AND:
				std::cerr<<"AND	 "<<std::endl;
				break;
			case AutoLang::Opcode::OR_OR:
				std::cerr<<"OR	 "<<std::endl;
				break;
			case AutoLang::Opcode::PLUS_PLUS:
				std::cerr<<"PLUS_PLUS	 "<<std::endl;
				break;
			case AutoLang::Opcode::MINUS_MINUS:
				std::cerr<<"MINUS_MINUS	 "<<std::endl;
				break;
			case AutoLang::Opcode::PLUS:
				std::cerr<<"PLUS	 "<<std::endl;
				break;
			case AutoLang::Opcode::MINUS:
				std::cerr<<"MINUS	 "<<std::endl;
				break;
			case AutoLang::Opcode::MUL:
				std::cerr<<"MUL	 "<<std::endl;
				break;
			case AutoLang::Opcode::DIVIDE:
				std::cerr<<"DIVIDE	 "<<std::endl;
				break;
			case AutoLang::Opcode::PLUS_EQUAL:
				std::cerr<<"PLUS_EQUAL	 "<<std::endl;
				break;
			case AutoLang::Opcode::MINUS_EQUAL:
				std::cerr<<"MINUS_EQUAL	 "<<std::endl;
				break;
			case AutoLang::Opcode::MUL_EQUAL:
				std::cerr<<"MUL_EQUAL	 "<<std::endl;
				break;
			case AutoLang::Opcode::DIVIDE_EQUAL:
				std::cerr<<"DIVIDE_EQUAL	 "<<std::endl;
				break;
			case AutoLang::Opcode::NEGATIVE:
				std::cerr<<"NEGATIVE	 "<<std::endl;
				break;
			case AutoLang::Opcode::MOD:
				std::cerr<<"MOD	 "<<std::endl;
				break;
			case AutoLang::Opcode::NOT:
				std::cerr<<"NOT	 "<<std::endl;
				break;
			case AutoLang::Opcode::EQUAL_VALUE:
				std::cerr<<"EQUAL_VALUE	 "<<std::endl;
				break;
			case AutoLang::Opcode::NOTEQ_VALUE:
				std::cerr<<"NOTEQ_VALUE	 "<<std::endl;
				break;
			case AutoLang::Opcode::EQUAL_POINTER:
				std::cerr<<"EQUAL_POINTER	 "<<std::endl;
				break;
			case AutoLang::Opcode::NOTEQ_POINTER:
				std::cerr<<"NOTEQ_POINTER	 "<<std::endl;
				break;
			case AutoLang::Opcode::LESS_THAN_EQ:
				std::cerr<<"LESS_THAN_EQ	 "<<std::endl;
				break;
			case AutoLang::Opcode::LESS_THAN:
				std::cerr<<"LESS_THAN	 "<<std::endl;
				break;
			case AutoLang::Opcode::GREATER_THAN_EQ:
				std::cerr<<"GREATER_THAN_EQ	 "<<std::endl;
				break;
			case AutoLang::Opcode::GREATER_THAN:
				std::cerr<<"GREATER_THAN	 "<<std::endl;
				break;
		}
	}
}

AVM::~AVM() {
	delete[] globalVariables;
}

template <typename K, typename V>
size_t estimateUnorderedMapSize(const std::unordered_map<K, V>& map) {
	size_t total = sizeof(map);
	total += map.bucket_count() * sizeof(void*);  // bucket array

	for (const auto& [key, value] : map) {
		total += sizeof(std::pair<const K, V>);
		total += sizeof(key);
		total += sizeof(value);
	}

	return total;
}

#endif