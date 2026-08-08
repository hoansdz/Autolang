#ifndef AVM_CPP
#define AVM_CPP

#include "backend/vm/AVM.hpp"
#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <functional>
#include <iostream>

namespace Autolang {

void AVM::run() {
	// std::cerr << "----------------RUNTIME----------------" << '\n';
	// auto start = std::chrono::high_resolution_clock::now();
	stackAllocator.setTop(0);
	auto mainCallFrame = callFrames.push();
	mainCallFrame->func = data.main;
	mainCallFrame->fromStackAllocator = 0;
	mainCallFrame->exception = nullptr;
	mainCallFrame->startStackCount = 0;
	mainCallFrame->i = 0;
	mainCallFrame->catchPositionIndex = 0;
#ifdef AUTOLANG_LIMIT_OPCODE
	currentLimitOpcodeCount = limitOpcodeCount;
#endif
	resume();
	switch (state) {
		case VMState::ERR: {
			assert(mainCallFrame->exception);
			std::vector<Function *> funcs;
			std::cerr << (isFatalException ? std::string("VMException: ")
			                               : std::string("Exception: "))
			          << mainCallFrame->exception->member->data[0]->str->data
			          << "\n";
			for (uint32_t i = 0; i < callFrames.getMaxSize(); ++i) {
				auto currentCallFrame = &callFrames.objects[i];
				if (currentCallFrame->exception != mainCallFrame->exception)
					break;
				funcs.push_back(currentCallFrame->func);
			}
			if (funcs.size() > 16) {
				for (size_t i = funcs.size(); i-- > funcs.size() - 8;) {
					std::cerr
					    << "  At " << funcs[i]->getName(data) << " ("
					    << (funcs[i]->functionFlags &
					                FunctionFlags::FUNC_IS_NATIVE
					            ? "Native"
					            : std::string(funcs[i]->path) + ":" +
					                  std::to_string(searchLine(
					                      funcs[i], callFrames.objects[i].i)))
					    << ")\n";
				}
				std::cerr << "  ... (" << funcs.size() - 16
				          << " frames omitted) \n";
				for (size_t i = 8; i-- > 1;) {
					std::cerr
					    << "  At " << funcs[i]->getName(data) << " ("
					    << (funcs[i]->functionFlags &
					                FunctionFlags::FUNC_IS_NATIVE
					            ? "Native"
					            : std::string(funcs[i]->path) + ":" +
					                  std::to_string(searchLine(
					                      funcs[i], callFrames.objects[i].i)))
					    << ")\n";
				}
			} else {
				for (size_t i = funcs.size(); i-- > 1;) {
					std::cerr
					    << "  At " << funcs[i]->getName(data) << " ("
					    << (funcs[i]->functionFlags &
					                FunctionFlags::FUNC_IS_NATIVE
					            ? "Native"
					            : std::string(funcs[i]->path) + ":" +
					                  std::to_string(searchLine(
					                      funcs[i], callFrames.objects[i].i)))
					    << ")\n";
				}
			}
			auto [line, path] =
			    searchMainLine(funcs[0], callFrames.objects[0].i);
			std::cerr << "  At .main ("
			          << std::string(path) + ":" + std::to_string(line)
			          << ")\n";
			break;
		}
		case VMState::WAITING: {

			break;
		}
	}
	// auto end = std::chrono::high_resolution_clock::now();
	// auto duration =
	//     std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cerr << '\n' << "Total runtime : " << duration.count() << " ms\n";
}

void AVM::input(AObject *inputData) {
	switch (state) {
		case VMState::WAITING: {
			break;
		}
		default: {
			throw std::logic_error(
			    "VM isn't required waiting, call restart() to restart");
		}
	}
	stack.push(inputData);
	inputData->retain();
	state = VMState::RUNNING;
}

template <bool loadVirtual, bool hasValue, bool isConstructor>
bool AVM::callFunction(CallFrame *&currentCallFrame, Function *currentFunction,
                       uint8_t *bytecodes, uint32_t &i) {
	if (callFrames.getSize() == callFrames.getMaxSize()) {
		notifier->throwFatalException(
		    "Runtime Error: Stack Overflow.\nDetails: "
		    "Maximum call frame limit of " +
		    std::to_string(callFrames.getMaxSize()) + " exceeded.");
		return false;
	}
	currentCallFrame = callFrames.push();
	currentCallFrame->fromStackAllocator =
	    stackAllocator.getTop() + currentFunction->maxDeclaration;
	currentCallFrame->exception = nullptr;
	currentCallFrame->startStackCount = stack.getSize();
	currentCallFrame->catchPositionIndex = data.allCatchPosition.size();
	stackAllocator.setTop(currentCallFrame->fromStackAllocator);
	uint32_t argumentCount;
	if constexpr (loadVirtual) {
		uint32_t funcPos = get_u32(bytecodes, i);
		argumentCount = get_u32(bytecodes, i);
		// std::cerr<<"Pos: "<<funcPos<<" & "<<argumentCount<<"\n";
		// Ensure
		stackAllocator.ensure(argumentCount);
		for (size_t size = argumentCount; size-- > 0;) {
			auto object = stack.pop();
			assert(object != nullptr);
			stackAllocator[size] = object;
		}
		// std::cerr<<data.classes[stackAllocator[0]->type]->getName(compile)<<"
		// type\n";
		currentCallFrame->func =
		    data.functions[data.classes[stackAllocator[0]->type]
		                       ->vtable[funcPos]];
	} else {
		uint32_t funcPos = get_u32(bytecodes, i);
		currentCallFrame->func = data.functions[funcPos];
		// Ensure
		argumentCount = currentCallFrame->func->argSize;
		stackAllocator.ensure(argumentCount);
		for (size_t size = argumentCount; size-- > 0;) {
			auto object = stack.pop();
			assert(object != nullptr);
			stackAllocator[size] = object;
		}
	}

	stackAllocator.ensure(currentCallFrame->func->maxDeclaration);

	notifier->callFrame = currentCallFrame;

	if (currentCallFrame->func->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto obj = (*currentCallFrame->func->native)(
		    *notifier, stackAllocator.currentPtr, argumentCount);
		// std::cerr << currentCallFrame->fromStackAllocator << " & "
		//           << stackAllocator.getTop() +
		//                  currentCallFrame->func->maxDeclaration - 1
		//           << "\n";
		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.getTop() +
		                         currentCallFrame->func->maxDeclaration - 1);
		if (currentCallFrame->exception) {
			// stackAllocator.clear(
			//     data.manager, currentCallFrame->fromStackAllocator,
			//     stackAllocator.getTop() +
			//     currentCallFrame->func->maxDeclaration -
			//         1);
			// stackAllocator.freeTo(callFrames.objects[callFrames.getSize() -
			// 2]
			//                           .fromStackAllocator);
			return false;
		}
		if constexpr (hasValue) {
			obj->retain();
			stack.push(obj);
		}
		// std::cerr << currentCallFrame->fromStackAllocator << " "
		//           << stackAllocator.getTop() +
		//                  currentCallFrame->func->maxDeclaration - 1
		//           << "\n";
		callFrames.pop();
		currentCallFrame = callFrames.top();
		notifier->callFrame = currentCallFrame;
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		return true;
	}
	if constexpr (isConstructor) {
		Autolang::DefaultFunction::data_constructor(
		    *notifier, stackAllocator.currentPtr, argumentCount);
	}
	currentCallFrame->i = 0;
	return false;
}

