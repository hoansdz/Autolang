#ifndef AVM_CPP
#define AVM_CPP

#include "backend/vm/AVM.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <functional>
#include <iostream>

void AVM::run() {
	std::cerr << "-------------------" << '\n';
	std::cerr << "Runtime" << '\n';
	auto start = std::chrono::high_resolution_clock::now();
	stackAllocator.top = 0;
	callFrames.allocate(128);
	run(callFrames.push(data.main));
	callFrames.pop();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n'
	          << "Total runtime : " << duration.count() << " ms" << '\n';
}

template <bool loadVirtual, bool hasValue>
void AVM::callFunction(Function *currentFunction, uint8_t *bytecodes,
                       uint32_t &i) {
	const size_t top = stackAllocator.top + currentFunction->maxDeclaration;
	stackAllocator.setTop(top);
	Function *func;
	if constexpr (loadVirtual) {
		uint32_t funcPos = get_u32(bytecodes, i);
		uint32_t argumentCount = get_u32(bytecodes, i);
		// Ensure
		stackAllocator.ensure(argumentCount);
		for (size_t size = argumentCount; size-- > 0;) {
			auto object = stack.pop();
			if (object != nullptr)
				object->retain();
			stackAllocator[size] = object;
		}
		func = data.functions[data.classes[stackAllocator[0]->type]
		                          ->vtable[funcPos]];
	} else {
		func = data.functions[get_u32(bytecodes, i)];
		// Ensure
		stackAllocator.ensure(func->nullableArgs.size);
		for (size_t size = func->nullableArgs.size; size-- > 0;) {
			auto object = stack.pop();
			if (object != nullptr)
				object->retain();
			stackAllocator[size] = object;
		}
	}
	// std::cerr << "Calling " << func->name << "\n";
	if constexpr (hasValue) {
		AObject *value;
		if (!func->bytecodes.empty()) {
			value = run(callFrames.push(func));
			callFrames.pop();
		}
		
		if (func->native) {
			value = func->native(data.manager,
									stackAllocator.args + stackAllocator.top,
									func->nullableArgs.size);
		}
		stack.push(value);
	} else {
		if (!func->bytecodes.empty()) {
			run(callFrames.push(func));
			callFrames.pop();
		}
		
		if (func->native) {
			func->native(data.manager, stackAllocator.args + stackAllocator.top,
			             func->nullableArgs.size);
		}
	}

	stackAllocator.clear(data.manager, top, top + func->maxDeclaration - 1);
	stackAllocator.freeTo(top - currentFunction->maxDeclaration);
}

