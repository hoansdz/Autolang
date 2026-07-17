#ifndef AVM_RUN_SWITCH_CPP
#define AVM_RUN_SWITCH_CPP

#define ip (currentCallFrame->i)

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <functional>
#include <iostream>

namespace AutoLang {

#define DATA_CAL_DATA(opcode, data1, data2)                                    \
	do_##opcode : {                                                            \
		uint8_t tablePos = bytecodes[ip++];                                    \
		tempAllocateArea[0] = data1[get_u32(bytecodes, ip)];                   \
		tempAllocateArea[1] = data2[get_u32(bytecodes, ip)];                   \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		DISPATCH();                                                            \
	}

#define DATA_MEMBER_CAL_DATA(opcode, data1, data2)                             \
	do_##opcode : {                                                            \
		uint8_t tablePos = bytecodes[ip++];                                    \
		uint32_t pos = get_u32(bytecodes, ip);                                 \
		tempAllocateArea[0] =                                                  \
		    data1[pos]->member->data[get_u32(bytecodes, ip)];                  \
		tempAllocateArea[1] = data2[get_u32(bytecodes, ip)];                   \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		DISPATCH();                                                            \
	}

#define DATA_CAL_DATA_MEMBER(opcode, data1, data2)                             \
	do_##opcode : {                                                            \
		uint8_t tablePos = bytecodes[ip++];                                    \
		tempAllocateArea[0] = data1[get_u32(bytecodes, ip)];                   \
		uint32_t pos = get_u32(bytecodes, ip);                                 \
		tempAllocateArea[1] =                                                  \
		    data2[pos]->member->data[get_u32(bytecodes, ip)];                  \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		DISPATCH();                                                            \
	}

#define DATA_MEMBER_CAL_DATA_MEMBER(opcode, data1, data2)                      \
	do_##opcode : {                                                            \
		uint8_t tablePos = bytecodes[ip++];                                    \
		uint32_t pos1 = get_u32(bytecodes, ip);                                \
		tempAllocateArea[0] =                                                  \
		    data1[pos1]->member->data[get_u32(bytecodes, ip)];                 \
		uint32_t pos2 = get_u32(bytecodes, ip);                                \
		tempAllocateArea[1] =                                                  \
		    data2[pos2]->member->data[get_u32(bytecodes, ip)];                 \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		DISPATCH();                                                            \
	}

#define DATA_STORE_DATA(opcode, data1, data2)                                  \
	do_##opcode : {                                                            \
		AObject *&obj1 = data1[get_u32(bytecodes, ip)];                        \
		AObject *obj2 = data2[get_u32(bytecodes, ip)];                         \
		obj2->retain();                                                        \
		if (obj1 != nullptr) {                                                 \
			data.manager.release(obj1);                                        \
		}                                                                      \
		obj1 = obj2;                                                           \
		DISPATCH();                                                            \
	}

#define DATA_STORE_DATA_CLONE(opcode, data1, data2)                            \
	do_##opcode : {                                                            \
		AObject *&obj1 = data1[get_u32(bytecodes, ip)];                        \
		AObject *obj2 = data2[get_u32(bytecodes, ip)];                         \
		if (obj1 != nullptr) {                                                 \
			data.manager.release(obj1);                                        \
		}                                                                      \
		switch (obj2->type) {                                                  \
			case DefaultClass::intClassId: {                                   \
				auto newValue = notifier->createInt(obj2->i);                  \
				newValue->retain();                                            \
				obj1 = newValue;                                               \
				break;                                                         \
			}                                                                  \
			case DefaultClass::floatClassId: {                                 \
				auto newValue = notifier->createFloat(obj2->f);                \
				newValue->retain();                                            \
				obj1 = newValue;                                               \
				break;                                                         \
			}                                                                  \
			default: {                                                         \
				obj2->retain();                                                \
				obj1 = obj2;                                                   \
				break;                                                         \
			}                                                                  \
		}                                                                      \
		DISPATCH();                                                            \
	}

