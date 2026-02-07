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
			while (callFrames.getSize() > 0)
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
	mainCallFrame->startStackCount = 0;
	mainCallFrame->i = 0;
	mainCallFrame->catchPosition.clear();
	resume();
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n'
	          << "Total runtime : " << duration.count() << " ms" << '\n';
	if (mainCallFrame->exception) {
		std::vector<Function *> funcs;
		std::cerr << "Exception: "
		          << mainCallFrame->exception->member->data[0]->str->data
		          << "\n";
		for (uint32_t i = 0; i < callFrames.getMaxSize(); ++i) {
			auto currentCallFrame = &callFrames.objects[i];
			if (currentCallFrame->exception != mainCallFrame->exception)
				break;
			funcs.push_back(currentCallFrame->func);
		}
		for (size_t i = funcs.size(); i-- > 0;) {
			std::cerr << "At function " << funcs[i]->name << "\n";
		}
		state = VMState::ERROR;
	}
}

template <bool loadVirtual, bool hasValue, bool isConstructor>
bool AVM::callFunction(CallFrame *&currentCallFrame, Function *currentFunction,
                       uint8_t *bytecodes, uint32_t &i) {
	currentCallFrame = callFrames.push();
	currentCallFrame->fromStackAllocator =
	    stackAllocator.top + currentFunction->maxDeclaration;
	currentCallFrame->exception = nullptr;
	currentCallFrame->startStackCount = stack.getSize();
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

	notifier->callFrame = currentCallFrame;

	if (currentCallFrame->func->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto obj =
		    currentCallFrame->func->native(*notifier, stackAllocator.currentPtr,
		                                   currentCallFrame->func->argSize);
		if (currentCallFrame->exception) {
			stackAllocator.clear(
			    data.manager, currentCallFrame->fromStackAllocator,
			    stackAllocator.top + currentCallFrame->func->maxDeclaration -
			        1);
			stackAllocator.freeTo(callFrames.objects[callFrames.getSize() - 2]
			                          .fromStackAllocator);
			currentCallFrame->exception->retain();
			return false;
		}
		if constexpr (hasValue) {
			stack.push(obj);
			stack.top()->retain();
		}
		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.top +
		                         currentCallFrame->func->maxDeclaration - 1);
		callFrames.pop();
		currentCallFrame = callFrames.top();
		notifier->callFrame = currentCallFrame;
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
	} else {
		if constexpr (isConstructor) {
			AutoLang::DefaultFunction::data_constructor(
			    *notifier, stackAllocator.currentPtr,
			    currentCallFrame->func->argSize);
		}
		currentCallFrame->i = 0;
		return false;
	}
	return true;
}