AObject *AVM::run(CallFrame *callFrame) {
	auto *currentFunction = callFrame->func;
	auto *bytecodes = callFrame->func->bytecodes.data();
	uint32_t &i = callFrame->i;
	const size_t size = callFrame->func->bytecodes.size();
	try {
		while (i < size) {
			// std::cerr << i << '\n';
			switch (bytecodes[i++]) {
				case AutoLang::Opcode::CALL_FUNCTION: {
					callFunction<false, true>(currentFunction, bytecodes, i);
					break;
				}
				case AutoLang::Opcode::CALL_VOID_FUNCTION: {
					callFunction<false, false>(currentFunction, bytecodes, i);
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_FUNCTION: {
					callFunction<true, true>(currentFunction, bytecodes, i);
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_VOID_FUNCTION: {
					callFunction<true, false>(currentFunction, bytecodes, i);
					break;
				}
				case AutoLang::Opcode::LOAD_CONST: {
					stack.push(getConstObject(get_u32(bytecodes, i)));
					break;
				}
				case AutoLang::Opcode::LOAD_CONST_PRIMARY: {
					stack.push(data.constPool[get_u32(bytecodes, i)]);
					break;
				}
				case AutoLang::Opcode::POP: {
					data.manager.release(stack.pop());
					break;
				}
				case AutoLang::Opcode::POP_NO_RELEASE: {
					stack.pop();
					break;
				}
				case AutoLang::Opcode::RETURN_LOCAL: {
					AObject **last = &stackAllocator[get_u32(bytecodes, i)];
					AObject *obj = *last;
					if (obj->refCount > 0) {
						--obj->refCount;
					}
					stack.push(obj);
					*last = nullptr;
					return obj;
				}
				case AutoLang::Opcode::CREATE_OBJECT: {
					uint32_t type = get_u32(bytecodes, i);
					size_t count = static_cast<size_t>(get_u32(bytecodes, i));
					stack.push(data.manager.get(type, count));
					break;
				}
				case AutoLang::Opcode::LOAD_GLOBAL: {
					stack.push(globalVariables[get_u32(bytecodes, i)]);
					break;
				}
				case AutoLang::Opcode::STORE_GLOBAL: {
					setGlobalVariables(get_u32(bytecodes, i), stack.pop());
					break;
				}
				case AutoLang::Opcode::LOAD_LOCAL: {
					stack.push(stackAllocator[get_u32(bytecodes, i)]);
					break;
				}
				case AutoLang::Opcode::STORE_LOCAL: {
					stackAllocator.set(data.manager, get_u32(bytecodes, i),
					                   stack.pop());
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER: {
					auto obj = stack.top();
					stack.top() = (*obj->member)[get_u32(bytecodes, i)];
					// if (obj->refCount == 0) data.manager.release()
					// # Memory leaks example A().b, we can't free A because b
					// will be destroyed
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER_IF_NNULL: {
					auto obj = stack.top();
					if (obj != AutoLang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
					} else {
						stack.pop();
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER_CAN_RET_NULL: {
					auto obj = stack.top();
					if (obj != AutoLang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::STORE_MEMBER: {
					AObject **last =
					    &stack.pop()->member->data[get_u32(bytecodes, i)];
					if (*last != nullptr) {
						data.manager.release(*last);
					}
					*last = stack.pop();
					(*last)->retain();
					break;
				}
				case AutoLang::Opcode::RETURN: {
					return nullptr;
				}
				case AutoLang::Opcode::RETURN_VALUE: {
					return stack.pop();
				}
				case AutoLang::Opcode::JUMP_IF_FALSE: {
					if (!stack.pop()->b) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::JUMP: {
					i = get_u32(bytecodes, i);
					break;
				}
				case AutoLang::Opcode::JUMP_IF_NULL: {
					auto obj = stack.pop();
					if (obj == AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						break;
					}
					i += 4;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::JUMP_AND_DELETE_IF_NULL: {
					auto obj = stack.top();
					if (obj == AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						stack.pop();
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::JUMP_AND_SET_IF_NULL: {
					auto obj = stack.top();
					if (obj == AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::JUMP_IF_NON_NULL: {
					auto obj = stack.pop();
					if (obj != AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						data.manager.release(obj);
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::TO_INT: {
					operate<AutoLang::DefaultFunction::to_int, 1>();
					break;
				}
				case AutoLang::Opcode::TO_FLOAT: {
					operate<AutoLang::DefaultFunction::to_float, 1>();
					break;
				}
				case AutoLang::Opcode::TO_STRING: {
					operate<AutoLang::DefaultFunction::to_string, 1>();
					break;
				}
				case AutoLang::Opcode::PLUS_PLUS: {
					operate<AutoLang::DefaultFunction::plus_plus, 1>();
					break;
				}
				case AutoLang::Opcode::MINUS_MINUS: {
					operate<AutoLang::DefaultFunction::minus_minus, 1>();
					break;
				}
				case AutoLang::Opcode::PLUS: {
					operate<AutoLang::DefaultFunction::plus, 2>();
					break;
				}
				case AutoLang::Opcode::MINUS: {
					operate<AutoLang::DefaultFunction::minus, 2>();
					break;
				}
				case AutoLang::Opcode::MUL: {
					operate<AutoLang::DefaultFunction::mul, 2>();
					break;
				}
				case AutoLang::Opcode::DIVIDE: {
					operate<AutoLang::DefaultFunction::divide, 2>();
					break;
				}
				case AutoLang::Opcode::PLUS_EQUAL:
					operate<AutoLang::DefaultFunction::plus_eq, 2, false>();
					break;
				case AutoLang::Opcode::MINUS_EQUAL:
					operate<AutoLang::DefaultFunction::minus_eq, 2, false>();
					break;
				case AutoLang::Opcode::MUL_EQUAL:
					operate<AutoLang::DefaultFunction::mul_eq, 2, false>();
					break;
				case AutoLang::Opcode::DIVIDE_EQUAL:
					operate<AutoLang::DefaultFunction::divide_eq, 2, false>();
					break;
				case AutoLang::Opcode::MOD: {
					operate<AutoLang::DefaultFunction::mod, 2>();
					break;
				}
				case AutoLang::Opcode::BITWISE_AND: {
					operate<AutoLang::DefaultFunction::bitwise_and, 2>();
					break;
				}
				case AutoLang::Opcode::BITWISE_OR: {
					operate<AutoLang::DefaultFunction::bitwise_or, 2>();
					break;
				}
				case AutoLang::Opcode::NEGATIVE: {
					operate<AutoLang::DefaultFunction::negative, 1>();
					break;
				}
				case AutoLang::Opcode::NOT: {
					operate<AutoLang::DefaultFunction::op_not, 1>();
					break;
				}
				case AutoLang::Opcode::AND_AND: {
					operate<AutoLang::DefaultFunction::op_and_and, 2>();
					break;
				}
				case AutoLang::Opcode::OR_OR: {
					operate<AutoLang::DefaultFunction::op_or_or, 2>();
					break;
				}
				case AutoLang::Opcode::EQUAL_VALUE:
					operate<AutoLang::DefaultFunction::op_eqeq, 2>();
					break;
				case AutoLang::Opcode::NOTEQ_VALUE:
					operate<AutoLang::DefaultFunction::op_not_eq, 2>();
					break;
				case AutoLang::Opcode::IS_NULL: {
					stack.push(ObjectManager::createBoolObject(
					    stack.pop() == AutoLang::DefaultClass::nullObject));
					break;
				}
				case AutoLang::Opcode::IS_NON_NULL: {
					stack.push(ObjectManager::createBoolObject(
					    stack.pop() != AutoLang::DefaultClass::nullObject));
					break;
				}
				case AutoLang::Opcode::LOAD_NULL: {
					stack.push(AutoLang::DefaultClass::nullObject);
					break;
				}
				case AutoLang::Opcode::LOAD_TRUE: {
					stack.push(AutoLang::DefaultClass::trueObject);
					break;
				}
				case AutoLang::Opcode::LOAD_FALSE: {
					stack.push(AutoLang::DefaultClass::falseObject);
					break;
				}
				case AutoLang::Opcode::EQUAL_POINTER: {
					operate<AutoLang::DefaultFunction::op_eq_pointer, 2>();
					break;
				}
				case AutoLang::Opcode::NOTEQ_POINTER: {
					operate<AutoLang::DefaultFunction::op_not_eq_pointer, 2>();
					break;
				}
				case AutoLang::Opcode::LESS_THAN_EQ:
					operate<AutoLang::DefaultFunction::op_less_than_eq, 2>();
					break;
				case AutoLang::Opcode::LESS_THAN:
					operate<AutoLang::DefaultFunction::op_less_than, 2>();
					break;
				case AutoLang::Opcode::GREATER_THAN_EQ:
					operate<AutoLang::DefaultFunction::op_greater_than_eq, 2>();
					break;
				case AutoLang::Opcode::GREATER_THAN:
					operate<AutoLang::DefaultFunction::op_greater_than, 2>();
					break;
				case AutoLang::Opcode::FLOAT_TO_INT: {
					auto obj = stack.pop();
					stack.push(data.manager.createIntObject(
					    static_cast<int64_t>(obj->f)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::INT_TO_FLOAT: {
					auto obj = stack.pop();
					stack.push(data.manager.createFloatObject(
					    static_cast<double>(obj->i)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::BOOL_TO_INT: {
					auto obj = stack.pop();
					stack.push(data.manager.createIntObject(
					    static_cast<int64_t>(obj->b)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::BOOL_TO_FLOAT: {
					auto obj = stack.pop();
					stack.push(data.manager.createFloatObject(
					    static_cast<double>(obj->b)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::INT_TO_STRING: {
					auto obj = stack.pop();
					stack.push(data.manager.create(AString::from(obj->i)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::FLOAT_TO_STRING: {
					auto obj = stack.pop();
					stack.push(data.manager.create(AString::from(obj->f)));
					++obj->refCount;
					data.manager.release(obj);
					break;
				}
				default:
					throw std::runtime_error("Bytecode not be defined");
			}
		}
		return nullptr;
	} catch (const std::exception &err) {
		std::cerr << "Function " << currentFunction->name << ", bytecode " << i
		          << ": ";
		throw std::runtime_error(err.what());
	}
}

AObject *AVM::getConstObject(uint32_t id) {
	AObject *obj = data.constPool[id];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return data.manager.create(obj->i);
		case AutoLang::DefaultClass::floatClassId:
			return data.manager.create(obj->f);
		default:
			return obj;
	}
}

void AVM::initGlobalVariables() {
	globalVariables = new AObject *[data.main->maxDeclaration] {};
}

void AVM::setGlobalVariables(uint32_t i, AObject *object) {
	AObject **last = &globalVariables[i];
	if (*last != nullptr) {
		data.manager.release(*last);
	}
	*last = object;
	object->retain();
}

template <AObject *(*native)(NativeFuncInput), size_t size, bool push>
void AVM::operate() {
	inputTempAllocateArea<size>();
	if constexpr (push) {
		stack.push(native(data.manager, tempAllocateArea, size));
	} else {
		native(data.manager, tempAllocateArea, size);
	}
	clearTempAllocateArea<size>();
}

uint32_t AVM::get_u32(uint8_t *code, uint32_t &ip) {
	uint32_t val;
	memcpy(&val, code + ip, 4);
	ip += 4;
	return val;
}

AVM::~AVM() {
	if (globalVariables)
		delete[] globalVariables;
}

template <typename K, typename V>
size_t estimateUnorderedMapSize(const HashMap<K, V> &map) {
	size_t total = sizeof(map);
	total += map.bucket_count() * sizeof(void *); // bucket array

	for (const auto &[key, value] : map) {
		total += sizeof(std::pair<const K, V>);
		total += sizeof(key);
		total += sizeof(value);
	}

	return total;
}

#endif