// template <bool hasValue>
// bool AVM::callNativeFunction(CallFrame *&currentCallFrame,
//                              Function *currentFunction, uint8_t *bytecodes,
//                              uint32_t &i) {
// 	uint32_t fromStackAllocator =
// 	    stackAllocator.getTop() + currentFunction->maxDeclaration;
// 	stackAllocator.setTop(fromStackAllocator);
// 	auto *func = data.functions[get_u32(bytecodes, i)];
// 	stackAllocator.ensure(func->argSize);
// 	currentCallFrame->func = func;
// 	for (size_t size = func->argSize; size-- > 0;) {
// 		auto object = stack.pop();
// 		assert(object != nullptr);
// 		stackAllocator[size] = object;
// 	}

// 	auto obj =
// 	    (*func->native)(*notifier, stackAllocator.currentPtr, func->argSize);
// 	currentCallFrame->func = currentFunction;
// 	stackAllocator.clear(data.manager, fromStackAllocator,
// 	                     fromStackAllocator + func->argSize - 1);
// 	if (currentCallFrame->exception) {
// 		if (callFrames.getSize() == callFrames.getMaxSize()) {
// 			notifier->throwException("Runtime Error: Stack Overflow.\nDetails: "
// 			                         "Maximum call frame limit of " +
// 			                         std::to_string(callFrames.getMaxSize()) +
// 			                         " exceeded.");
// 			return false;
// 		}
// 		auto callFrame = callFrames.push();
// 		callFrame->fromStackAllocator = fromStackAllocator;
// 		callFrame->func = func;
// 		callFrame->startStackCount = stack.getSize();
// 		callFrame->exception = currentCallFrame->exception;
// 		callFrame->catchPosition.clear();
// 		currentCallFrame->exception = nullptr;
// 		return false;
// 	}
// 	if constexpr (hasValue) {
// 		assert(obj != nullptr);
// 		obj->retain();
// 		stack.push(obj);
// 	}
// 	stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
// 	return true;
// }