void AVM::resume() {
	auto currentCallFrame = callFrames.top();
resumeCallFrame:;
	if (currentCallFrame->exception) {
		if (currentCallFrame->catchPosition.empty()) {
			// std::cerr << currentCallFrame->fromStackAllocator << " & "
			//           << stackAllocator.top << "\n";
			stackAllocator.clear(
			    data.manager, currentCallFrame->fromStackAllocator,
			    currentCallFrame->fromStackAllocator +
			        currentCallFrame->func->maxDeclaration - 1);
			while (stack.getSize() > currentCallFrame->startStackCount) {
				auto obj = stack.pop();
				data.manager.release(obj);
			}
			if (callFrames.getSize() == 1) {
				stackAllocator.freeTo(0);
				return;
			}
			callFrames.pop();
			// std::cerr<<"from "<<currentCallFrame->func->name<<"\n";
			auto oldCallFrame = callFrames.top();
			oldCallFrame->exception = currentCallFrame->exception;
			currentCallFrame = oldCallFrame;
			stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
			// std::cerr<<"from " << currentCallFrame->fromStackAllocator << "\n";
			goto resumeCallFrame;
		} else {
			// std::cerr << "First size " << currentCallFrame->catchPosition.size() << "\n";
			currentCallFrame->i = currentCallFrame->catchPosition.back();
			currentCallFrame->catchPosition.pop_back();
			// std::cerr << "Second size " << currentCallFrame->catchPosition.size() << "\n";
			// std::cerr << "Goto " << currentCallFrame->i << "\n";
		}
	}
	auto *currentFunction = currentCallFrame->func;
	auto *bytecodes = currentCallFrame->func->bytecodes.data();
	uint32_t &i = currentCallFrame->i;
	const size_t size = currentCallFrame->func->bytecodes.size();
	notifier->callFrame = currentCallFrame;
	// std::cerr << "Called function " << currentCallFrame->func->name << " " << currentCallFrame->fromStackAllocator << " with "
	//           << currentCallFrame->func->argSize << " arguments \n";
	try {
		while (i < size) {
			// std::cerr << i << '\n';
			// std::cerr << "Stack size: " << stack.getSize() << "\n";
			switch (bytecodes[i++]) {
				case AutoLang::Opcode::CALL_FUNCTION: {
					if (!callFunction<false, true, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VOID_FUNCTION: {
					if (!callFunction<false, false, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_FUNCTION: {
					if (!callFunction<true, true, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_VTABLE_VOID_FUNCTION: {
					if (!callFunction<true, false, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case AutoLang::Opcode::CALL_DATA_CONTRUCTOR: {
					if (!callFunction<false, true, true>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
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
				case AutoLang::Opcode::IS: {
					auto obj = stack.pop();
					uint32_t classId = get_u32(bytecodes, i);
					stack.push(data.manager.createBoolObject(
					    obj->type == classId ||
					    data.classes[obj->type]->inheritance.get(classId)));
					stack.top()->retain();
					data.manager.release(obj);
					break;
				}
				case AutoLang::Opcode::LOAD_EXCEPTION: {
					stack.push(currentCallFrame->exception);
					currentCallFrame->exception = nullptr;
					break;
				}
				case AutoLang::Opcode::THROW_EXCEPTION: {
					currentCallFrame->exception = stack.pop();
					goto resumeCallFrame;
				}
				case AutoLang::Opcode::ADD_TRY_BLOCK: {
					currentCallFrame->catchPosition.push_back(
					    get_u32(bytecodes, i));
					break;
				}
				case AutoLang::Opcode::REMOVE_TRY_AND_JUMP: {
					assert(!currentCallFrame->catchPosition.empty());
					currentCallFrame->catchPosition.pop_back();
					i = get_u32(bytecodes, i);
					break;
				}
				case AutoLang::Opcode::REMOVE_TRY: {
					assert(!currentCallFrame->catchPosition.empty());
					currentCallFrame->catchPosition.pop_back();
					break;
				}
				case AutoLang::Opcode::TO_INT: {
					if (!operate<AutoLang::DefaultFunction::to_int, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::TO_FLOAT: {
					if (!operate<AutoLang::DefaultFunction::to_float, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::TO_STRING: {
					if (!operate<AutoLang::DefaultFunction::to_string, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::PLUS_PLUS: {
					if (!operate<AutoLang::DefaultFunction::plus_plus, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::MINUS_MINUS: {
					if (!operate<AutoLang::DefaultFunction::minus_minus, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::PLUS: {
					if (!operate<AutoLang::DefaultFunction::plus, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::MINUS: {
					if (!operate<AutoLang::DefaultFunction::minus, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::MUL: {
					if (!operate<AutoLang::DefaultFunction::mul, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::DIVIDE: {
					if (!operate<AutoLang::DefaultFunction::divide, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::PLUS_EQUAL:
					if (!operate<AutoLang::DefaultFunction::plus_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::MINUS_EQUAL:
					if (!operate<AutoLang::DefaultFunction::minus_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::MUL_EQUAL:
					if (!operate<AutoLang::DefaultFunction::mul_eq, 2, false>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::DIVIDE_EQUAL:
					if (!operate<AutoLang::DefaultFunction::divide_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::MOD: {
					if (!operate<AutoLang::DefaultFunction::mod, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::BITWISE_AND: {
					if (!operate<AutoLang::DefaultFunction::bitwise_and, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::BITWISE_OR: {
					if (!operate<AutoLang::DefaultFunction::bitwise_or, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::NEGATIVE: {
					if (!operate<AutoLang::DefaultFunction::negative, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::NOT: {
					if (!operate<AutoLang::DefaultFunction::op_not, 1>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::AND_AND: {
					if (!operate<AutoLang::DefaultFunction::op_and_and, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::OR_OR: {
					if (!operate<AutoLang::DefaultFunction::op_or_or, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::EQUAL_VALUE:
					if (!operate<AutoLang::DefaultFunction::op_eqeq, 2>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::NOTEQ_VALUE:
					if (!operate<AutoLang::DefaultFunction::op_not_eq, 2>())
						goto resumeCallFrame;
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
					if (!operate<AutoLang::DefaultFunction::op_eq_pointer, 2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::NOTEQ_POINTER: {
					if (!operate<AutoLang::DefaultFunction::op_not_eq_pointer,
					             2>())
						goto resumeCallFrame;
					break;
				}
				case AutoLang::Opcode::LESS_THAN_EQ:
					if (!operate<AutoLang::DefaultFunction::op_less_than_eq,
					             2>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::LESS_THAN:
					if (!operate<AutoLang::DefaultFunction::op_less_than, 2>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::GREATER_THAN_EQ:
					if (!operate<AutoLang::DefaultFunction::op_greater_than_eq,
					             2>())
						goto resumeCallFrame;
					break;
				case AutoLang::Opcode::GREATER_THAN:
					if (!operate<AutoLang::DefaultFunction::op_greater_than,
					             2>())
						goto resumeCallFrame;
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
		if (callFrames.getSize() == 1)
			return;
		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.top +
		                         currentCallFrame->func->maxDeclaration - 1);
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

template <ANativeFunction native, size_t size, bool push> bool AVM::operate() {
	inputTempAllocateArea<size>();
	if constexpr (push) {
		auto obj = native(*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			clearTempAllocateArea<size>();
			return false;
		} else {
			stack.push(obj);
			obj->retain();
		}
	} else {
		native(*notifier, tempAllocateArea, size);
		if (notifier->callFrame->exception) {
			clearTempAllocateArea<size>();
			return false;
		}
	}
	clearTempAllocateArea<size>();
	return true;
}

uint32_t AVM::get_u32(uint8_t *code, uint32_t &ip) {
	uint32_t val;
	memcpy(&val, code + ip, 4);
	ip += 4;
	return val;
}

AVM::~AVM() {
	delete notifier;
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