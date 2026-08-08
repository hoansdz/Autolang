#ifndef AVM_RUN_SWITCH_CPP
#define AVM_RUN_SWITCH_CPP

#include "backend/libs/array.hpp"
#include "backend/libs/map.hpp"
#include "backend/libs/set.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <functional>
#include <iostream>

namespace Autolang {

#define DATA_CAL_DATA(opcode, data1, data2)                                    \
	case Autolang::Opcode::opcode: {                                           \
		uint8_t tablePos = bytecodes[i++];                                     \
		tempAllocateArea[0] = data1[get_u32(bytecodes, i)];                    \
		tempAllocateArea[1] = data2[get_u32(bytecodes, i)];                    \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		break;                                                                 \
	}

#define DATA_MEMBER_CAL_DATA(opcode, data1, data2)                             \
	case Autolang::Opcode::opcode: {                                           \
		uint8_t tablePos = bytecodes[i++];                                     \
		uint32_t pos = get_u32(bytecodes, i);                                  \
		tempAllocateArea[0] = data1[pos]->member->data[get_u32(bytecodes, i)]; \
		tempAllocateArea[1] = data2[get_u32(bytecodes, i)];                    \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		break;                                                                 \
	}

#define DATA_CAL_DATA_MEMBER(opcode, data1, data2)                             \
	case Autolang::Opcode::opcode: {                                           \
		uint8_t tablePos = bytecodes[i++];                                     \
		tempAllocateArea[0] = data1[get_u32(bytecodes, i)];                    \
		uint32_t pos = get_u32(bytecodes, i);                                  \
		tempAllocateArea[1] = data2[pos]->member->data[get_u32(bytecodes, i)]; \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		break;                                                                 \
	}

#define DATA_MEMBER_CAL_DATA_MEMBER(opcode, data1, data2)                      \
	case Autolang::Opcode::opcode: {                                           \
		uint8_t tablePos = bytecodes[i++];                                     \
		uint32_t pos1 = get_u32(bytecodes, i);                                 \
		tempAllocateArea[0] =                                                  \
		    data1[pos1]->member->data[get_u32(bytecodes, i)];                  \
		uint32_t pos2 = get_u32(bytecodes, i);                                 \
		tempAllocateArea[1] =                                                  \
		    data2[pos2]->member->data[get_u32(bytecodes, i)];                  \
		if (!fastOperate<2>(operatorTable[tablePos]))                          \
			goto resumeCallFrame;                                              \
		break;                                                                 \
	}

#define DATA_STORE_DATA(opcode, data1, data2)                                  \
	case Autolang::Opcode::opcode: {                                           \
		AObject *&obj1 = data1[get_u32(bytecodes, i)];                         \
		AObject *obj2 = data2[get_u32(bytecodes, i)];                          \
		obj2->retain();                                                        \
		if (obj1 != nullptr) {                                                 \
			data.manager.release(obj1);                                        \
		}                                                                      \
		obj1 = obj2;                                                           \
		break;                                                                 \
	}

#define DATA_STORE_DATA_CLONE(opcode, data1, data2)                            \
	case Autolang::Opcode::opcode: {                                           \
		AObject *&obj1 = data1[get_u32(bytecodes, i)];                         \
		AObject *obj2 = data2[get_u32(bytecodes, i)];                          \
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
		break;                                                                 \
	}

#define NEGATIVE_DATA(opcode, data)                                            \
	case Autolang::Opcode::opcode: {                                           \
		auto obj = data[get_u32(bytecodes, i)];                                \
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
		break;                                                                 \
	}

#define NEGATIVE_DATA_MEMBER(opcode, data1)                                    \
	case Autolang::Opcode::opcode: {                                           \
		auto parent = data1[get_u32(bytecodes, i)];                            \
		auto obj = parent->member->data[get_u32(bytecodes, i)];                \
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
		break;                                                                 \
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
resumeCallFrame:;
	if (currentCallFrame->exception) {
		if (isFatalException) {
			while (data.allCatchPosition.size() >
			       currentCallFrame->catchPositionIndex) {
				data.allCatchPosition.pop_back();
			}
		}
		if (data.allCatchPosition.size() <=
		    currentCallFrame->catchPositionIndex) {
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
			currentCallFrame->i = data.allCatchPosition.back();
			data.allCatchPosition.pop_back();
			// std::cerr << "Second size " <<
			// currentCallFrame->catchPosition.size() << "\n"; std::cerr <<
			// "Goto " << currentCallFrame->i << "\n";
		}
	}
	auto *currentFunction = currentCallFrame->func;
	auto *bytecodes =
	    data.allBytecodes.data() + currentCallFrame->func->bytecodes.offset;
	uint32_t &i = currentCallFrame->i;
	const size_t size = currentCallFrame->func->bytecodes.size;
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
		while (i < size) {
			// std::cerr << i << "/" << size << "\n";
			// std::cerr << "Stack size: " << stack.getSize() << "\n";

#ifdef AUTOLANG_LIMIT_OPCODE
			if (--currentLimitOpcodeCount == 0) {
				notifier->throwFatalException("Instruction limit exceeded (" +
				                              std::to_string(limitOpcodeCount) +
				                              ")");
				goto resumeCallFrame;
			}
#endif
			if (data.manager.areaAllocator.changedMemory) {
				if (data.manager.getCurrentManagedMemory() >
				    data.manager.getMaxManagedMemory()) {
					notifier->throwMemoryLimitExceeded();
					goto resumeCallFrame;
				}
				data.manager.areaAllocator.changedMemory = false;
			}
			switch (bytecodes[i++]) {
				case Autolang::Opcode::CALL_FUNCTION_OBJECT: {
					auto obj = stack.pop();
					if (!callFunctionObject(obj)) {
						data.manager.release(obj);
						// std::cerr << "A\n";
						goto resumeCallFrame;
					}
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::CALL_FUNCTION: {
					if (!callFunction<false, true, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::CALL_VOID_FUNCTION: {
					if (!callFunction<false, false, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					// if (state == VMState::WAITING) {
					// 	return;
					// }
					break;
				}
				// case Autolang::Opcode::CALL_NATIVE_FUNCTION: {
				// 	if (!callNativeFunction<true>(
				// 	        currentCallFrame, currentFunction, bytecodes, i)) {
				// 		goto resumeCallFrame;
				// 	}
				// 	break;
				// }
				// case Autolang::Opcode::CALL_VOID_NATIVE_FUNCTION: {
				// 	if (!callNativeFunction<false>(
				// 	        currentCallFrame, currentFunction, bytecodes, i)) {
				// 		goto resumeCallFrame;
				// 	}
				// 	// if (state == VMState::WAITING) {
				// 	// 	return;
				// 	// }
				// 	break;
				// }
				case Autolang::Opcode::CALL_VTABLE_FUNCTION: {
					if (!callFunction<true, true, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::CALL_VTABLE_VOID_FUNCTION: {
					if (!callFunction<true, false, false>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					// if (state == VMState::WAITING) {
					// 	return;
					// }
					break;
				}
				case Autolang::Opcode::CREATE_FUNCTION_OBJECT: {
					Function *func = data.functions[get_u32(bytecodes, i)];
					uint32_t size = get_u32(bytecodes, i);
					AObject **args = new AObject *[func->argSize];
					for (uint32_t i = size; i-- > 0;) {
						args[i] = stack.pop();
					}
					auto funcObj = new FunctionObject(size, args, func);
					auto obj = data.manager.get(funcObj);
					obj->retain();
					stack.push(obj);
					break;
				}
				case Autolang::Opcode::CREATE_FUNCTION_OBJECT_FROM_VTABLE: {
					auto funcPos = get_u32(bytecodes, i);
					AObject **args = new AObject *[1];
					args[0] = stack.pop();
					Function *func = data.functions[data.classes[args[0]->type]
					                                    ->vtable[funcPos]];
					auto funcObj = new FunctionObject(1, args, func);
					auto obj = data.manager.get(funcObj);
					obj->retain();
					stack.push(obj);
					break;
				}
				case Autolang::Opcode::CALL_DATA_CONTRUCTOR: {
					if (!callFunction<false, true, true>(
					        currentCallFrame, currentFunction, bytecodes, i)) {
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::FOR_LIST: {
					AObject *list = stack.pop();
					bool isGlobal = bytecodes[i++] == Opcode::STORE_GLOBAL;
					AObject **iterator;
					AObject **container;
					if (isGlobal) {
						container = &globalVariables[get_u32(bytecodes, i)];
						iterator = &globalVariables[get_u32(bytecodes, i)];
					} else {
						container = &stackAllocator[get_u32(bytecodes, i)];
						iterator = &stackAllocator[get_u32(bytecodes, i)];
					}
					// if (!list) {
					// 	i = get_u32(bytecodes, i);
					// 	break;
					// }
					if (*iterator == DefaultClass::nullObject) {
						if (list->member->size == 0) {
							i = get_u32(bytecodes, i);
							break;
						}
						*iterator = data.manager.createIntObject(0);
						*container = list->member->data[0];
						(*container)->retain();
						i += 4;
						break;
					}
					data.manager.release(*container);
					uint32_t newIndex = ++(*iterator)->i;
					if (list->member->size == newIndex) {
						data.manager.release(*iterator);
						*iterator = nullptr;
						i = get_u32(bytecodes, i);
						break;
					}
					*container = list->member->data[newIndex];
					(*container)->retain();
					i += 4;
					break;
				}
				case Autolang::Opcode::FOR_SET: {
					auto setObject = stack.pop();
					auto unorderedSetData =
					    static_cast<Autolang::Libs::set::AUnorderedSet *>(
					        setObject->data->data);
					bool isGlobal = bytecodes[i++] == Opcode::STORE_GLOBAL;
					AObject **iterator;
					AObject **container;
					if (isGlobal) {
						container = &globalVariables[get_u32(bytecodes, i)];
						iterator = &globalVariables[get_u32(bytecodes, i)];
					} else {
						container = &stackAllocator[get_u32(bytecodes, i)];
						iterator = &stackAllocator[get_u32(bytecodes, i)];
					}

					switch (unorderedSetData->type) {
						case DefaultClass::intClassId: {
							auto set =
							    static_cast<Autolang::Libs::set::IntHashSet *>(
							        unorderedSetData->data);
							if (*iterator == DefaultClass::nullObject) {
								if (set->empty()) {
									i = get_u32(bytecodes, i);
									break;
								}
								auto it = new Autolang::Libs::set::IntHashSet::
								    iterator(set->begin());
								*iterator = notifier->createNativeData(
								    setObject->type, it,
								    [](ANotifier &notifier,
								       void *unorderedSetData) -> void {
									    delete static_cast<
									        Autolang::Libs::set::IntHashSet::
									            iterator *>(unorderedSetData);
								    });
								*container = notifier->createInt(**it);
								(*container)->retain();
								i += 4;
								break;
							}
							auto &it = *static_cast<
							    Autolang::Libs::set::IntHashSet::iterator *>(
							    (*iterator)->data->data);
							++it;
							data.manager.release(*container);
							if (it == set->end()) {
								i = get_u32(bytecodes, i);
								break;
							}
							*container = notifier->createInt(*it);
							(*container)->retain();
							i += 4;
							break;
						}

						case DefaultClass::floatClassId: {
							auto set = static_cast<
							    Autolang::Libs::set::FloatHashSet *>(
							    unorderedSetData->data);
							if (*iterator == DefaultClass::nullObject) {
								if (set->empty()) {
									i = get_u32(bytecodes, i);
									break;
								}
								auto it = new Autolang::Libs::set::
								    FloatHashSet::iterator(set->begin());
								*iterator = notifier->createNativeData(
								    setObject->type, it,
								    [](ANotifier &notifier,
								       void *unorderedSetData) -> void {
									    delete static_cast<
									        Autolang::Libs::set::FloatHashSet::
									            iterator *>(unorderedSetData);
								    });
								*container = notifier->createFloat(**it);
								(*container)->retain();
								i += 4;
								break;
							}
							auto &it = *static_cast<
							    Autolang::Libs::set::FloatHashSet::iterator *>(
							    (*iterator)->data->data);
							++it;
							data.manager.release(*container);
							if (it == set->end()) {
								i = get_u32(bytecodes, i);
								break;
							}
							*container = notifier->createFloat(*it);
							(*container)->retain();
							i += 4;
							break;
						}

						case DefaultClass::stringClassId: {
							auto set = static_cast<
							    Autolang::Libs::set::StringHashSet *>(
							    unorderedSetData->data);
							if (*iterator == DefaultClass::nullObject) {
								if (set->empty()) {
									i = get_u32(bytecodes, i);
									break;
								}
								auto it = new Autolang::Libs::set::
								    StringHashSet::iterator(set->begin());
								*iterator = notifier->createNativeData(
								    setObject->type, it,
								    [](ANotifier &notifier,
								       void *unorderedSetData) -> void {
									    delete static_cast<
									        Autolang::Libs::set::StringHashSet::
									            iterator *>(unorderedSetData);
								    });
								*container = **it;
								(*container)->retain();
								i += 4;
								break;
							}
							auto &it = *static_cast<
							    Autolang::Libs::set::StringHashSet::iterator *>(
							    (*iterator)->data->data);
							++it;
							data.manager.release(*container);
							if (it == set->end()) {
								i = get_u32(bytecodes, i);
								break;
							}
							*container = *it;
							(*container)->retain();
							i += 4;
							break;
						}

						default: {
							auto set = static_cast<
							    Autolang::Libs::set::ObjectHashSet *>(
							    unorderedSetData->data);
							if (*iterator == DefaultClass::nullObject) {
								if (set->empty()) {
									i = get_u32(bytecodes, i);
									break;
								}
								auto it = new Autolang::Libs::set::
								    ObjectHashSet::iterator(set->begin());
								*iterator = notifier->createNativeData(
								    setObject->type, it,
								    [](ANotifier &notifier,
								       void *unorderedSetData) -> void {
									    delete static_cast<
									        Autolang::Libs::set::ObjectHashSet::
									            iterator *>(unorderedSetData);
								    });
								*container = **it;
								(*container)->retain();
								i += 4;
								break;
							}
							auto &it = *static_cast<
							    Autolang::Libs::set::ObjectHashSet::iterator *>(
							    (*iterator)->data->data);
							++it;
							data.manager.release(*container);
							if (it == set->end()) {
								i = get_u32(bytecodes, i);
								break;
							}
							*container = *it;
							(*container)->retain();
							i += 4;
							break;
						}
					}

					break;
				}
				case Autolang::Opcode::IN_RANGE: {
					auto obj2 = stack.pop();
					auto obj1 = stack.pop();
					auto obj = stack.pop();
					bool isLessThan = bytecodes[i++];
					if (isLessThan) {
						stack.push(notifier->createBool(obj->i >= obj1->i &&
						                                obj->i < obj2->i));
					} else {
						stack.push(notifier->createBool(obj->i >= obj1->i &&
						                                obj->i <= obj2->i));
					}
					data.manager.release(obj);
					data.manager.release(obj1);
					data.manager.release(obj2);
					break;
				}
				case Autolang::Opcode::LOAD_CONST: {
					stack.push(getConstObject(get_u32(bytecodes, i)));
					// std::cerr<<stack.top()<<" created\n";
					break;
				}
				case Autolang::Opcode::LOAD_CONST_PRIMARY: {
					stack.push(data.constPool[get_u32(bytecodes, i)]);
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::POP: {
					auto obj = stack.pop();
					if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
						--obj->refCount;
					}
					data.manager.tryRelease(obj);
					break;
				}
				case Autolang::Opcode::POP_NO_RELEASE: {
					auto obj = stack.pop();
					if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
						--obj->refCount;
					}
					break;
				}
				case Autolang::Opcode::CHECK_FORCE_NON_NULL: {
					auto obj = stack.top();
					if (obj == DefaultClass::nullObject) {
						notifier->throwException(
						    "Cannot unwrap null value with '!' operator");
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::RETURN_LOCAL: {
					while (stack.getSize() >
					       currentCallFrame->startStackCount) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					AObject *&last = stackAllocator[get_u32(bytecodes, i)];
					stack.push(last);
					last = nullptr;
					goto doneReturnFunction;
				}
				case Autolang::Opcode::CREATE_OBJECT: {
					ClassId classId = get_u32(bytecodes, i);
					size_t count = static_cast<size_t>(get_u32(bytecodes, i));
					stack.push(data.manager.get(classId, count));
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::FAST_SAVE_MEMBER: {
					ClassId classId = get_u32(bytecodes, i);
					uint32_t count = get_u32(bytecodes, i);
					auto obj = data.manager.get(classId, count);
					for (; count-- > 0;) {
						obj->member->data[count] = stack.pop();
					}
					obj->flags |= AObject::Flags::OBJ_IS_ARRAY;
					stack.push(obj);
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::CREATE_SET_OBJECT: {
					ClassId classId = get_u32(bytecodes, i);
					ClassId keyId = get_u32(bytecodes, i);
					uint32_t count = get_u32(bytecodes, i);
					auto obj = Autolang::Libs::set::constructor(*notifier,
					                                            classId, keyId);
					obj->flags |= AObject::Flags::OBJ_IS_SET;
					tempAllocateArea[0] = obj;
					for (; count-- > 0;) {
						tempAllocateArea[1] = stack.pop();
						Autolang::Libs::set::add(*notifier, tempAllocateArea,
						                         2);
					}
					stack.push(obj);
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::CREATE_MAP_OBJECT: {
					ClassId classId = get_u32(bytecodes, i);
					ClassId keyId = get_u32(bytecodes, i);
					uint32_t count = get_u32(bytecodes, i);
					auto obj = Autolang::Libs::map::constructor(*notifier,
					                                            classId, keyId);
					obj->flags |= AObject::Flags::OBJ_IS_MAP;
					tempAllocateArea[0] = obj;
					for (; count-- > 0;) {
						tempAllocateArea[2] = stack.pop();
						tempAllocateArea[1] = stack.pop();
						Autolang::Libs::map::set(*notifier, tempAllocateArea,
						                         3);
					}
					stack.push(obj);
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::CREATE_NATIVE_OBJECT: {
					ClassId classId = get_u32(bytecodes, i);
					stack.push(data.manager.get(
					    classId, new ANativeData{nullptr, nullptr}));
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::LOAD_GLOBAL: {
					stack.push(globalVariables[get_u32(bytecodes, i)]);
					stack.top()->retain();
					break;
				}
				case Autolang::Opcode::STORE_GLOBAL: {
					setGlobalVariables(get_u32(bytecodes, i), stack.pop());
					break;
				}
				case Autolang::Opcode::LOAD_LOCAL: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = stackAllocator[pos];
					assert(obj != nullptr);
					stack.push(obj);
					obj->retain();
					break;
				}
				case Autolang::Opcode::STORE_LOCAL: {
					auto obj = stack.pop();
					uint32_t pos = get_u32(bytecodes, i);
					// std::cerr << pos << " "
					//           << DefaultFunction::to_string(*notifier, obj)
					//           << " " <<
					//           data.classes[obj->type]->getName(compile) <<
					//           "\n";
					stackAllocator.set(data.manager, pos, obj);
					break;
				}
					DATA_STORE_DATA(LOCAL_STORE_LOCAL, stackAllocator,
					                stackAllocator)
					DATA_STORE_DATA(LOCAL_STORE_GLOBAL, stackAllocator,
					                globalVariables)
					DATA_STORE_DATA(LOCAL_STORE_CONST, stackAllocator,
					                data.constPool)
					DATA_STORE_DATA_CLONE(LOCAL_STORE_LOCAL_CLONE,
					                      stackAllocator, stackAllocator)
					DATA_STORE_DATA_CLONE(LOCAL_STORE_GLOBAL_CLONE,
					                      stackAllocator, globalVariables)
					DATA_STORE_DATA_CLONE(LOCAL_STORE_CONST_CLONE,
					                      stackAllocator, data.constPool)
					DATA_STORE_DATA(GLOBAL_STORE_LOCAL, globalVariables,
					                stackAllocator)
					DATA_STORE_DATA(GLOBAL_STORE_GLOBAL, globalVariables,
					                globalVariables)
					DATA_STORE_DATA(GLOBAL_STORE_CONST, globalVariables,
					                data.constPool)
					DATA_STORE_DATA_CLONE(GLOBAL_STORE_LOCAL_CLONE,
					                      globalVariables, stackAllocator)
					DATA_STORE_DATA_CLONE(GLOBAL_STORE_GLOBAL_CLONE,
					                      globalVariables, globalVariables)
					DATA_STORE_DATA_CLONE(GLOBAL_STORE_CONST_CLONE,
					                      globalVariables, data.constPool)
				case Autolang::Opcode::LOCAL_LOAD_MEMBER: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = stackAllocator[pos];
					AObject *member = obj->member->data[get_u32(bytecodes, i)];
					member->retain();
					stack.push(member);
					break;
				}
				case Autolang::Opcode::GLOBAL_LOAD_MEMBER: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = globalVariables[pos];
					AObject *member = obj->member->data[get_u32(bytecodes, i)];
					member->retain();
					stack.push(member);
					break;
				}
				case Autolang::Opcode::LOCAL_LOAD_LATEINIT_MEMBER: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = stackAllocator[pos];
					uint32_t memberIndex = get_u32(bytecodes, i);
					AObject *member = obj->member->data[memberIndex];
					if (member) {
						member->retain();
						stack.push(member);
						break;
					}
					auto clazz = data.classes[obj->type];
					std::string className = notifier->getClassName(obj->type);
					for (auto &[name, index] : clazz->memberMap) {
						if (index != memberIndex)
							continue;
						notifier->throwException("Member '" + name +
						                         "' at class '" + className +
						                         "' is uninitialized ");
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::GLOBAL_LOAD_LATEINIT_MEMBER: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = globalVariables[pos];
					uint32_t memberIndex = get_u32(bytecodes, i);
					AObject *member = obj->member->data[memberIndex];
					if (member) {
						member->retain();
						stack.push(member);
						break;
					}
					auto clazz = data.classes[obj->type];
					std::string className = notifier->getClassName(obj->type);
					for (auto &[name, index] : clazz->memberMap) {
						if (index != memberIndex)
							continue;
						notifier->throwException("Member '" + name +
						                         "' at class '" + className +
						                         "' is uninitialized ");
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::GLOBAL_LOAD_MEMBER_AND_STORE: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = globalVariables[pos];
					obj->member->data[get_u32(bytecodes, i)] = stack.pop();
					break;
				}
				case Autolang::Opcode::LOCAL_LOAD_MEMBER_AND_STORE: {
					uint32_t pos = get_u32(bytecodes, i);
					AObject *obj = stackAllocator[pos];
					// std::cerr<<currentFunction->getName(compile)<<"
					// "<<DefaultFunction::to_string(*notifier, obj)<<"
					// "<<pos<<" "<<obj->member->size<<"\n";
					obj->member->data[get_u32(bytecodes, i)] = stack.pop();
					break;
				}
				case Autolang::Opcode::LOAD_MEMBER: {
					AObject *parent = stack.top();
					stack.top() = (*parent->member)[get_u32(bytecodes, i)];
					stack.top()->retain();
					data.manager.release(parent);
					break;
				}
				case Autolang::Opcode::LOAD_LATEINIT_MEMBER: {
					AObject *parent = stack.top();
					uint32_t memberIndex = get_u32(bytecodes, i);
					AObject *member = parent->member->data[memberIndex];
					if (member) {
						stack.top() = member;
						member->retain();
						data.manager.release(parent);
						break;
					}
					auto clazz = data.classes[parent->type];
					std::string className =
					    notifier->getClassName(parent->type);
					for (auto &[name, index] : clazz->memberMap) {
						if (index != memberIndex)
							continue;
						data.manager.release(parent);
						notifier->throwException("Member '" + name +
						                         "' at class '" + className +
						                         "' is uninitialized ");
						goto resumeCallFrame;
					}
					break;
				}
				case Autolang::Opcode::LOAD_MEMBER_IF_NNULL_OR_JUMP: {
					AObject *obj = stack.top();
					if (obj != Autolang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
						stack.top()->retain();
						i += 4;
						data.manager.release(obj);
					} else {
						stack.pop();
						i += 4;
						i = get_u32(bytecodes, i);
					}
					break;
				}
				case Autolang::Opcode::LOAD_MEMBER_CAN_RET_NULL_OR_JUMP: {
					AObject *obj = stack.top();
					if (obj != Autolang::DefaultClass::nullObject) {
						stack.top() = (*obj->member)[get_u32(bytecodes, i)];
						stack.top()->retain();
						i += 4;
						data.manager.release(obj);
					} else {
						i += 4;
						i = get_u32(bytecodes, i);
					}
					break;
				}
				case Autolang::Opcode::STORE_MEMBER: {
					AObject *parent = stack.pop();
					AObject *&last =
					    parent->member->data[get_u32(bytecodes, i)];
					if (last != nullptr) {
						data.manager.release(last);
					}
					// New value
					last = stack.pop();
					data.manager.release(parent);
					break;
				}
				case Autolang::Opcode::RETURN: {
					while (stack.getSize() >
					       currentCallFrame->startStackCount) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					goto doneReturnFunction;
				}
				case Autolang::Opcode::RETURN_VALUE: {
					auto value = stack.pop();
					while (stack.getSize() >
					       currentCallFrame->startStackCount) {
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
				case Autolang::Opcode::RETURN_CONST: {
					uint32_t pos = currentCallFrame->startStackCount;
					while (stack.getSize() > pos) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					stack.push(getConstObject(get_u32(bytecodes, i)));
					goto doneReturnFunction;
				}
				case Autolang::Opcode::RETURN_GLOBAL: {
					uint32_t pos = currentCallFrame->startStackCount;
					while (stack.getSize() > pos) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					auto obj = globalVariables[get_u32(bytecodes, i)];
					obj->retain();
					stack.push(obj);
					goto doneReturnFunction;
				}
				case Autolang::Opcode::RETURN_LOCAL_MEMBER: {
					uint32_t pos = currentCallFrame->startStackCount;
					while (stack.getSize() > pos) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					uint32_t localPos = get_u32(bytecodes, i);
					auto obj = stackAllocator[localPos]
					               ->member->data[get_u32(bytecodes, i)];
					obj->retain();
					stack.push(obj);
					goto doneReturnFunction;
				}
				case Autolang::Opcode::RETURN_GLOBAL_MEMBER: {
					uint32_t pos = currentCallFrame->startStackCount;
					while (stack.getSize() > pos) {
						auto obj = stack.pop();
						data.manager.release(obj);
					}
					uint32_t globalPos = get_u32(bytecodes, i);
					auto obj = globalVariables[globalPos]
					               ->member->data[get_u32(bytecodes, i)];
					obj->retain();
					stack.push(obj);
					goto doneReturnFunction;
				}
				case Autolang::Opcode::JUMP_IF_FALSE: {
					AObject *obj = stack.pop();
					if (obj == DefaultClass::falseObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::JUMP_IF_FALSE_NO_POP: {
					AObject *obj = stack.top();
					if (obj == DefaultClass::falseObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
						stack.pop();
					}
					break;
				}
				case Autolang::Opcode::JUMP_IF_TRUE_NO_POP: {
					AObject *obj = stack.top();
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
						stack.pop();
					}
					break;
				}
				case Autolang::Opcode::JUMP: {
					i = get_u32(bytecodes, i);
					break;
				}
				case Autolang::Opcode::JUMP_IF_NULL: {
					AObject *obj = stack.pop();
					if (obj == Autolang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						break;
					}
					// if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
					// 	data.manager.release(obj);
					// }
					i += 4;
					break;
				}
				case Autolang::Opcode::JUMP_AND_DELETE_IF_NULL: {
					AObject *obj = stack.top();
					if (obj == Autolang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
						stack.pop();
						// --obj->refCount;
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::JUMP_AND_SET_IF_NULL: {
					auto obj = stack.top();
					if (obj == Autolang::DefaultClass::nullObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::JUMP_IF_NON_NULL: {
					auto obj = stack.pop();
					if (obj != Autolang::DefaultClass::nullObject) {
						if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
							data.manager.release(obj);
						}
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::IS: {
					auto obj = stack.pop();
					uint32_t classId = get_u32(bytecodes, i);
					stack.push(data.manager.createBoolObject(
					    notifier->instanceof(obj, classId)));
					// stack.top()->retain();
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::SAFE_CAST: {
					auto obj = stack.top();
					uint32_t classId = get_u32(bytecodes, i);
					if (obj->type == classId ||
					    data.classes[obj->type]->inheritance.get(classId)) {
						break;
					}
					stack.pop();
					data.manager.release(obj);
					stack.push(DefaultClass::nullObject);
					// DefaultClass::nullObject->retain();
					break;
				}
				case Autolang::Opcode::UNSAFE_CAST: {
					auto obj = stack.top();
					uint32_t classId = get_u32(bytecodes, i);
					if (obj->type == classId ||
					    data.classes[obj->type]->inheritance.get(classId)) {
						break;
					}
					notifier->throwException(
					    "Cannot cast '" + notifier->getClassName(obj->type) +
					    "' to '" + notifier->getClassName(classId) + "'");
					data.manager.release(stack.pop());
					goto resumeCallFrame;
				}
				case Autolang::Opcode::WAIT_INPUT: {
					state = VMState::WAITING;
					break;
				}
				case Autolang::Opcode::LOAD_EXCEPTION: {
					stack.push(currentCallFrame->exception);
					currentCallFrame->exception->retain();
					currentCallFrame->exception = nullptr;
					break;
				}
				case Autolang::Opcode::THROW_EXCEPTION: {
					currentCallFrame->exception = stack.pop();
					goto resumeCallFrame;
				}
				case Autolang::Opcode::ADD_TRY_BLOCK: {
					data.allCatchPosition.push_back(get_u32(bytecodes, i));
					break;
				}
				case Autolang::Opcode::REMOVE_TRY_AND_JUMP: {
					assert(data.allCatchPosition.size() >
					       currentCallFrame->catchPositionIndex);
					data.allCatchPosition.pop_back();
					i = get_u32(bytecodes, i);
					break;
				}
				case Autolang::Opcode::REMOVE_TRY: {
					assert(data.allCatchPosition.size() >
					       currentCallFrame->catchPositionIndex);
					data.allCatchPosition.pop_back();
					break;
				}
				case Autolang::Opcode::CLONE: {
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
					break;
				}
				case Autolang::Opcode::TO_INT: {
					if (!operate<Autolang::DefaultFunction::to_int, 1>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::TO_FLOAT: {
					if (!operate<Autolang::DefaultFunction::to_float, 1>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::TO_STRING: {
					if (!operate<Autolang::DefaultFunction::to_string, 1>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::PLUS_PLUS: {
					// if (!operate<Autolang::DefaultFunction::plus_plus, 1>())
					// 	goto resumeCallFrame;
					++stack.top()->i;
					break;
				}
				case Autolang::Opcode::PLUS_PLUS_GLOBAL: {
					// if (!operate<Autolang::DefaultFunction::plus_plus, 1>())
					// 	goto resumeCallFrame;
					++globalVariables[get_u32(bytecodes, i)]->i;
					break;
				}
				case Autolang::Opcode::PLUS_PLUS_LOCAL: {
					// if (!operate<Autolang::DefaultFunction::plus_plus, 1>())
					// 	goto resumeCallFrame;
					++stackAllocator[get_u32(bytecodes, i)]->i;
					break;
				}
				case Autolang::Opcode::MINUS_MINUS: {
					if (!operate<Autolang::DefaultFunction::minus_minus, 1>())
						goto resumeCallFrame;
					break;
				}

					DATA_CAL_DATA(GLOBAL_CAL_GLOBAL, globalVariables,
					              globalVariables)
					DATA_CAL_DATA(GLOBAL_CAL_LOCAL, globalVariables,
					              stackAllocator)
					DATA_CAL_DATA(GLOBAL_CAL_CONST, globalVariables,
					              data.constPool)
					DATA_CAL_DATA_MEMBER(GLOBAL_CAL_GLOBAL_MEMBER,
					                     globalVariables, globalVariables)
					DATA_CAL_DATA_MEMBER(GLOBAL_CAL_LOCAL_MEMBER,
					                     globalVariables, stackAllocator)

					DATA_CAL_DATA(LOCAL_CAL_GLOBAL, stackAllocator,
					              globalVariables)
					DATA_CAL_DATA(LOCAL_CAL_LOCAL, stackAllocator,
					              stackAllocator)
					DATA_CAL_DATA(LOCAL_CAL_CONST, stackAllocator,
					              data.constPool)
					DATA_CAL_DATA_MEMBER(LOCAL_CAL_GLOBAL_MEMBER,
					                     stackAllocator, globalVariables)
					DATA_CAL_DATA_MEMBER(LOCAL_CAL_LOCAL_MEMBER, stackAllocator,
					                     stackAllocator)

					DATA_CAL_DATA(CONST_CAL_GLOBAL, data.constPool,
					              globalVariables)
					DATA_CAL_DATA(CONST_CAL_LOCAL, data.constPool,
					              stackAllocator)
					DATA_CAL_DATA_MEMBER(CONST_CAL_GLOBAL_MEMBER,
					                     data.constPool, globalVariables)
					DATA_CAL_DATA_MEMBER(CONST_CAL_LOCAL_MEMBER, data.constPool,
					                     stackAllocator)

					DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_GLOBAL,
					                     globalVariables, globalVariables)
					DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_LOCAL,
					                     globalVariables, stackAllocator)
					DATA_MEMBER_CAL_DATA(GLOBAL_MEMBER_CAL_CONST,
					                     globalVariables, data.constPool)
					DATA_MEMBER_CAL_DATA_MEMBER(GLOBAL_MEMBER_CAL_GLOBAL_MEMBER,
					                            globalVariables,
					                            globalVariables)
					DATA_MEMBER_CAL_DATA_MEMBER(GLOBAL_MEMBER_CAL_LOCAL_MEMBER,
					                            globalVariables, stackAllocator)

					DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_GLOBAL,
					                     stackAllocator, globalVariables)
					DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_LOCAL, stackAllocator,
					                     stackAllocator)
					DATA_MEMBER_CAL_DATA(LOCAL_MEMBER_CAL_CONST, stackAllocator,
					                     data.constPool)
					DATA_MEMBER_CAL_DATA_MEMBER(LOCAL_MEMBER_CAL_GLOBAL_MEMBER,
					                            stackAllocator, globalVariables)
					DATA_MEMBER_CAL_DATA_MEMBER(LOCAL_MEMBER_CAL_LOCAL_MEMBER,
					                            stackAllocator, stackAllocator)

				case Autolang::Opcode::GLOBAL_CAL_CONST_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] =
					    globalVariables[get_u32(bytecodes, i)];
					tempAllocateArea[1] = data.constPool[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::GLOBAL_CAL_LOCAL_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] =
					    globalVariables[get_u32(bytecodes, i)];
					tempAllocateArea[1] = stackAllocator[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::GLOBAL_CAL_GLOBAL_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] =
					    globalVariables[get_u32(bytecodes, i)];
					tempAllocateArea[1] =
					    globalVariables[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::LOCAL_CAL_CONST_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, i)];
					tempAllocateArea[1] = data.constPool[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::LOCAL_CAL_LOCAL_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, i)];
					tempAllocateArea[1] = stackAllocator[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::LOCAL_CAL_GLOBAL_JUMP: {
					uint8_t tablePos = bytecodes[i++];
					tempAllocateArea[0] = stackAllocator[get_u32(bytecodes, i)];
					tempAllocateArea[1] =
					    globalVariables[get_u32(bytecodes, i)];
					auto obj = operatorTable[tablePos](*notifier,
					                                   tempAllocateArea, size);
					if (notifier->callFrame->exception) {
						goto resumeCallFrame;
					}
					if (obj == DefaultClass::trueObject) {
						i = get_u32(bytecodes, i);
					} else {
						i += 4;
					}
					break;
				}
				case Autolang::Opcode::PLUS: {
					if (!operate<Autolang::DefaultFunction::plus, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::MINUS: {
					if (!operate<Autolang::DefaultFunction::minus, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::MUL: {
					if (!operate<Autolang::DefaultFunction::mul, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::DIVIDE: {
					if (!operate<Autolang::DefaultFunction::divide, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::PLUS_EQUAL: {
					if (!operate<Autolang::DefaultFunction::plus_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::MINUS_EQUAL:
					if (!operate<Autolang::DefaultFunction::minus_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::MUL_EQUAL:
					if (!operate<Autolang::DefaultFunction::mul_eq, 2, false>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::DIVIDE_EQUAL:
					if (!operate<Autolang::DefaultFunction::divide_eq, 2,
					             false>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::MOD: {
					if (!operate<Autolang::DefaultFunction::mod, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::BITWISE_AND: {
					if (!operate<Autolang::DefaultFunction::bitwise_and, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::BITWISE_OR: {
					if (!operate<Autolang::DefaultFunction::bitwise_or, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::NEGATIVE: {
					if (!operate<Autolang::DefaultFunction::negative, 1>())
						goto resumeCallFrame;
					break;
				}
					NEGATIVE_DATA(NEGATIVE_LOCAL, stackAllocator);
					NEGATIVE_DATA(NEGATIVE_GLOBAL, globalVariables);
					NEGATIVE_DATA_MEMBER(NEGATIVE_LOCAL_MEMBER, stackAllocator);
					NEGATIVE_DATA_MEMBER(NEGATIVE_GLOBAL_MEMBER,
					                     globalVariables);
				case Autolang::Opcode::NOT: {
					if (!operate<Autolang::DefaultFunction::op_not, 1>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::NOT_LOCAL: {
					auto obj = stackAllocator[get_u32(bytecodes, i)];
					stack.push(notifier->createBool(!obj->b));
					break;
				}
				case Autolang::Opcode::NOT_GLOBAL: {
					auto obj = globalVariables[get_u32(bytecodes, i)];
					stack.push(notifier->createBool(!obj->b));
					break;
				}
				case Autolang::Opcode::NOT_LOCAL_MEMBER: {
					auto obj = stackAllocator[get_u32(bytecodes, i)];
					stack.push(notifier->createBool(
					    !obj->member->data[get_u32(bytecodes, i)]->b));
					break;
				}
				case Autolang::Opcode::NOT_GLOBAL_MEMBER: {
					auto obj = globalVariables[get_u32(bytecodes, i)];
					stack.push(notifier->createBool(
					    !obj->member->data[get_u32(bytecodes, i)]->b));
					break;
				}
				case Autolang::Opcode::AND_AND: {
					if (!operate<Autolang::DefaultFunction::op_and_and, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::OR_OR: {
					if (!operate<Autolang::DefaultFunction::op_or_or, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::EQUAL_VALUE:
					if (!operate<Autolang::DefaultFunction::op_eqeq, 2>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::NOTEQ_VALUE:
					if (!operate<Autolang::DefaultFunction::op_not_eq, 2>())
						goto resumeCallFrame;
					break;
				// Support restart(), null refcount default 2 bilion. If call
				// restart(), null will be reset to 2 bilion
				case Autolang::Opcode::IS_NULL: {
					AObject *obj = stack.pop();
					if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
						--obj->refCount;
					}
					stack.push(ObjectManager::createBoolObject(
					    obj == Autolang::DefaultClass::nullObject));
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::IS_NON_NULL: {
					AObject *obj = stack.pop();
					if (!(obj->flags & AObject::Flags::OBJ_IS_CONST)) {
						--obj->refCount;
					}
					stack.push(ObjectManager::createBoolObject(
					    obj != Autolang::DefaultClass::nullObject));
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::LOAD_NULL: {
					stack.push(Autolang::DefaultClass::nullObject);
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::LOAD_TRUE: {
					stack.push(Autolang::DefaultClass::trueObject);
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::LOAD_FALSE: {
					assert(Autolang::DefaultClass::falseObject != nullptr);
					stack.push(Autolang::DefaultClass::falseObject);
					// stack.top()->retain();
					break;
				}
				case Autolang::Opcode::EQUAL_POINTER: {
					if (!operate<Autolang::DefaultFunction::op_eq_pointer, 2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::NOTEQ_POINTER: {
					if (!operate<Autolang::DefaultFunction::op_not_eq_pointer,
					             2>())
						goto resumeCallFrame;
					break;
				}
				case Autolang::Opcode::LESS_THAN_EQ:
					if (!operate<Autolang::DefaultFunction::op_less_than_eq,
					             2>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::LESS_THAN:
					if (!operate<Autolang::DefaultFunction::op_less_than, 2>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::GREATER_THAN_EQ:
					if (!operate<Autolang::DefaultFunction::op_greater_than_eq,
					             2>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::GREATER_THAN:
					if (!operate<Autolang::DefaultFunction::op_greater_than,
					             2>())
						goto resumeCallFrame;
					break;
				case Autolang::Opcode::INT_FROM_INT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createIntObject(
					    static_cast<int64_t>(obj->i));
					newObj->retain();
					stack.push(newObj);
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::FLOAT_TO_INT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createIntObject(
					    static_cast<int64_t>(obj->f));
					newObj->retain();
					stack.push(newObj);
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::FLOAT_FROM_FLOAT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createFloatObject(
					    static_cast<double>(obj->f));
					newObj->retain();
					stack.push(newObj);
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::INT_TO_FLOAT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createFloatObject(
					    static_cast<double>(obj->i));
					newObj->retain();
					stack.push(newObj);
					data.manager.release(obj);
					break;
				}
				case Autolang::Opcode::BOOL_TO_INT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createIntObject(
					    static_cast<double>(obj->b));
					newObj->retain();
					stack.push(newObj);
					break;
				}
				case Autolang::Opcode::BOOL_TO_FLOAT: {
					AObject *obj = stack.pop();
					auto newObj = data.manager.createFloatObject(
					    static_cast<double>(obj->b));
					newObj->retain();
					stack.push(newObj);
					break;
				}
				case Autolang::Opcode::I_CAL_I: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->i += b->i;
						data.manager.release(b);
						break;
					}
					stack.top() = data.manager.createIntObject(a->i + b->i);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::I_CAL_F: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->type = DefaultClass::floatClassId;
						a->f = (double)a->i + b->f;
						data.manager.release(b);
						break;
					}
					stack.top() =
					    data.manager.createFloatObject((double)a->i + b->f);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::F_CAL_F: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->f += b->f;
						data.manager.release(b);
						break;
					}
					stack.top() = data.manager.createFloatObject(a->f + b->f);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::F_CAL_I: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->f += (double)b->i;
						data.manager.release(b);
						break;
					}
					stack.top() =
					    data.manager.createFloatObject(a->f + (double)b->i);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::I_MINUS_I: {
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
						break;
					}
					stack.top() = data.manager.createIntObject(a->i - b->i);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::I_MINUS_F: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->type = DefaultClass::floatClassId;
						a->f = (double)a->i - b->f;
						data.manager.release(b);
						break;
					}
					stack.top() =
					    data.manager.createFloatObject((double)a->i - b->f);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::F_MINUS_F: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->f -= b->f;
						data.manager.release(b);
						break;
					}
					stack.top() = data.manager.createFloatObject(a->f - b->f);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				case Autolang::Opcode::F_MINUS_I: {
					AObject *b = stack.pop();
					AObject *a = stack.top();
					if (a->refCount <= 1) {
						a->f -= b->i;
						data.manager.release(b);
						break;
					}
					stack.top() = data.manager.createFloatObject(a->f - b->i);
					++stack.top()->refCount;
					data.manager.release(a);
					data.manager.release(b);
					break;
				}
				default:
					throw std::runtime_error("Bytecode not be defined");
			}
		}
	endFunction:;
		while (stack.getSize() > currentCallFrame->startStackCount) {
			auto obj = stack.pop();
			data.manager.release(obj);
		}
	doneReturnFunction:;
		while (data.allCatchPosition.size() >
		       currentCallFrame->catchPositionIndex) {
			data.allCatchPosition.pop_back();
		}
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
		          << ", bytecode at position " << i << ": "
		          << uint32_t(bytecodes[i]) << "\n";
		throw std::runtime_error(err.what());
	}
}

} // namespace Autolang

#endif