#ifndef AVM_CPP
#define AVM_CPP

#include "backend/vm/AVM.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <functional>
#include <iostream>

namespace AutoLang {

void AVM::run() {
	switch (state) {
		case VMState::INIT: {
			throw std::logic_error("Virtual machine isn't inited");
		}
		default: {
			while (callFrames.index != 0)
				callFrames.pop();
			break;
		}
	}
	std::cerr << "-------------------" << '\n';
	std::cerr << "Runtime" << '\n';
	auto start = std::chrono::high_resolution_clock::now();
	stackAllocator.top = 0;
	auto mainCallFrame = callFrames.push();
	mainCallFrame->func = data.main;
	mainCallFrame->fromStackAllocator = 0;
	mainCallFrame->exception = nullptr;
	mainCallFrame->i = 0;
	mainCallFrame->catchPosition.clear();
	resume();
}

template <bool loadVirtual>
CallFrame *AVM::callFunction(Function *currentFunction, uint8_t *bytecodes,
                             uint32_t &i) {
	auto currentCallFrame = callFrames.push();
	currentCallFrame->fromStackAllocator =
	    stackAllocator.top + currentFunction->maxDeclaration;
	currentCallFrame->exception = nullptr;
	currentCallFrame->catchPosition.clear();
	stackAllocator.setTop(currentCallFrame->fromStackAllocator);
	if constexpr (loadVirtual) {
		uint32_t funcPos = get_u32(bytecodes, i);
		uint32_t argumentCount = get_u32(bytecodes, i);
		// std::cerr<<"Pos: "<<funcPos<<" & "<<argumentCount<<"\n";
		// Ensure
		stackAllocator.ensure(argumentCount);
		for (size_t size = argumentCount; size-- > 0;) {
			auto object = stack.pop();
			assert(object != nullptr);
			stackAllocator[size] = object;
		}
		// std::cerr<<data.classes[stackAllocator[0]->type]->name<<" type\n";
		currentCallFrame->func =
		    data.functions[data.classes[stackAllocator[0]->type]
		                       ->vtable[funcPos]];
		stackAllocator.ensure(currentCallFrame->func->argSize);
	} else {
		currentCallFrame->func = data.functions[get_u32(bytecodes, i)];
		// Ensure
		stackAllocator.ensure(currentCallFrame->func->argSize);
		for (size_t size = currentCallFrame->func->argSize; size-- > 0;) {
			auto object = stack.pop();
			assert(object != nullptr);
			stackAllocator[size] = object;
		}
	}

	return currentCallFrame;
}

void AVM::resume() {
	auto currentCallFrame = callFrames.top();
resumeCallFrame:;
	auto *currentFunction = currentCallFrame->func;
	auto *bytecodes = currentCallFrame->func->bytecodes.data();
	uint32_t &i = currentCallFrame->i;
	const size_t size = currentCallFrame->func->bytecodes.size();
	// std::cerr << "Called function " << currentCallFrame->func->name << " "
	//           << currentCallFrame->func->argSize << " with arguments \n";
	try {
		while (i < size) {
			// std::cerr << i << '\n';
			// std::cerr << "Stack size: " << stack.index << "\n";
			switch (bytecodes[i++]) {
				case AutoLang::Opcode::CALL_FUNCTION: {
					currentCallFrame =
					    callFunction<false>(currentFunction, bytecodes, i);
					if (currentCallFrame->func->functionFlags &
					    FunctionFlags::FUNC_IS_NATIVE) {
						stack.push(currentCallFrame->func->native(
						    data.manager,
						    stackAllocator.args + stackAllocator.top,
						    currentCallFrame->func->argSize));
						stack.top()->retain();
						stackAllocator.clear(
						    data.manager, currentCallFrame->fromStackAllocator,
						    stackAllocator.top +
						        currentCallFrame->func->maxDeclaration - 1);
						callFrames.pop();
						currentCallFrame = callFrames.top();
						stackAllocator.freeTo(
						    currentCallFrame->fromStackAllocator);
					} else {
						currentCallFrame->i = 0;
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VOID_FUNCTION: {
					currentCallFrame =
					    callFunction<false>(currentFunction, bytecodes, i);
					if (currentCallFrame->func->functionFlags &
					    FunctionFlags::FUNC_IS_NATIVE) {
						currentCallFrame->func->native(
						    data.manager,
						    stackAllocator.args + stackAllocator.top,
						    currentCallFrame->func->argSize);
						stackAllocator.clear(
						    data.manager, currentCallFrame->fromStackAllocator,
						    stackAllocator.top +
						        currentCallFrame->func->maxDeclaration - 1);
						callFrames.pop();
						currentCallFrame = callFrames.top();
						stackAllocator.freeTo(
						    currentCallFrame->fromStackAllocator);
					} else {
						currentCallFrame->i = 0;
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_FUNCTION: {
					currentCallFrame =
					    callFunction<true>(currentFunction, bytecodes, i);
					if (currentCallFrame->func->functionFlags &
					    FunctionFlags::FUNC_IS_NATIVE) {
						stack.push(currentCallFrame->func->native(
						    data.manager, stackAllocator.currentPtr,
						    currentCallFrame->func->argSize));
						stack.top()->retain();
						stackAllocator.clear(
						    data.manager, currentCallFrame->fromStackAllocator,
						    stackAllocator.top +
						        currentCallFrame->func->maxDeclaration - 1);
						callFrames.pop();
						currentCallFrame = callFrames.top();
						stackAllocator.freeTo(
						    currentCallFrame->fromStackAllocator);
					} else {
						currentCallFrame->i = 0;
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_VOID_FUNCTION: {
					currentCallFrame =
					    callFunction<true>(currentFunction, bytecodes, i);
					if (currentCallFrame->func->functionFlags &
					    FunctionFlags::FUNC_IS_NATIVE) {
						currentCallFrame->func->native(
						    data.manager, stackAllocator.currentPtr,
						    currentCallFrame->func->argSize);
						stackAllocator.clear(
						    data.manager, currentCallFrame->fromStackAllocator,
						    stackAllocator.top +
						        currentCallFrame->func->maxDeclaration - 1);
						callFrames.pop();
						currentCallFrame = callFrames.top();
						stackAllocator.freeTo(
						    currentCallFrame->fromStackAllocator);
					} else {
						currentCallFrame->i = 0;
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_DATA_CONTRUCTOR: {
					currentCallFrame =
					    callFunction<false>(currentFunction, bytecodes, i);
					AutoLang::DefaultFunction::data_constructor(
					    data.manager, stackAllocator.currentPtr,
					    currentCallFrame->func->argSize);
					currentCallFrame->i = 0;
					goto resumeCallFrame;
				}
				case AutoLang::Opcode::LOAD_CONST: {
					stack.push(getConstObject(get_u32(bytecodes, i)));
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::LOAD_CONST_PRIMARY: {
					stack.push(data.constPool[get_u32(bytecodes, i)]);
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::POP: {
					auto obj = stack.pop();
					--obj->refCount;
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::POP_NO_RELEASE: {
					auto obj = stack.pop();
					--obj->refCount;
					break;
				}
				case AutoLang::Opcode::RETURN_LOCAL: {
					AObject **last = &stackAllocator[get_u32(bytecodes, i)];
					stack.push(*last);
					*last = nullptr;
					goto endFunction;
				}
				case AutoLang::Opcode::CREATE_OBJECT: {
					uint32_t type = get_u32(bytecodes, i);
					size_t count = static_cast<size_t>(get_u32(bytecodes, i));
					stack.push(data.manager.get(type, count));
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::LOAD_GLOBAL: {
					stack.push(globalVariables[get_u32(bytecodes, i)]);
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::STORE_GLOBAL: {
					setGlobalVariables(get_u32(bytecodes, i), stack.pop());
					break;
				}
				case AutoLang::Opcode::LOAD_LOCAL: {
					stack.push(stackAllocator[get_u32(bytecodes, i)]);
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::STORE_LOCAL: {
					stackAllocator.set(data.manager, get_u32(bytecodes, i),
					                   stack.pop());
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER: {
					auto obj = stack.top();
					--obj->refCount;
					stack.top() = (*obj->member)[get_u32(bytecodes, i)];
					stack.top()->retain();
					// if (obj->refCount == 0) data.manager.release()
					// # Memory leaks example A().b, we can't free A because b
					// will be destroyed
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER_IF_NNULL: {
					auto obj = stack.top();
					--obj->refCount;
					if (obj != AutoLang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
						stack.top()->retain();
					} else {
						stack.pop();
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::LOAD_MEMBER_CAN_RET_NULL: {
					auto obj = stack.top();
					--obj->refCount;
					if (obj != AutoLang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
					} else {
						i += 4;
					}
					break;
				}
				case AutoLang::Opcode::STORE_MEMBER: {
					AObject *parent = stack.pop();
					--parent->refCount;
					AObject **last =
					    &parent->member->data[get_u32(bytecodes, i)];
					if (*last != nullptr) {
						data.manager.release(*last);
					}
					// New value
					*last = stack.pop();
					break;
				}
				case AutoLang::Opcode::RETURN: {
					goto endFunction;
				}
				case AutoLang::Opcode::RETURN_VALUE: {
					goto endFunction;
				}
				case AutoLang::Opcode::JUMP_IF_FALSE: {
					AObject *obj = stack.pop();
					--obj->refCount;
					if (!obj->b) {
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
					AObject *obj = stack.pop();
					--obj->refCount;
					if (obj == AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						break;
					}
					i += 4;
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::JUMP_AND_DELETE_IF_NULL: {
					AObject *obj = stack.top();
					if (obj == AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						stack.pop();
						--obj->refCount;
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
					--obj->refCount;
					if (obj != AutoLang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						data.manager.tryRelease(obj);
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
				// Support restart(), null refcount default 2 bilion. If call
				// restart(), null will be reset to 2 bilion
				case AutoLang::Opcode::IS_NULL: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(ObjectManager::createBoolObject(
					    obj == AutoLang::DefaultClass::nullObject));
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::IS_NON_NULL: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(ObjectManager::createBoolObject(
					    obj != AutoLang::DefaultClass::nullObject));
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::LOAD_NULL: {
					stack.push(AutoLang::DefaultClass::nullObject);
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::LOAD_TRUE: {
					stack.push(AutoLang::DefaultClass::trueObject);
					stack.top()->retain();
					break;
				}
				case AutoLang::Opcode::LOAD_FALSE: {
					assert(AutoLang::DefaultClass::falseObject != nullptr);
					stack.push(AutoLang::DefaultClass::falseObject);
					stack.top()->retain();
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
				case AutoLang::Opcode::INT_FROM_INT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createIntObject(
					    static_cast<int64_t>(obj->i)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::FLOAT_TO_INT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createIntObject(
					    static_cast<int64_t>(obj->f)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::FLOAT_FROM_FLOAT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createFloatObject(
					    static_cast<int64_t>(obj->f)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::INT_TO_FLOAT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createFloatObject(
					    static_cast<double>(obj->i)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::BOOL_TO_INT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createIntObject(
					    static_cast<int64_t>(obj->b)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				case AutoLang::Opcode::BOOL_TO_FLOAT: {
					AObject *obj = stack.pop();
					--obj->refCount;
					stack.push(data.manager.createFloatObject(
					    static_cast<double>(obj->b)));
					stack.top()->retain();
					data.manager.tryRelease(obj);
					break;
				}
				default:
					throw std::runtime_error("Bytecode not be defined");
			}
		}
	endFunction:;
		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.top +
		                         currentCallFrame->func->maxDeclaration - 1);
		if (callFrames.index == 1)
			return;
		callFrames.pop();
		currentCallFrame = callFrames.top();
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		goto resumeCallFrame;
	} catch (const std::exception &err) {
		std::cerr << "Function " << currentFunction->name
		          << ", bytecode at position " << i << ": "
		          << uint32_t(bytecodes[i]) << "\n";
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
}

template <ANativeFunction native, size_t size, bool push> void AVM::operate() {
	inputTempAllocateArea<size>();
	if constexpr (push) {
		stack.push(native(data.manager, tempAllocateArea, size));
		stack.top()->retain();
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
	delete[] tempAllocateArea;
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

} // namespace AutoLang

#endif