bool AVM::callFunctionObject(AObject *obj) {
	auto funcObj = obj->function;
	uint32_t argumentCount = funcObj->function->argSize;
	uint32_t fromStackAllocator =
	    stackAllocator.getTop() +
	    ((callFrames.index != 0) ? callFrames.top()->func->maxDeclaration : 0);
	if (callFrames.getSize() == callFrames.getMaxSize()) {
		notifier->throwFatalException(
		    "Runtime Error: Stack Overflow.\nDetails: "
		    "Maximum call frame limit of " +
		    std::to_string(callFrames.getMaxSize()) + " exceeded.");
		return false;
	}
	auto currentCallFrame = callFrames.push();
	currentCallFrame->fromStackAllocator = fromStackAllocator;
	currentCallFrame->exception = nullptr;
	currentCallFrame->func = funcObj->function;
	currentCallFrame->startStackCount = stack.getSize();
	currentCallFrame->catchPositionIndex = data.allCatchPosition.size();
	stackAllocator.setTop(fromStackAllocator);

	if (funcObj->function->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		for (uint32_t i = argumentCount; i-- > funcObj->size;) {
			funcObj->args[i] = stack.pop();
		}
		auto object = (*currentCallFrame->func->native)(
		    *notifier, stackAllocator.currentPtr, argumentCount);
		if (currentCallFrame->exception) {
			// Clear here
			return false;
		}
		if (currentCallFrame->func->returnId != DefaultClass::voidClassId) {
			object->retain();
			stack.push(object);
		}
		callFrames.pop();
		currentCallFrame = callFrames.top();
		notifier->callFrame = currentCallFrame;
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		return true;
	}
	stackAllocator.ensure(argumentCount);
	for (uint32_t i = argumentCount; i-- > funcObj->size;) {
		stackAllocator[i] = stack.pop();
	}
	for (uint32_t i = 0; i < funcObj->size; ++i) {
		auto object = funcObj->args[i];
		stackAllocator[i] = object;
		object->retain();
	}
	return callFunction(currentCallFrame, argumentCount);
}