#define NEGATIVE_DATA(opcode, data)                                            \
	do_##opcode : {                                                            \
		auto obj = data[get_u32(bytecodes, ip)];                               \
		switch (obj->type) {                                                   \
			case DefaultClass::intClassId: {                                   \
				auto newValue = notifier->createInt(-obj->i);                  \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
			case DefaultClass::floatClassId: {                                 \
				auto newValue = notifier->createFloat(-obj->f);                \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
			case DefaultClass::boolClassId: {                                  \
				auto newValue = notifier->createInt(-obj->b);                  \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
		}                                                                      \
		DISPATCH();                                                            \
	}

#define NEGATIVE_DATA_MEMBER(opcode, data1)                                    \
	do_##opcode : {                                                            \
		auto parent = data1[get_u32(bytecodes, ip)];                           \
		auto obj = parent->member->data[get_u32(bytecodes, ip)];               \
		switch (obj->type) {                                                   \
			case DefaultClass::intClassId: {                                   \
				auto newValue = notifier->createInt(-obj->i);                  \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
			case DefaultClass::floatClassId: {                                 \
				auto newValue = notifier->createFloat(-obj->f);                \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
			case DefaultClass::boolClassId: {                                  \
				auto newValue = notifier->createBool(-obj->b);                 \
				newValue->retain();                                            \
				stack.push(newValue);                                          \
				break;                                                         \
			}                                                                  \
		}                                                                      \
		DISPATCH();                                                            \
	}
void AVM::resume() {
	static ANativeFunction operatorTable[] = {

	    /*  0 */ DefaultFunction::plus_plus,   // ++
	    /*  1 */ DefaultFunction::minus_minus, // --

	    /*  2 */ DefaultFunction::plus,      // +
	    /*  3 */ DefaultFunction::plus_eq,   // +=
	    /*  4 */ DefaultFunction::minus,     // -
	    /*  5 */ DefaultFunction::minus_eq,  // -=
	    /*  6 */ DefaultFunction::mul,       // *
	    /*  7 */ DefaultFunction::mul_eq,    // *=
	    /*  8 */ DefaultFunction::divide,    // /
	    /*  9 */ DefaultFunction::divide_eq, // /=

	    /* 10 */ DefaultFunction::mod, // %

	    /* 11 */ DefaultFunction::bitwise_and, // &
	    /* 12 */ DefaultFunction::bitwise_or,  // |

	    /* 13 */ DefaultFunction::negative, // unary -
	    /* 14 */ DefaultFunction::op_not,   // !

	    /* 15 */ DefaultFunction::op_and_and, // &&
	    /* 16 */ DefaultFunction::op_or_or,   // ||

	    /* 17 */ DefaultFunction::op_less_than,       // <
	    /* 18 */ DefaultFunction::op_greater_than,    // >
	    /* 19 */ DefaultFunction::op_less_than_eq,    // <=
	    /* 20 */ DefaultFunction::op_greater_than_eq, // >=

	    /* 21 */ DefaultFunction::op_eqeq,   // ==
	    /* 22 */ DefaultFunction::op_not_eq, // !=

	    /* 23 */ DefaultFunction::op_eq_pointer,    // === (pointer equality)
	    /* 24 */ DefaultFunction::op_not_eq_pointer // !== (pointer inequality)
	};
	uint32_t topCallFrame = callFrames.getSize();
	auto currentCallFrame = callFrames.top();

	Function *currentFunction = nullptr;
	uint8_t *bytecodes = nullptr;
	size_t size = 0;
resumeCallFrame:;
	if (currentCallFrame->exception) {
		if (currentCallFrame->catchPosition.empty()) {
			// std::cerr << currentCallFrame->fromStackAllocator << " & "
			//           << stackAllocator.getTop() << "\n";
			stackAllocator.clear(
			    data.manager, currentCallFrame->fromStackAllocator,
			    currentCallFrame->fromStackAllocator +
			        currentCallFrame->func->maxDeclaration - 1);
			while (stack.getSize() > currentCallFrame->startStackCount) {
				auto obj = stack.pop();
				data.manager.release(obj);
			}
			if (callFrames.getSize() == topCallFrame) {
				callFrames.pop();
				state = VMState::ERR;
				if (callFrames.getSize() != 0) {
					auto oldCallFrame = callFrames.top();
					oldCallFrame->exception = currentCallFrame->exception;
					currentCallFrame = oldCallFrame;
					stackAllocator.freeTo(oldCallFrame->fromStackAllocator);
					return;
				}
				stackAllocator.freeTo(0);
				return;
			}
			callFrames.pop();
			// std::cerr<<"from
			// "<<currentCallFrame->func->getName(compile)<<"\n";
			auto oldCallFrame = callFrames.top();
			oldCallFrame->exception = currentCallFrame->exception;
			currentCallFrame = oldCallFrame;
			stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
			// std::cerr<<"from " << currentCallFrame->fromStackAllocator <<
			// "\n";
			goto resumeCallFrame;
		} else {
			// std::cerr << "First size " <<
			// currentCallFrame->catchPosition.size() << "\n";
			currentCallFrame->i = currentCallFrame->catchPosition.back();
			currentCallFrame->catchPosition.pop_back();
			// std::cerr << "Second size " <<
			// currentCallFrame->catchPosition.size() << "\n"; std::cerr <<
			// "Goto " << currentCallFrame->i << "\n";
		}
	}
	currentFunction = currentCallFrame->func;
	bytecodes =
	    data.allBytecodes.data() + currentCallFrame->func->bytecodes.offset;

	size = currentCallFrame->func->bytecodes.size;
	notifier->callFrame = currentCallFrame;
	// std::cerr << "Called function " <<
	// currentCallFrame->func->getName(compile) << " "
	//           << currentCallFrame->fromStackAllocator << " with "
	//           << currentCallFrame->func->argSize << " arguments and "
	//           << currentCallFrame->func->maxDeclaration
	//           << " declaration and bytecode size: " << size
	//           << " and from stack allocator: "
	//           << currentCallFrame->fromStackAllocator << "\n";
	try {
		static void *dispatchTable[256];
		static bool dispatchInitialized = false;
		if (!dispatchInitialized) {
			dispatchInitialized = true;
			for (int __i = 0; __i < 256; ++__i)
				dispatchTable[__i] = &&do_ILLEGAL;
			dispatchTable[AutoLang::Opcode::CALL_FUNCTION_OBJECT] =
			    &&do_CALL_FUNCTION_OBJECT;
			dispatchTable[AutoLang::Opcode::CALL_FUNCTION] = &&do_CALL_FUNCTION;
			dispatchTable[AutoLang::Opcode::CALL_VOID_FUNCTION] =
			    &&do_CALL_VOID_FUNCTION;
			dispatchTable[AutoLang::Opcode::CALL_NATIVE_FUNCTION] =
			    &&do_CALL_NATIVE_FUNCTION;
			dispatchTable[AutoLang::Opcode::CALL_VOID_NATIVE_FUNCTION] =
			    &&do_CALL_VOID_NATIVE_FUNCTION;
			dispatchTable[AutoLang::Opcode::CALL_VTABLE_FUNCTION] =
			    &&do_CALL_VTABLE_FUNCTION;
			dispatchTable[AutoLang::Opcode::CALL_VTABLE_VOID_FUNCTION] =
			    &&do_CALL_VTABLE_VOID_FUNCTION;
			dispatchTable[AutoLang::Opcode::CREATE_FUNCTION_OBJECT] =
			    &&do_CREATE_FUNCTION_OBJECT;
			dispatchTable
			    [AutoLang::Opcode::CREATE_FUNCTION_OBJECT_FROM_VTABLE] =
			        &&do_CREATE_FUNCTION_OBJECT_FROM_VTABLE;
			dispatchTable[AutoLang::Opcode::CALL_DATA_CONTRUCTOR] =
			    &&do_CALL_DATA_CONTRUCTOR;
			dispatchTable[AutoLang::Opcode::FOR_LIST] = &&do_FOR_LIST;
			dispatchTable[AutoLang::Opcode::FOR_SET] = &&do_FOR_SET;
			dispatchTable[AutoLang::Opcode::IN_RANGE] = &&do_IN_RANGE;
			dispatchTable[AutoLang::Opcode::LOAD_CONST] = &&do_LOAD_CONST;
			dispatchTable[AutoLang::Opcode::LOAD_CONST_PRIMARY] =
			    &&do_LOAD_CONST_PRIMARY;
			dispatchTable[AutoLang::Opcode::POP] = &&do_POP;
			dispatchTable[AutoLang::Opcode::POP_NO_RELEASE] =
			    &&do_POP_NO_RELEASE;
			dispatchTable[AutoLang::Opcode::NOT] = &&do_NOT;
			dispatchTable[AutoLang::Opcode::NEGATIVE] = &&do_NEGATIVE;
			dispatchTable[AutoLang::Opcode::RETURN_LOCAL] = &&do_RETURN_LOCAL;
			dispatchTable[AutoLang::Opcode::CREATE_OBJECT] = &&do_CREATE_OBJECT;
			dispatchTable[AutoLang::Opcode::FAST_SAVE_MEMBER] =
			    &&do_FAST_SAVE_MEMBER;
			dispatchTable[AutoLang::Opcode::CREATE_SET_OBJECT] =
			    &&do_CREATE_SET_OBJECT;
			dispatchTable[AutoLang::Opcode::CREATE_MAP_OBJECT] =
			    &&do_CREATE_MAP_OBJECT;
			dispatchTable[AutoLang::Opcode::CREATE_NATIVE_OBJECT] =
			    &&do_CREATE_NATIVE_OBJECT;
			dispatchTable[AutoLang::Opcode::LOAD_GLOBAL] = &&do_LOAD_GLOBAL;
			dispatchTable[AutoLang::Opcode::STORE_GLOBAL] = &&do_STORE_GLOBAL;
			dispatchTable[AutoLang::Opcode::LOAD_LOCAL] = &&do_LOAD_LOCAL;
			dispatchTable[AutoLang::Opcode::STORE_LOCAL] = &&do_STORE_LOCAL;
			dispatchTable[AutoLang::Opcode::LOCAL_LOAD_MEMBER] =
			    &&do_LOCAL_LOAD_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_LOAD_MEMBER] =
			    &&do_GLOBAL_LOAD_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_LOAD_MEMBER_AND_STORE] =
			    &&do_GLOBAL_LOAD_MEMBER_AND_STORE;
			dispatchTable[AutoLang::Opcode::LOCAL_LOAD_MEMBER_AND_STORE] =
			    &&do_LOCAL_LOAD_MEMBER_AND_STORE;
			dispatchTable[AutoLang::Opcode::LOAD_MEMBER] = &&do_LOAD_MEMBER;
			dispatchTable[AutoLang::Opcode::LOAD_MEMBER_IF_NNULL_OR_JUMP] =
			    &&do_LOAD_MEMBER_IF_NNULL_OR_JUMP;
			dispatchTable[AutoLang::Opcode::LOAD_MEMBER_CAN_RET_NULL_OR_JUMP] =
			    &&do_LOAD_MEMBER_CAN_RET_NULL_OR_JUMP;
			dispatchTable[AutoLang::Opcode::STORE_MEMBER] = &&do_STORE_MEMBER;
			dispatchTable[AutoLang::Opcode::RETURN] = &&do_RETURN;
			dispatchTable[AutoLang::Opcode::RETURN_VALUE] = &&do_RETURN_VALUE;
			dispatchTable[AutoLang::Opcode::RETURN_CONST] = &&do_RETURN_CONST;
			dispatchTable[AutoLang::Opcode::RETURN_GLOBAL] = &&do_RETURN_GLOBAL;
			dispatchTable[AutoLang::Opcode::RETURN_LOCAL_MEMBER] =
			    &&do_RETURN_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::RETURN_GLOBAL_MEMBER] =
			    &&do_RETURN_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::JUMP_IF_FALSE] = &&do_JUMP_IF_FALSE;
			dispatchTable[AutoLang::Opcode::JUMP_IF_FALSE_NO_POP] =
			    &&do_JUMP_IF_FALSE_NO_POP;
			dispatchTable[AutoLang::Opcode::JUMP_IF_TRUE_NO_POP] =
			    &&do_JUMP_IF_TRUE_NO_POP;
			dispatchTable[AutoLang::Opcode::JUMP] = &&do_JUMP;
			dispatchTable[AutoLang::Opcode::JUMP_IF_NULL] = &&do_JUMP_IF_NULL;
			dispatchTable[AutoLang::Opcode::JUMP_AND_DELETE_IF_NULL] =
			    &&do_JUMP_AND_DELETE_IF_NULL;
			dispatchTable[AutoLang::Opcode::JUMP_AND_SET_IF_NULL] =
			    &&do_JUMP_AND_SET_IF_NULL;
			dispatchTable[AutoLang::Opcode::JUMP_IF_NON_NULL] =
			    &&do_JUMP_IF_NON_NULL;
			dispatchTable[AutoLang::Opcode::IS] = &&do_IS;
			dispatchTable[AutoLang::Opcode::SAFE_CAST] = &&do_SAFE_CAST;
			dispatchTable[AutoLang::Opcode::UNSAFE_CAST] = &&do_UNSAFE_CAST;
			dispatchTable[AutoLang::Opcode::WAIT_INPUT] = &&do_WAIT_INPUT;
			dispatchTable[AutoLang::Opcode::LOAD_EXCEPTION] =
			    &&do_LOAD_EXCEPTION;
			dispatchTable[AutoLang::Opcode::THROW_EXCEPTION] =
			    &&do_THROW_EXCEPTION;
			dispatchTable[AutoLang::Opcode::ADD_TRY_BLOCK] = &&do_ADD_TRY_BLOCK;
			dispatchTable[AutoLang::Opcode::REMOVE_TRY_AND_JUMP] =
			    &&do_REMOVE_TRY_AND_JUMP;
			dispatchTable[AutoLang::Opcode::REMOVE_TRY] = &&do_REMOVE_TRY;
			dispatchTable[AutoLang::Opcode::CLONE] = &&do_CLONE;
			dispatchTable[AutoLang::Opcode::TO_INT] = &&do_TO_INT;
			dispatchTable[AutoLang::Opcode::TO_FLOAT] = &&do_TO_FLOAT;
			dispatchTable[AutoLang::Opcode::TO_STRING] = &&do_TO_STRING;
			dispatchTable[AutoLang::Opcode::PLUS_PLUS] = &&do_PLUS_PLUS;
			dispatchTable[AutoLang::Opcode::PLUS_PLUS_GLOBAL] =
			    &&do_PLUS_PLUS_GLOBAL;
			dispatchTable[AutoLang::Opcode::PLUS_PLUS_LOCAL] =
			    &&do_PLUS_PLUS_LOCAL;
			dispatchTable[AutoLang::Opcode::MINUS_MINUS] = &&do_MINUS_MINUS;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_CONST_JUMP] =
			    &&do_GLOBAL_CAL_CONST_JUMP;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_LOCAL_JUMP] =
			    &&do_GLOBAL_CAL_LOCAL_JUMP;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_GLOBAL_JUMP] =
			    &&do_GLOBAL_CAL_GLOBAL_JUMP;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_CONST_JUMP] =
			    &&do_LOCAL_CAL_CONST_JUMP;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_LOCAL_JUMP] =
			    &&do_LOCAL_CAL_LOCAL_JUMP;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_GLOBAL_JUMP] =
			    &&do_LOCAL_CAL_GLOBAL_JUMP;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_GLOBAL] =
			    &&do_GLOBAL_CAL_GLOBAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_LOCAL] =
			    &&do_GLOBAL_CAL_LOCAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_CONST] =
			    &&do_GLOBAL_CAL_CONST;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_GLOBAL_MEMBER] =
			    &&do_GLOBAL_CAL_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_CAL_LOCAL_MEMBER] =
			    &&do_GLOBAL_CAL_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_GLOBAL] =
			    &&do_LOCAL_CAL_GLOBAL;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_LOCAL] =
			    &&do_LOCAL_CAL_LOCAL;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_CONST] =
			    &&do_LOCAL_CAL_CONST;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_GLOBAL_MEMBER] =
			    &&do_LOCAL_CAL_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::LOCAL_CAL_LOCAL_MEMBER] =
			    &&do_LOCAL_CAL_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::CONST_CAL_LOCAL] =
			    &&do_CONST_CAL_LOCAL;
			dispatchTable[AutoLang::Opcode::CONST_CAL_GLOBAL] =
			    &&do_CONST_CAL_GLOBAL;
			dispatchTable[AutoLang::Opcode::CONST_CAL_LOCAL_MEMBER] =
			    &&do_CONST_CAL_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::CONST_CAL_GLOBAL_MEMBER] =
			    &&do_CONST_CAL_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::LOCAL_MEMBER_CAL_LOCAL] =
			    &&do_LOCAL_MEMBER_CAL_LOCAL;
			dispatchTable[AutoLang::Opcode::LOCAL_MEMBER_CAL_GLOBAL] =
			    &&do_LOCAL_MEMBER_CAL_GLOBAL;
			dispatchTable[AutoLang::Opcode::LOCAL_MEMBER_CAL_CONST] =
			    &&do_LOCAL_MEMBER_CAL_CONST;
			dispatchTable[AutoLang::Opcode::LOCAL_MEMBER_CAL_LOCAL_MEMBER] =
			    &&do_LOCAL_MEMBER_CAL_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::LOCAL_MEMBER_CAL_GLOBAL_MEMBER] =
			    &&do_LOCAL_MEMBER_CAL_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_MEMBER_CAL_LOCAL] =
			    &&do_GLOBAL_MEMBER_CAL_LOCAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_MEMBER_CAL_GLOBAL] =
			    &&do_GLOBAL_MEMBER_CAL_GLOBAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_MEMBER_CAL_CONST] =
			    &&do_GLOBAL_MEMBER_CAL_CONST;
			dispatchTable[AutoLang::Opcode::GLOBAL_MEMBER_CAL_GLOBAL_MEMBER] =
			    &&do_GLOBAL_MEMBER_CAL_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_MEMBER_CAL_LOCAL_MEMBER] =
			    &&do_GLOBAL_MEMBER_CAL_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_LOAD_MEMBER] =
			    &&do_GLOBAL_LOAD_MEMBER;
			dispatchTable[AutoLang::Opcode::LOCAL_LOAD_MEMBER] =
			    &&do_LOCAL_LOAD_MEMBER;
			dispatchTable[AutoLang::Opcode::GLOBAL_LOAD_MEMBER_AND_STORE] =
			    &&do_GLOBAL_LOAD_MEMBER_AND_STORE;
			dispatchTable[AutoLang::Opcode::LOCAL_LOAD_MEMBER_AND_STORE] =
			    &&do_LOCAL_LOAD_MEMBER_AND_STORE;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_GLOBAL] =
			    &&do_GLOBAL_STORE_GLOBAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_LOCAL] =
			    &&do_GLOBAL_STORE_LOCAL;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_CONST] =
			    &&do_GLOBAL_STORE_CONST;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_GLOBAL_CLONE] =
			    &&do_GLOBAL_STORE_GLOBAL_CLONE;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_LOCAL_CLONE] =
			    &&do_GLOBAL_STORE_LOCAL_CLONE;
			dispatchTable[AutoLang::Opcode::GLOBAL_STORE_CONST_CLONE] =
			    &&do_GLOBAL_STORE_CONST_CLONE;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_GLOBAL] =
			    &&do_LOCAL_STORE_GLOBAL;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_LOCAL] =
			    &&do_LOCAL_STORE_LOCAL;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_CONST] =
			    &&do_LOCAL_STORE_CONST;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_GLOBAL_CLONE] =
			    &&do_LOCAL_STORE_GLOBAL_CLONE;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_LOCAL_CLONE] =
			    &&do_LOCAL_STORE_LOCAL_CLONE;
			dispatchTable[AutoLang::Opcode::LOCAL_STORE_CONST_CLONE] =
			    &&do_LOCAL_STORE_CONST_CLONE;
			dispatchTable[AutoLang::Opcode::NOT_LOCAL] = &&do_NOT_LOCAL;
			dispatchTable[AutoLang::Opcode::NOT_GLOBAL] = &&do_NOT_GLOBAL;
			dispatchTable[AutoLang::Opcode::NOT_LOCAL_MEMBER] =
			    &&do_NOT_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::NOT_GLOBAL_MEMBER] =
			    &&do_NOT_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::NEGATIVE_LOCAL] =
			    &&do_NEGATIVE_LOCAL;
			dispatchTable[AutoLang::Opcode::NEGATIVE_GLOBAL] =
			    &&do_NEGATIVE_GLOBAL;
			dispatchTable[AutoLang::Opcode::NEGATIVE_LOCAL_MEMBER] =
			    &&do_NEGATIVE_LOCAL_MEMBER;
			dispatchTable[AutoLang::Opcode::NEGATIVE_GLOBAL_MEMBER] =
			    &&do_NEGATIVE_GLOBAL_MEMBER;
			dispatchTable[AutoLang::Opcode::AND_AND] = &&do_AND_AND;
			dispatchTable[AutoLang::Opcode::OR_OR] = &&do_OR_OR;
			dispatchTable[AutoLang::Opcode::EQUAL_VALUE] = &&do_EQUAL_VALUE;
			dispatchTable[AutoLang::Opcode::NOTEQ_VALUE] = &&do_NOTEQ_VALUE;
			dispatchTable[AutoLang::Opcode::IS_NULL] = &&do_IS_NULL;
			dispatchTable[AutoLang::Opcode::IS_NON_NULL] = &&do_IS_NON_NULL;
			dispatchTable[AutoLang::Opcode::LOAD_NULL] = &&do_LOAD_NULL;
			dispatchTable[AutoLang::Opcode::LOAD_TRUE] = &&do_LOAD_TRUE;
			dispatchTable[AutoLang::Opcode::LOAD_FALSE] = &&do_LOAD_FALSE;
			dispatchTable[AutoLang::Opcode::EQUAL_POINTER] = &&do_EQUAL_POINTER;
			dispatchTable[AutoLang::Opcode::NOTEQ_POINTER] = &&do_NOTEQ_POINTER;
			dispatchTable[AutoLang::Opcode::LESS_THAN_EQ] = &&do_LESS_THAN_EQ;
			dispatchTable[AutoLang::Opcode::LESS_THAN] = &&do_LESS_THAN;
			dispatchTable[AutoLang::Opcode::GREATER_THAN_EQ] =
			    &&do_GREATER_THAN_EQ;
			dispatchTable[AutoLang::Opcode::GREATER_THAN] = &&do_GREATER_THAN;
			dispatchTable[AutoLang::Opcode::INT_FROM_INT] = &&do_INT_FROM_INT;
			dispatchTable[AutoLang::Opcode::FLOAT_TO_INT] = &&do_FLOAT_TO_INT;
			dispatchTable[AutoLang::Opcode::FLOAT_FROM_FLOAT] =
			    &&do_FLOAT_FROM_FLOAT;
			dispatchTable[AutoLang::Opcode::INT_TO_FLOAT] = &&do_INT_TO_FLOAT;
			dispatchTable[AutoLang::Opcode::BOOL_TO_INT] = &&do_BOOL_TO_INT;
			dispatchTable[AutoLang::Opcode::BOOL_TO_FLOAT] = &&do_BOOL_TO_FLOAT;
			dispatchTable[AutoLang::Opcode::I_CAL_I] = &&do_I_CAL_I;
			dispatchTable[AutoLang::Opcode::I_CAL_F] = &&do_I_CAL_F;
			dispatchTable[AutoLang::Opcode::F_CAL_F] = &&do_F_CAL_F;
			dispatchTable[AutoLang::Opcode::F_CAL_I] = &&do_F_CAL_I;
			dispatchTable[AutoLang::Opcode::I_MINUS_I] = &&do_I_MINUS_I;
			dispatchTable[AutoLang::Opcode::I_MINUS_F] = &&do_I_MINUS_F;
			dispatchTable[AutoLang::Opcode::F_MINUS_F] = &&do_F_MINUS_F;
			dispatchTable[AutoLang::Opcode::F_MINUS_I] = &&do_F_MINUS_I;
			dispatchTable[AutoLang::Opcode::PLUS] = &&do_PLUS;
			dispatchTable[AutoLang::Opcode::MINUS] = &&do_MINUS;
			dispatchTable[AutoLang::Opcode::MUL] = &&do_MUL;
			dispatchTable[AutoLang::Opcode::DIVIDE] = &&do_DIVIDE;
			dispatchTable[AutoLang::Opcode::PLUS_EQUAL] = &&do_PLUS_EQUAL;
			dispatchTable[AutoLang::Opcode::MINUS_EQUAL] = &&do_MINUS_EQUAL;
			dispatchTable[AutoLang::Opcode::MUL_EQUAL] = &&do_MUL_EQUAL;
			dispatchTable[AutoLang::Opcode::DIVIDE_EQUAL] = &&do_DIVIDE_EQUAL;
			dispatchTable[AutoLang::Opcode::MOD] = &&do_MOD;
			dispatchTable[AutoLang::Opcode::BITWISE_AND] = &&do_BITWISE_AND;
			dispatchTable[AutoLang::Opcode::BITWISE_OR] = &&do_BITWISE_OR;
		}

#ifdef AUTOLANG_LIMIT_OPCODE
#define DISPATCH()                                                             \
	if (ip >= size)                                                            \
		goto endFunction;                                                      \
	if (--currentLimitOpcodeCount == 0) {                                      \
		notifier->throwException("VMException: instruction limit exceeded (" + \
		                         std::to_string(limitOpcodeCount) + ")");      \
		goto resumeCallFrame;                                                  \
	}                                                                          \
	goto *dispatchTable[bytecodes[ip++]]
#else
#define DISPATCH()                                                             \
	if (ip >= size)                                                            \
		goto endFunction;                                                      \
	goto *dispatchTable[bytecodes[ip++]]
#endif

		DISPATCH();

	do_ILLEGAL: {
		std::cerr << "Illegal opcode: " << int(bytecodes[ip - 1])
		          << " at i=" << (ip - 1) << "\n";
		notifier->throwException("VMException: illegal opcode");
		goto resumeCallFrame;
	}

	do_CALL_FUNCTION_OBJECT: {
		auto obj = stack.pop();
		if (!callFunctionObject(obj)) {
			data.manager.release(obj);
			// std::cerr << "A\n";
			goto resumeCallFrame;
		}
		data.manager.release(obj);
		DISPATCH();
	}

	do_CALL_FUNCTION: {
		if (!callFunction<false, true, false>(currentCallFrame, currentFunction,
		                                      bytecodes, ip)) {
			goto resumeCallFrame;
		}
		DISPATCH();
	}

	do_CALL_VOID_FUNCTION: {
		if (!callFunction<false, false, false>(
		        currentCallFrame, currentFunction, bytecodes, ip)) {
			goto resumeCallFrame;
		}
		// if (state == VMState::WAITING) {
		// 	return;
		// }
		DISPATCH();
	}

	do_CALL_NATIVE_FUNCTION: {
		if (!callNativeFunction<true>(currentCallFrame, currentFunction,
		                              bytecodes, ip)) {
			goto resumeCallFrame;
		}
		DISPATCH();
	}

	do_CALL_VOID_NATIVE_FUNCTION: {
		if (!callNativeFunction<false>(currentCallFrame, currentFunction,
		                               bytecodes, ip)) {
			goto resumeCallFrame;
		}
		// if (state == VMState::WAITING) {
		// 	return;
		// }
		DISPATCH();
	}

	do_CALL_VTABLE_FUNCTION: {
		if (!callFunction<true, true, false>(currentCallFrame, currentFunction,
		                                     bytecodes, ip)) {
			goto resumeCallFrame;
		}
		DISPATCH();
	}

	do_CALL_VTABLE_VOID_FUNCTION: {
		if (!callFunction<true, false, false>(currentCallFrame, currentFunction,
		                                      bytecodes, ip)) {
			goto resumeCallFrame;
		}
		// if (state == VMState::WAITING) {
		// 	return;
		// }
		DISPATCH();
	}

	do_CREATE_FUNCTION_OBJECT: {
		Function *func = data.functions[get_u32(bytecodes, ip)];
		uint32_t size = get_u32(bytecodes, ip);
		AObject **args = new AObject *[func->argSize];
		for (uint32_t idx = size; idx-- > 0;) {
			args[idx] = stack.pop();
		}
		auto funcObj = new FunctionObject(size, args, func);
		auto obj = data.manager.get(funcObj);
		obj->retain();
		stack.push(obj);
		DISPATCH();
	}

	do_CREATE_FUNCTION_OBJECT_FROM_VTABLE: {
		auto funcPos = get_u32(bytecodes, ip);
		AObject **args = new AObject *[1];
		args[0] = stack.pop();
		Function *func =
		    data.functions[data.classes[args[0]->type]->vtable[funcPos]];
		auto funcObj = new FunctionObject(1, args, func);
		auto obj = data.manager.get(funcObj);
		obj->retain();
		stack.push(obj);
		DISPATCH();
	}

	do_CALL_DATA_CONTRUCTOR: {
		if (!callFunction<false, true, true>(currentCallFrame, currentFunction,
		                                     bytecodes, ip)) {
			goto resumeCallFrame;
		}
		DISPATCH();
	}

	do_FOR_LIST: {
		AObject *list = stack.pop();
		bool isGlobal = bytecodes[ip++] == Opcode::STORE_GLOBAL;
		AObject **iterator;
		AObject **container;
		if (isGlobal) {
			container = &globalVariables[get_u32(bytecodes, ip)];
			iterator = &globalVariables[get_u32(bytecodes, ip)];
		} else {
			container = &stackAllocator[get_u32(bytecodes, ip)];
			iterator = &stackAllocator[get_u32(bytecodes, ip)];
		}
		// if (!list) {
		// 	ip = get_u32(bytecodes, ip);
		// 	break;
		// }
		if (*iterator == DefaultClass::nullObject) {
			if (list->member->size == 0) {
				ip = get_u32(bytecodes, ip);
				DISPATCH();
			}
			*iterator = data.manager.createIntObject(0);
			*container = list->member->data[0];
			(*container)->retain();
			ip += 4;
			DISPATCH();
		}
		data.manager.release(*container);
		uint32_t newIndex = ++(*iterator)->i;
		if (list->member->size == newIndex) {
			data.manager.release(*iterator);
			*iterator = nullptr;
			ip = get_u32(bytecodes, ip);
			DISPATCH();
		}
		*container = list->member->data[newIndex];
		(*container)->retain();
		ip += 4;
		DISPATCH();
	}

	do_FOR_SET: {
		auto setObject = stack.pop();
		auto unorderedSetData =
		    static_cast<AutoLang::Libs::set::AUnorderedSet *>(
		        setObject->data->data);
		bool isGlobal = bytecodes[ip++] == Opcode::STORE_GLOBAL;
		AObject **iterator;
		AObject **container;
		if (isGlobal) {
			container = &globalVariables[get_u32(bytecodes, ip)];
			iterator = &globalVariables[get_u32(bytecodes, ip)];
		} else {
			container = &stackAllocator[get_u32(bytecodes, ip)];
			iterator = &stackAllocator[get_u32(bytecodes, ip)];
		}

		switch (unorderedSetData->type) {
			case DefaultClass::intClassId: {
				auto set = static_cast<AutoLang::Libs::set::IntHashSet *>(
				    unorderedSetData->data);
				if (*iterator == DefaultClass::nullObject) {
					if (set->empty()) {
						ip = get_u32(bytecodes, ip);
						break;
					}
					auto it = new AutoLang::Libs::set::IntHashSet::iterator(
					    set->begin());
					*iterator = notifier->createNativeData(
					    setObject->type, it,
					    [](ANotifier &notifier,
					       void *unorderedSetData) -> void {
						    delete static_cast<
						        AutoLang::Libs::set::IntHashSet::iterator *>(
						        unorderedSetData);
					    });
					*container = notifier->createInt(**it);
					(*container)->retain();
					ip += 4;
					break;
				}
				auto &it =
				    *static_cast<AutoLang::Libs::set::IntHashSet::iterator *>(
				        (*iterator)->data->data);
				++it;
				data.manager.release(*container);
				if (it == set->end()) {
					ip = get_u32(bytecodes, ip);
					break;
				}
				*container = notifier->createInt(*it);
				(*container)->retain();
				ip += 4;
				break;
			}

			case DefaultClass::floatClassId: {
				auto set = static_cast<AutoLang::Libs::set::FloatHashSet *>(
				    unorderedSetData->data);
				if (*iterator == DefaultClass::nullObject) {
					if (set->empty()) {
						ip = get_u32(bytecodes, ip);
						break;
					}
					auto it = new AutoLang::Libs::set::FloatHashSet::iterator(
					    set->begin());
					*iterator = notifier->createNativeData(
					    setObject->type, it,
					    [](ANotifier &notifier,
					       void *unorderedSetData) -> void {
						    delete static_cast<
						        AutoLang::Libs::set::FloatHashSet::iterator *>(
						        unorderedSetData);
					    });
					*container = notifier->createFloat(**it);
					(*container)->retain();
					ip += 4;
					break;
				}
				auto &it =
				    *static_cast<AutoLang::Libs::set::FloatHashSet::iterator *>(
				        (*iterator)->data->data);
				++it;
				data.manager.release(*container);
				if (it == set->end()) {
					ip = get_u32(bytecodes, ip);
					break;
				}
				*container = notifier->createFloat(*it);
				(*container)->retain();
				ip += 4;
				break;
			}

			case DefaultClass::stringClassId: {
				auto set = static_cast<AutoLang::Libs::set::StringHashSet *>(
				    unorderedSetData->data);
				if (*iterator == DefaultClass::nullObject) {
					if (set->empty()) {
						ip = get_u32(bytecodes, ip);
						break;
					}
					auto it = new AutoLang::Libs::set::StringHashSet::iterator(
					    set->begin());
					*iterator = notifier->createNativeData(
					    setObject->type, it,
					    [](ANotifier &notifier,
					       void *unorderedSetData) -> void {
						    delete static_cast<
						        AutoLang::Libs::set::StringHashSet::iterator *>(
						        unorderedSetData);
					    });
					*container = **it;
					(*container)->retain();
					ip += 4;
					break;
				}
				auto &it = *static_cast<
				    AutoLang::Libs::set::StringHashSet::iterator *>(
				    (*iterator)->data->data);
				++it;
				data.manager.release(*container);
				if (it == set->end()) {
					ip = get_u32(bytecodes, ip);
					break;
				}
				*container = *it;
				(*container)->retain();
				ip += 4;
				break;
			}

			default: {
				auto set = static_cast<AutoLang::Libs::set::ObjectHashSet *>(
				    unorderedSetData->data);
				if (*iterator == DefaultClass::nullObject) {
					if (set->empty()) {
						ip = get_u32(bytecodes, ip);
						break;
					}
					auto it = new AutoLang::Libs::set::ObjectHashSet::iterator(
					    set->begin());
					*iterator = notifier->createNativeData(
					    setObject->type, it,
					    [](ANotifier &notifier,
					       void *unorderedSetData) -> void {
						    delete static_cast<
						        AutoLang::Libs::set::ObjectHashSet::iterator *>(
						        unorderedSetData);
					    });
					*container = **it;
					(*container)->retain();
					ip += 4;
					break;
				}
				auto &it = *static_cast<
				    AutoLang::Libs::set::ObjectHashSet::iterator *>(
				    (*iterator)->data->data);
				++it;
				data.manager.release(*container);
				if (it == set->end()) {
					ip = get_u32(bytecodes, ip);
					break;
				}
				*container = *it;
				(*container)->retain();
				ip += 4;
				break;
			}
		}

		DISPATCH();
	}

	do_IN_RANGE: {
		auto obj2 = stack.pop();
		auto obj1 = stack.pop();
		auto obj = stack.pop();
		bool isLessThan = bytecodes[ip++];
		if (isLessThan) {
			stack.push(
			    notifier->createBool(obj->i >= obj1->i && obj->i < obj2->i));
		} else {
			stack.push(
			    notifier->createBool(obj->i >= obj1->i && obj->i <= obj2->i));
		}
		data.manager.release(obj);
		data.manager.release(obj1);
		data.manager.release(obj2);
		DISPATCH();
	}

	do_LOAD_CONST: {
		stack.push(getConstObject(get_u32(bytecodes, ip)));
		// std::cerr<<stack.top()<<" created\n";
		DISPATCH();
	}

	do_LOAD_CONST_PRIMARY: {
		stack.push(data.constPool[get_u32(bytecodes, ip)]);
		// stack.top()->retain();
		DISPATCH();
	}

	do_POP: {
		auto obj = stack.pop();
		if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
			--obj->refCount;
		}
		data.manager.tryRelease(obj);
		DISPATCH();
	}

	do_POP_NO_RELEASE: {
		auto obj = stack.pop();
		if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
			--obj->refCount;
		}
		DISPATCH();
	}

	do_RETURN_LOCAL: {
		while (stack.getSize() > currentCallFrame->startStackCount) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		AObject *&last = stackAllocator[get_u32(bytecodes, ip)];
		stack.push(last);
		last = nullptr;
		goto doneReturnFunction;
	}

	do_CREATE_OBJECT: {
		ClassId classId = get_u32(bytecodes, ip);
		size_t count = static_cast<size_t>(get_u32(bytecodes, ip));
		stack.push(data.manager.get(classId, count));
		stack.top()->retain();
		DISPATCH();
	}

	do_FAST_SAVE_MEMBER: {
		ClassId classId = get_u32(bytecodes, ip);
		uint32_t count = get_u32(bytecodes, ip);
		auto obj = data.manager.get(classId, count);
		for (; count-- > 0;) {
			obj->member->data[count] = stack.pop();
		}
		obj->flags |= AObject::Flags::OBJ_IS_ARRAY;
		stack.push(obj);
		stack.top()->retain();
		DISPATCH();
	}

	do_CREATE_SET_OBJECT: {
		ClassId classId = get_u32(bytecodes, ip);
		ClassId keyId = get_u32(bytecodes, ip);
		uint32_t count = get_u32(bytecodes, ip);
		auto obj = AutoLang::Libs::set::constructor(*notifier, classId, keyId);
		obj->flags |= AObject::Flags::OBJ_IS_SET;
		tempAllocateArea[0] = obj;
		for (; count-- > 0;) {
			tempAllocateArea[1] = stack.pop();
			AutoLang::Libs::set::add(*notifier, tempAllocateArea, 2);
		}
		stack.push(obj);
		stack.top()->retain();
		DISPATCH();
	}

	do_CREATE_MAP_OBJECT: {
		ClassId classId = get_u32(bytecodes, ip);
		ClassId keyId = get_u32(bytecodes, ip);
		uint32_t count = get_u32(bytecodes, ip);
		auto obj = AutoLang::Libs::map::constructor(*notifier, classId, keyId);
		obj->flags |= AObject::Flags::OBJ_IS_MAP;
		tempAllocateArea[0] = obj;
		for (; count-- > 0;) {
			tempAllocateArea[2] = stack.pop();
			tempAllocateArea[1] = stack.pop();
			AutoLang::Libs::map::set(*notifier, tempAllocateArea, 3);
		}
		stack.push(obj);
		stack.top()->retain();
		DISPATCH();
	}

	do_CREATE_NATIVE_OBJECT: {
		ClassId classId = get_u32(bytecodes, ip);
		stack.push(
		    data.manager.get(classId, new ANativeData{nullptr, nullptr}));
		stack.top()->retain();
		DISPATCH();
	}

	do_LOAD_GLOBAL: {
		stack.push(globalVariables[get_u32(bytecodes, ip)]);
		stack.top()->retain();
		DISPATCH();
	}

	do_STORE_GLOBAL: {
		setGlobalVariables(get_u32(bytecodes, ip), stack.pop());
		DISPATCH();
	}

	do_LOAD_LOCAL: {
		uint32_t pos = get_u32(bytecodes, ip);
		AObject *obj = stackAllocator[pos];
		assert(obj != nullptr);
		stack.push(obj);
		obj->retain();
		DISPATCH();
	}

	do_STORE_LOCAL: {
		auto obj = stack.pop();
		uint32_t pos = get_u32(bytecodes, ip);
		// std::cerr << pos << " "
		//           << DefaultFunction::to_string(*notifier, obj)
		//           << " " <<
		//           data.classes[obj->type]->getName(compile) <<
		//           "\n";
		stackAllocator.set(data.manager, pos, obj);
		DISPATCH();
	}
		DATA_STORE_DATA(LOCAL_STORE_LOCAL, stackAllocator, stackAllocator)
		DATA_STORE_DATA(LOCAL_STORE_GLOBAL, stackAllocator, globalVariables)
		DATA_STORE_DATA(LOCAL_STORE_CONST, stackAllocator, data.constPool)
		DATA_STORE_DATA_CLONE(LOCAL_STORE_LOCAL_CLONE, stackAllocator,
		                      stackAllocator)
		DATA_STORE_DATA_CLONE(LOCAL_STORE_GLOBAL_CLONE, stackAllocator,
		                      globalVariables)
		DATA_STORE_DATA_CLONE(LOCAL_STORE_CONST_CLONE, stackAllocator,
		                      data.constPool)
		DATA_STORE_DATA(GLOBAL_STORE_LOCAL, globalVariables, stackAllocator)
		DATA_STORE_DATA(GLOBAL_STORE_GLOBAL, globalVariables, globalVariables)
		DATA_STORE_DATA(GLOBAL_STORE_CONST, globalVariables, data.constPool)
		DATA_STORE_DATA_CLONE(GLOBAL_STORE_LOCAL_CLONE, globalVariables,
		                      stackAllocator)
		DATA_STORE_DATA_CLONE(GLOBAL_STORE_GLOBAL_CLONE, globalVariables,
		                      globalVariables)
		DATA_STORE_DATA_CLONE(GLOBAL_STORE_CONST_CLONE, globalVariables,
		                      data.constPool)

	do_LOCAL_LOAD_MEMBER: {
		uint32_t pos = get_u32(bytecodes, ip);
		AObject *obj = stackAllocator[pos];
		AObject *member = obj->member->data[get_u32(bytecodes, ip)];
		member->retain();
		stack.push(member);
		DISPATCH();
	}

	do_GLOBAL_LOAD_MEMBER: {
		uint32_t pos = get_u32(bytecodes, ip);
		AObject *obj = globalVariables[pos];
		AObject *member = obj->member->data[get_u32(bytecodes, ip)];
		member->retain();
		stack.push(member);
		DISPATCH();
	}

	do_GLOBAL_LOAD_MEMBER_AND_STORE: {
		uint32_t pos = get_u32(bytecodes, ip);
		AObject *obj = globalVariables[pos];
		obj->member->data[get_u32(bytecodes, ip)] = stack.pop();
		DISPATCH();
	}

	do_LOCAL_LOAD_MEMBER_AND_STORE: {
		uint32_t pos = get_u32(bytecodes, ip);
		AObject *obj = stackAllocator[pos];
		// std::cerr<<currentFunction->getName(compile)<<"
		// "<<DefaultFunction::to_string(*notifier, obj)<<"
		// "<<pos<<" "<<obj->member->size<<"\n";
		obj->member->data[get_u32(bytecodes, ip)] = stack.pop();
		DISPATCH();
	}

	do_LOAD_MEMBER: {
		AObject *parent = stack.top();
		stack.top() = (*parent->member)[get_u32(bytecodes, ip)];
		stack.top()->retain();
		data.manager.release(parent);
		DISPATCH();
	}

	do_LOAD_MEMBER_IF_NNULL_OR_JUMP: {
		AObject *obj = stack.top();
		if (obj != AutoLang::DefaultClass::nullObject) {
			stack.top() = (*obj->member)[get_u32(bytecodes, ip)];
			stack.top()->retain();
			ip += 4;
			data.manager.release(obj);
		} else {
			stack.pop();
			ip += 4;
			ip = get_u32(bytecodes, ip);
		}
		DISPATCH();
	}

	do_LOAD_MEMBER_CAN_RET_NULL_OR_JUMP: {
		AObject *obj = stack.top();
		if (obj != AutoLang::DefaultClass::nullObject) {
			stack.top() = (*obj->member)[get_u32(bytecodes, ip)];
			stack.top()->retain();
			ip += 4;
			data.manager.release(obj);
		} else {
			ip += 4;
			ip = get_u32(bytecodes, ip);
		}
		DISPATCH();
	}

	do_STORE_MEMBER: {
		AObject *parent = stack.pop();
		AObject *&last = parent->member->data[get_u32(bytecodes, ip)];
		if (last != nullptr) {
			data.manager.release(last);
		}
		// New value
		last = stack.pop();
		data.manager.release(parent);
		DISPATCH();
	}

	do_RETURN: {
		while (stack.getSize() > currentCallFrame->startStackCount) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		goto doneReturnFunction;
	}

	do_RETURN_VALUE: {
		auto value = stack.pop();
		while (stack.getSize() > currentCallFrame->startStackCount) {
			auto obj = stack.pop();
			// std::cerr << "VALUE: "
			//           << DefaultFunction::to_string(*notifier,
			//           obj)
			//           << "\n";
			data.manager.release(obj);
		}
		stack.push(value);
		// std::cerr << "VALUE: "
		//           << DefaultFunction::to_string(*notifier, value)
		//           << "\n";
		goto doneReturnFunction;
	}

	do_RETURN_CONST: {
		uint32_t pos = currentCallFrame->startStackCount;
		while (stack.getSize() > pos) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		stack.push(getConstObject(get_u32(bytecodes, ip)));
		goto doneReturnFunction;
	}

	do_RETURN_GLOBAL: {
		uint32_t pos = currentCallFrame->startStackCount;
		while (stack.getSize() > pos) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		auto obj = globalVariables[get_u32(bytecodes, ip)];
		obj->retain();
		stack.push(obj);
		goto doneReturnFunction;
	}

	do_RETURN_LOCAL_MEMBER: {
		uint32_t pos = currentCallFrame->startStackCount;
		while (stack.getSize() > pos) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		uint32_t localPos = get_u32(bytecodes, ip);
		auto obj =
		    stackAllocator[localPos]->member->data[get_u32(bytecodes, ip)];
		obj->retain();
		stack.push(obj);
		goto doneReturnFunction;
	}

	do_RETURN_GLOBAL_MEMBER: {
		uint32_t pos = currentCallFrame->startStackCount;
		while (stack.getSize() > pos) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
		uint32_t globalPos = get_u32(bytecodes, ip);
		auto obj =
		    globalVariables[globalPos]->member->data[get_u32(bytecodes, ip)];
		obj->retain();
		stack.push(obj);
		goto doneReturnFunction;
	}

	do_JUMP_IF_FALSE: {
		AObject *obj = stack.pop();
		if (obj == DefaultClass::falseObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_JUMP_IF_FALSE_NO_POP: {
		AObject *obj = stack.top();
		if (obj == DefaultClass::falseObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
			stack.pop();
		}
		DISPATCH();
	}

	do_JUMP_IF_TRUE_NO_POP: {
		AObject *obj = stack.top();
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
			stack.pop();
		}
		DISPATCH();
	}

	do_JUMP: {
		ip = get_u32(bytecodes, ip);
		DISPATCH();
	}

	do_JUMP_IF_NULL: {
		AObject *obj = stack.pop();
		if (obj == AutoLang::DefaultClass::nullObject) {
			ip = get_u32(bytecodes, ip);
			DISPATCH();
		}
		// if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
		// 	data.manager.release(obj);
		// }
		ip += 4;
		DISPATCH();
	}

	do_JUMP_AND_DELETE_IF_NULL: {
		AObject *obj = stack.top();
		if (obj == AutoLang::DefaultClass::nullObject) {
			ip = get_u32(bytecodes, ip);
			stack.pop();
			// --obj->refCount;
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_JUMP_AND_SET_IF_NULL: {
		auto obj = stack.top();
		if (obj == AutoLang::DefaultClass::nullObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_JUMP_IF_NON_NULL: {
		auto obj = stack.pop();
		if (obj != AutoLang::DefaultClass::nullObject) {
			if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
				data.manager.release(obj);
			}
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_IS: {
		auto obj = stack.pop();
		uint32_t classId = get_u32(bytecodes, ip);
		stack.push(data.manager.createBoolObject(
		    obj->type == classId ||
		    data.classes[obj->type]->inheritance.get(classId)));
		// stack.top()->retain();
		data.manager.release(obj);
		DISPATCH();
	}

	do_SAFE_CAST: {
		auto obj = stack.top();
		uint32_t classId = get_u32(bytecodes, ip);
		if (obj->type == classId ||
		    data.classes[obj->type]->inheritance.get(classId)) {
			DISPATCH();
		}
		stack.pop();
		data.manager.release(obj);
		stack.push(DefaultClass::nullObject);
		// DefaultClass::nullObject->retain();
		DISPATCH();
	}

	do_UNSAFE_CAST: {
		auto obj = stack.top();
		uint32_t classId = get_u32(bytecodes, ip);
		if (obj->type == classId ||
		    data.classes[obj->type]->inheritance.get(classId)) {
			DISPATCH();
		}
		notifier->throwException("Cannot cast '" +
		                         notifier->getClassName(obj->type) + "' to " +
		                         notifier.getClassName(classId));
		data.manager.release(stack.pop());
		goto resumeCallFrame;
	}

	do_WAIT_INPUT: {
		state = VMState::WAITING;
		DISPATCH();
	}

	do_LOAD_EXCEPTION: {
		stack.push(currentCallFrame->exception);
		currentCallFrame->exception->retain();
		currentCallFrame->exception = nullptr;
		DISPATCH();
	}

	do_THROW_EXCEPTION: {
		currentCallFrame->exception = stack.pop();
		goto resumeCallFrame;
	}

	do_ADD_TRY_BLOCK: {
		currentCallFrame->catchPosition.push_back(get_u32(bytecodes, ip));
		DISPATCH();
	}

	do_REMOVE_TRY_AND_JUMP: {
		assert(!currentCallFrame->catchPosition.empty());
		currentCallFrame->catchPosition.pop_back();
		ip = get_u32(bytecodes, ip);
		DISPATCH();
	}

	do_REMOVE_TRY: {
		assert(!currentCallFrame->catchPosition.empty());
		currentCallFrame->catchPosition.pop_back();
		DISPATCH();
	}

	do_CLONE: {
		auto value = stack.top();
		switch (value->type) {
			case DefaultClass::intClassId: {
				auto newValue = notifier->createInt(value->i);
				newValue->retain();
				stack.pop();
				notifier->release(value);
				stack.push(newValue);
				break;
			}
			case DefaultClass::floatClassId: {
				auto newValue = notifier->createFloat(value->f);
				newValue->retain();
				stack.pop();
				notifier->release(value);
				stack.push(newValue);
				break;
			}
			case DefaultClass::nullClassId: {
				break;
			}
			default: {
				notifier->throwException("Cannot clone");
				goto resumeCallFrame;
			}
		}
		DISPATCH();
	}

	do_TO_INT: {
		if (!operate<AutoLang::DefaultFunction::to_int, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_TO_FLOAT: {
		if (!operate<AutoLang::DefaultFunction::to_float, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_TO_STRING: {
		if (!operate<AutoLang::DefaultFunction::to_string, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_PLUS_PLUS: {
		// if (!operate<AutoLang::DefaultFunction::plus_plus, 1>())
		// 	goto resumeCallFrame;
		++stack.top()->i;
		DISPATCH();
	}

	do_PLUS_PLUS_GLOBAL: {
		// if (!operate<AutoLang::DefaultFunction::plus_plus, 1>())
		// 	goto resumeCallFrame;
		++globalVariables[get_u32(bytecodes, ip)]->i;
		DISPATCH();
	}

	do_PLUS_PLUS_LOCAL: {
		// if (!operate<AutoLang::DefaultFunction::plus_plus, 1>())
		// 	goto resumeCallFrame;
		++stackAllocator[get_u32(bytecodes, ip)]->i;
		DISPATCH();
	}

	do_MINUS_MINUS: {
		if (!operate<AutoLang::DefaultFunction::minus_minus, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}

		DATA_CAL_DATA(GLOBAL_CAL_GLOBAL, globalVariables, globalVariables)
		DATA_CAL_DATA(GLOBAL_CAL_LOCAL, globalVariables, stackAllocator)
		DATA_CAL_DATA(GLOBAL_CAL_CONST, globalVariables, data.constPool)
		DATA_CAL_DATA_MEMBER(GLOBAL_CAL_GLOBAL_MEMBER, globalVariables,
		                     globalVariables)
		DATA_CAL_DATA_MEMBER(GLOBAL_CAL_LOCAL_MEMBER, globalVariables,
		                     stackAllocator)

		DATA_CAL_DATA(LOCAL_CAL_GLOBAL, stackAllocator, globalVariables)
		DATA_CAL_DATA(LOCAL_CAL_LOCAL, stackAllocator, stackAllocator)
		DATA_CAL_DATA(LOCAL_CAL_CONST, stackAllocator, data.constPool)
		DATA_CAL_DATA_MEMBER(LOCAL_CAL_GLOBAL_MEMBER, stackAllocator,
		                     globalVariables)
		DATA_CAL_DATA_MEMBER(LOCAL_CAL_LOCAL_MEMBER, stackAllocator,
		                     stackAllocator)

		DATA_CAL_DATA(CONST_CAL_GLOBAL, data.constPool, globalVariables)
		DATA_CAL_DATA(CONST_CAL_LOCAL, data.constPool, stackAllocator)
		DATA_CAL_DATA_MEMBER(CONST_CAL_GLOBAL_MEMBER, data.constPool,
		                     globalVariables)
		DATA_CAL_DATA_MEMBER(CONST_CAL_LOCAL_MEMBER, data.constPool,
		                     stackAllocator)

		DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_GLOBAL, globalVariables,
		                     globalVariables)
		DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_LOCAL, globalVariables,
		                     stackAllocator)
		DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_CONST, globalVariables,
		                     data.constPool)
		DATA_MEMBER_CAL_DATA_MEMBER(GLOBAL_MEMBER_CAL_GLOBAL_MEMBER,
		                            globalVariables, globalVariables)
		DATA_MEMBER_CAL_DATA_MEMBER(GLOBAL_MEMBER_CAL_LOCAL_MEMBER,
		                            globalVariables, stackAllocator)

		DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_GLOBAL, stackAllocator,
		                     globalVariables)
		DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_LOCAL, stackAllocator,
		                     stackAllocator)
		DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_CONST, stackAllocator,
		                     data.constPool)
		DATA_MEMBER_CAL_DATA_MEMBER(LOCAL_MEMBER_CAL_GLOBAL_MEMBER,
		                            stackAllocator, globalVariables)
		DATA_MEMBER_CAL_DATA_MEMBER(LOCAL_MEMBER_CAL_LOCAL_MEMBER,
		                            stackAllocator, stackAllocator)

	do_GLOBAL_CAL_CONST_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = globalVariables[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = data.constPool[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_GLOBAL_CAL_LOCAL_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = globalVariables[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = stackAllocator[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_GLOBAL_CAL_GLOBAL_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = globalVariables[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = globalVariables[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_LOCAL_CAL_CONST_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = data.constPool[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_LOCAL_CAL_LOCAL_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = stackAllocator[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_LOCAL_CAL_GLOBAL_JUMP: {
		uint8_t tablePos = bytecodes[ip++];
		tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, ip)];
		tempAllocateArea[1] = globalVariables[get_u32(bytecodes, ip)];
		auto obj = operatorTable[tablePos](*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			goto resumeCallFrame;
		}
		if (obj == DefaultClass::trueObject) {
			ip = get_u32(bytecodes, ip);
		} else {
			ip += 4;
		}
		DISPATCH();
	}

	do_PLUS: {
		if (!operate<AutoLang::DefaultFunction::plus, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_MINUS: {
		if (!operate<AutoLang::DefaultFunction::minus, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_MUL: {
		if (!operate<AutoLang::DefaultFunction::mul, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_DIVIDE: {
		if (!operate<AutoLang::DefaultFunction::divide, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_PLUS_EQUAL: {
		if (!operate<AutoLang::DefaultFunction::plus_eq, 2, false>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_MINUS_EQUAL:
		if (!operate<AutoLang::DefaultFunction::minus_eq, 2, false>())
			goto resumeCallFrame;
		DISPATCH();

	do_MUL_EQUAL:
		if (!operate<AutoLang::DefaultFunction::mul_eq, 2, false>())
			goto resumeCallFrame;
		DISPATCH();

	do_DIVIDE_EQUAL:
		if (!operate<AutoLang::DefaultFunction::divide_eq, 2, false>())
			goto resumeCallFrame;
		DISPATCH();

	do_MOD: {
		if (!operate<AutoLang::DefaultFunction::mod, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_BITWISE_AND: {
		if (!operate<AutoLang::DefaultFunction::bitwise_and, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_BITWISE_OR: {
		if (!operate<AutoLang::DefaultFunction::bitwise_or, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_NEGATIVE: {
		if (!operate<AutoLang::DefaultFunction::negative, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}
		NEGATIVE_DATA(NEGATIVE_LOCAL, stackAllocator);
		NEGATIVE_DATA(NEGATIVE_GLOBAL, globalVariables);
		NEGATIVE_DATA_MEMBER(NEGATIVE_LOCAL_MEMBER, stackAllocator);
		NEGATIVE_DATA_MEMBER(NEGATIVE_GLOBAL_MEMBER, globalVariables);

	do_NOT: {
		if (!operate<AutoLang::DefaultFunction::op_not, 1>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_NOT_LOCAL: {
		auto obj = stackAllocator[get_u32(bytecodes, ip)];
		stack.push(notifier->createBool(!obj->b));
		DISPATCH();
	}

	do_NOT_GLOBAL: {
		auto obj = globalVariables[get_u32(bytecodes, ip)];
		stack.push(notifier->createBool(!obj->b));
		DISPATCH();
	}

	do_NOT_LOCAL_MEMBER: {
		auto obj = stackAllocator[get_u32(bytecodes, ip)];
		stack.push(notifier->createBool(
		    !obj->member->data[get_u32(bytecodes, ip)]->b));
		DISPATCH();
	}

	do_NOT_GLOBAL_MEMBER: {
		auto obj = globalVariables[get_u32(bytecodes, ip)];
		stack.push(notifier->createBool(
		    !obj->member->data[get_u32(bytecodes, ip)]->b));
		DISPATCH();
	}

	do_AND_AND: {
		if (!operate<AutoLang::DefaultFunction::op_and_and, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_OR_OR: {
		if (!operate<AutoLang::DefaultFunction::op_or_or, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_EQUAL_VALUE:
		if (!operate<AutoLang::DefaultFunction::op_eqeq, 2>())
			goto resumeCallFrame;
		DISPATCH();

	do_NOTEQ_VALUE:
		if (!operate<AutoLang::DefaultFunction::op_not_eq, 2>())
			goto resumeCallFrame;
		DISPATCH();
		// Support restart(), null refcount default 2 bilion. If call
		// restart(), null will be reset to 2 bilion

	do_IS_NULL: {
		AObject *obj = stack.pop();
		if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
			--obj->refCount;
		}
		stack.push(ObjectManager::createBoolObject(
		    obj == AutoLang::DefaultClass::nullObject));
		// stack.top()->retain();
		DISPATCH();
	}

	do_IS_NON_NULL: {
		AObject *obj = stack.pop();
		if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
			--obj->refCount;
		}
		stack.push(ObjectManager::createBoolObject(
		    obj != AutoLang::DefaultClass::nullObject));
		// stack.top()->retain();
		DISPATCH();
	}

	do_LOAD_NULL: {
		stack.push(AutoLang::DefaultClass::nullObject);
		// stack.top()->retain();
		DISPATCH();
	}

	do_LOAD_TRUE: {
		stack.push(AutoLang::DefaultClass::trueObject);
		// stack.top()->retain();
		DISPATCH();
	}

	do_LOAD_FALSE: {
		assert(AutoLang::DefaultClass::falseObject != nullptr);
		stack.push(AutoLang::DefaultClass::falseObject);
		// stack.top()->retain();
		DISPATCH();
	}

	do_EQUAL_POINTER: {
		if (!operate<AutoLang::DefaultFunction::op_eq_pointer, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_NOTEQ_POINTER: {
		if (!operate<AutoLang::DefaultFunction::op_not_eq_pointer, 2>())
			goto resumeCallFrame;
		DISPATCH();
	}

	do_LESS_THAN_EQ:
		if (!operate<AutoLang::DefaultFunction::op_less_than_eq, 2>())
			goto resumeCallFrame;
		DISPATCH();

	do_LESS_THAN:
		if (!operate<AutoLang::DefaultFunction::op_less_than, 2>())
			goto resumeCallFrame;
		DISPATCH();

	do_GREATER_THAN_EQ:
		if (!operate<AutoLang::DefaultFunction::op_greater_than_eq, 2>())
			goto resumeCallFrame;
		DISPATCH();

	do_GREATER_THAN:
		if (!operate<AutoLang::DefaultFunction::op_greater_than, 2>())
			goto resumeCallFrame;
		DISPATCH();

	do_INT_FROM_INT: {
		AObject *obj = stack.pop();
		auto newObj =
		    data.manager.createIntObject(static_cast<int64_t>(obj->i));
		newObj->retain();
		stack.push(newObj);
		data.manager.release(obj);
		DISPATCH();
	}

	do_FLOAT_TO_INT: {
		AObject *obj = stack.pop();
		auto newObj =
		    data.manager.createIntObject(static_cast<int64_t>(obj->f));
		newObj->retain();
		stack.push(newObj);
		data.manager.release(obj);
		DISPATCH();
	}

	do_FLOAT_FROM_FLOAT: {
		AObject *obj = stack.pop();
		auto newObj =
		    data.manager.createFloatObject(static_cast<double>(obj->f));
		newObj->retain();
		stack.push(newObj);
		data.manager.release(obj);
		DISPATCH();
	}

	do_INT_TO_FLOAT: {
		AObject *obj = stack.pop();
		auto newObj =
		    data.manager.createFloatObject(static_cast<double>(obj->i));
		newObj->retain();
		stack.push(newObj);
		data.manager.release(obj);
		DISPATCH();
	}

	do_BOOL_TO_INT: {
		AObject *obj = stack.pop();
		auto newObj = data.manager.createIntObject(static_cast<double>(obj->b));
		newObj->retain();
		stack.push(newObj);
		DISPATCH();
	}

	do_BOOL_TO_FLOAT: {
		AObject *obj = stack.pop();
		auto newObj =
		    data.manager.createFloatObject(static_cast<double>(obj->b));
		newObj->retain();
		stack.push(newObj);
		DISPATCH();
	}

	do_I_CAL_I: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->i += b->i;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createIntObject(a->i + b->i);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_I_CAL_F: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->type = DefaultClass::floatClassId;
			a->f = (double)a->i + b->f;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject((double)a->i + b->f);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_F_CAL_F: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->f += b->f;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject(a->f + b->f);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_F_CAL_I: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->f += (double)b->i;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject(a->f + (double)b->i);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_I_MINUS_I: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		// std::cerr << a->refCount << " " << b->refCount << "\n";
		// std::cerr << data.classes[a->type]->getName(compile) << "
		// "
		//           << data.classes[b->type]->getName(compile) <<
		//           "\n";
		// std::cerr << a->i << " " << b->i << "\n";
		if (a->refCount <= 1) {
			a->i -= b->i;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createIntObject(a->i - b->i);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_I_MINUS_F: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->type = DefaultClass::floatClassId;
			a->f = (double)a->i - b->f;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject((double)a->i - b->f);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_F_MINUS_F: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->f -= b->f;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject(a->f - b->f);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}

	do_F_MINUS_I: {
		AObject *b = stack.pop();
		AObject *a = stack.top();
		if (a->refCount <= 1) {
			a->f -= b->i;
			data.manager.release(b);
			DISPATCH();
		}
		stack.top() = data.manager.createFloatObject(a->f - b->i);
		++stack.top()->refCount;
		data.manager.release(a);
		data.manager.release(b);
		DISPATCH();
	}
#undef DISPATCH

	endFunction:;
		while (stack.getSize() > currentCallFrame->startStackCount) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
	doneReturnFunction:;
		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.getTop() +
		                         currentCallFrame->func->maxDeclaration - 1);
		if (callFrames.getSize() == topCallFrame) {
			callFrames.pop();
			stackAllocator.freeTo(callFrames.getSize() != 0
			                          ? callFrames.top()->fromStackAllocator
			                          : 0);
			state = VMState::HALTED;
			return;
		}
		callFrames.pop();
		currentCallFrame = callFrames.top();
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		goto resumeCallFrame;
	} catch (const std::exception &err) {
		std::cerr << "Function " << currentFunction->getName(data)
		          << ", bytecode at position " << ip << ": "
		          << uint32_t(bytecodes[ip]) << "\n";
		throw std::runtime_error(err.what());
	}
}

} // namespace AutoLang

#endif
#undef ip