inline bool AVM::callFunction(Function *currentFunction) {
	if (callFrames.getSize() == callFrames.getMaxSize()) {
		notifier->throwFatalException(
		    "Runtime Error: Stack Overflow.\nDetails: "
		    "Maximum call frame limit of " +
		    std::to_string(callFrames.getMaxSize()) + " exceeded.");
		return false;
	}
	auto currentCallFrame = callFrames.push();
	currentCallFrame->fromStackAllocator =
	    stackAllocator.getTop() + currentFunction->maxDeclaration;
	currentCallFrame->exception = nullptr;
	currentCallFrame->func = currentFunction;
	currentCallFrame->startStackCount = stack.getSize();
	currentCallFrame->catchPositionIndex = data.allCatchPosition.size();
	stackAllocator.setTop(currentCallFrame->fromStackAllocator);
	uint32_t argumentCount = currentFunction->argSize;

	stackAllocator.ensure(argumentCount);

	for (uint32_t i = argumentCount; i-- > 0;) {
		stackAllocator[i] = stack.pop();
	}

	return callFunction(currentCallFrame, argumentCount);
}

bool AVM::callFunction(CallFrame *currentCallFrame, uint32_t argumentCount) {
	notifier->callFrame = currentCallFrame;

	if (currentCallFrame->func->functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto obj = (*currentCallFrame->func->native)(
		    *notifier, stackAllocator.currentPtr, argumentCount);
		if (currentCallFrame->exception) {
			// stackAllocator.clear(
			//     data.manager, currentCallFrame->fromStackAllocator,
			//     stackAllocator.getTop() +
			//     currentCallFrame->func->maxDeclaration -
			//         1);
			// stackAllocator.freeTo(callFrames.objects[callFrames.getSize() -
			// 2]
			//                           .fromStackAllocator);
			return false;
		}
		if (currentCallFrame->func->returnId != DefaultClass::voidClassId) {
			obj->retain();
			stack.push(obj);
		}
		// std::cerr << currentCallFrame->fromStackAllocator << " "
		//           << stackAllocator.getTop() +
		//                  currentCallFrame->func->maxDeclaration - 1
		//           << "\n";

		stackAllocator.clear(data.manager, currentCallFrame->fromStackAllocator,
		                     stackAllocator.getTop() +
		                         currentCallFrame->func->maxDeclaration - 1);
		callFrames.pop();
		currentCallFrame = callFrames.top();
		notifier->callFrame = currentCallFrame;
		stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		return true;
	}
	if (currentCallFrame->func->functionFlags &
	    FunctionFlags::FUNC_IS_DATA_CONSTRUCTOR) {
		Autolang::DefaultFunction::data_constructor(
		    *notifier, stackAllocator.currentPtr, argumentCount);
		// if (currentCallFrame->func->)
		// stack.push(stackAllocator[0]);
		// stackAllocator[0] = nullptr;
		// stackAllocator.clear(
		//     data.manager, currentCallFrame->fromStackAllocator + 1,
		//     stackAllocator.getTop() + currentCallFrame->func->maxDeclaration
		//     -
		//         1);
		// callFrames.pop();
		// currentCallFrame = callFrames.top();
		// notifier->callFrame = currentCallFrame;
		// stackAllocator.freeTo(currentCallFrame->fromStackAllocator);
		// return true;
	}
	currentCallFrame->i = 0;
	resume();

	return !(currentCallFrame->exception);
}

AObject *AVM::getConstObject(uint32_t id) {
	AObject *obj = data.constPool[id];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId: {
			auto newObj = data.manager.create(obj->i);
			newObj->refCount = 1;
			return newObj;
		}
		case Autolang::DefaultClass::floatClassId: {
			auto newObj = data.manager.create(obj->f);
			newObj->refCount = 1;
			return newObj;
		}
		default:
			return obj;
	}
}

void AVM::initGlobalVariables() {
	if (globalVariables) {
		delete[] globalVariables;
	}
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

template <size_t size> inline bool AVM::fastOperate(ANativeFunction native) {
	auto obj = native(*notifier, tempAllocateArea, size);
	if (notifier->callFrame->exception) {
		return false;
	}
	if (obj != nullptr) {
		stack.push(obj);
		obj->retain();
	}
	return true;
}

uint32_t AVM::get_u32(uint8_t *code, uint32_t &ip) {
	// uint32_t val = *reinterpret_cast<uint32_t *>(code + ip);
	uint32_t val;
	memcpy(&val, code + ip, 4);
	ip += 4;
	return val;
}

} // namespace Autolang

#endif