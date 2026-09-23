#include "shared/DefaultOperator.hpp"
#include "backend/libs/array.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/default_functions/ConversionFunctions.hpp"
#include <cmath>

namespace Autolang {
namespace DefaultFunction {

AObject *plus_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	void *ptr = notifier.vm->pointerVariable;
	ClassId ptrClassId = notifier.vm->pointerClassId;
	notifier.vm->pointerVariable = nullptr;
	notifier.vm->pointerClassId = 0;

	if (ptr != nullptr) {
		if (ptrClassId == Autolang::DefaultClass::intClassId) {
			auto valPtr = static_cast<int64_t *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr += obj2->i;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr += static_cast<int64_t>(obj2->f);
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr += obj2->b;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::floatClassId) {
			auto valPtr = static_cast<double *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr += obj2->i;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr += obj2->f;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr += obj2->b;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::stringClassId) {
			AString *resStr = nullptr;
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					resStr = (*obj1->str) + obj2->i;
					break;
				case Autolang::DefaultClass::floatClassId:
					resStr = (*obj1->str) + obj2->f;
					break;
				case Autolang::DefaultClass::boolClassId:
					resStr = (*obj1->str) + (obj2->b ? "true" : "false");
					break;
				case Autolang::DefaultClass::stringClassId:
					resStr = (*obj1->str) + obj2->str;
					break;
				case Autolang::DefaultClass::nullClassId:
					resStr = (*obj1->str) + "null";
					break;
				default: {
					std::string s = to_string(notifier, obj2);
					if (notifier.hasException())
						return nullptr;
					resStr = (*obj1->str) + s.c_str();
					break;
				}
			}
			if (resStr) {
				auto oldObj = obj1;
				auto newStrObj = notifier.createString(resStr);
				newStrObj->retain();
				(*static_cast<AObject **>(ptr)) = newStrObj;
				notifier.release(oldObj);
				return nullptr;
			}
		} else if (ptrClassId == Autolang::DefaultClass::arrayClassId ||
		           (obj1 && (obj1->flags & AObject::Flags::OBJ_IS_ARRAY))) {
			AObject *newArrObj = Libs::array::plus(notifier, args, argSize);
			if (newArrObj) {
				auto oldObj = obj1;
				newArrObj->retain();
				(*static_cast<AObject **>(ptr)) = newArrObj;
				notifier.release(oldObj);
				return nullptr;
			}
		}
	}

	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->i += obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->i += obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->i += obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->f += obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->f += obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->f += obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::stringClassId: {
			AString *resStr = nullptr;
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					resStr = (*obj1->str) + obj2->i;
					break;
				case Autolang::DefaultClass::floatClassId:
					resStr = (*obj1->str) + obj2->f;
					break;
				case Autolang::DefaultClass::boolClassId:
					resStr = (*obj1->str) + (obj2->b ? "true" : "false");
					break;
				case Autolang::DefaultClass::stringClassId:
					resStr = (*obj1->str) + obj2->str;
					break;
				case Autolang::DefaultClass::nullClassId:
					resStr = (*obj1->str) + "null";
					break;
				default: {
					std::string s = to_string(notifier, obj2);
					if (notifier.hasException())
						return nullptr;
					resStr = (*obj1->str) + s.c_str();
					break;
				}
			}
			if (resStr) {
				auto oldObj = obj1;
				auto newStrObj = notifier.createString(resStr);
				newStrObj->retain();
				if (ptr) {
					(*static_cast<AObject **>(ptr)) = newStrObj;
				}
				notifier.release(oldObj);
				return nullptr;
			}
			break;
		}
		default: {
			if (obj1->flags & AObject::Flags::OBJ_IS_ARRAY) {
				AObject *newArrObj = Libs::array::plus(notifier, args, argSize);
				if (newArrObj) {
					auto oldObj = obj1;
					newArrObj->retain();
					if (ptr) {
						(*static_cast<AObject **>(ptr)) = newArrObj;
					}
					notifier.release(oldObj);
					return nullptr;
				}
			}
			break;
		}
	}
	notifier.throwException(
	    "Cannot use += between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *minus_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	void *ptr = notifier.vm->pointerVariable;
	ClassId ptrClassId = notifier.vm->pointerClassId;
	notifier.vm->pointerVariable = nullptr;
	notifier.vm->pointerClassId = 0;

	if (ptr != nullptr) {
		if (ptrClassId == Autolang::DefaultClass::intClassId) {
			auto valPtr = static_cast<int64_t *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr -= obj2->i;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr -= static_cast<int64_t>(obj2->f);
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr -= obj2->b;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::floatClassId) {
			auto valPtr = static_cast<double *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr -= obj2->i;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr -= obj2->f;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr -= obj2->b;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::arrayClassId ||
		           (obj1 && (obj1->flags & AObject::Flags::OBJ_IS_ARRAY))) {
			AObject *newArrObj = Libs::array::minus(notifier, args, argSize);
			if (newArrObj) {
				auto oldObj = obj1;
				newArrObj->retain();
				(*static_cast<AObject **>(ptr)) = newArrObj;
				notifier.release(oldObj);
				return nullptr;
			}
		}
	}

	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->i -= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->i -= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->i -= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->f -= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->f -= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->f -= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		default: {
			if (obj1->flags & AObject::Flags::OBJ_IS_ARRAY) {
				AObject *newArrObj = Libs::array::minus(notifier, args, argSize);
				if (newArrObj) {
					auto oldObj = obj1;
					newArrObj->retain();
					if (ptr) {
						(*static_cast<AObject **>(ptr)) = newArrObj;
					}
					notifier.release(oldObj);
					return nullptr;
				}
			}
			break;
		}
	}
	notifier.throwException(
	    "Cannot use -= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *mul_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	void *ptr = notifier.vm->pointerVariable;
	ClassId ptrClassId = notifier.vm->pointerClassId;
	notifier.vm->pointerVariable = nullptr;
	notifier.vm->pointerClassId = 0;

	if (ptr != nullptr) {
		if (ptrClassId == Autolang::DefaultClass::intClassId) {
			auto valPtr = static_cast<int64_t *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr *= obj2->i;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr = static_cast<int64_t>(*valPtr * obj2->f);
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr *= obj2->b;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::floatClassId) {
			auto valPtr = static_cast<double *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					*valPtr *= obj2->i;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					*valPtr *= obj2->f;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					*valPtr *= obj2->b;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				default:
					break;
			}
		}
	}

	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->i *= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->i *= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->i *= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					obj1->f *= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					obj1->f *= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					obj1->f *= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use *= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *divide_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	void *ptr = notifier.vm->pointerVariable;
	ClassId ptrClassId = notifier.vm->pointerClassId;
	notifier.vm->pointerVariable = nullptr;
	notifier.vm->pointerClassId = 0;

	if (ptr != nullptr) {
		if (ptrClassId == Autolang::DefaultClass::intClassId) {
			auto valPtr = static_cast<int64_t *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					*valPtr /= obj2->i;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					*valPtr = static_cast<int64_t>(*valPtr / obj2->f);
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					*valPtr /= obj2->b;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::floatClassId) {
			auto valPtr = static_cast<double *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					*valPtr /= obj2->i;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					*valPtr /= obj2->f;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					*valPtr /= obj2->b;
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				default:
					break;
			}
		}
	}

	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					obj1->i /= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					obj1->i /= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					obj1->i /= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					obj1->f /= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					obj1->f /= obj2->f;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					obj1->f /= obj2->b;
					return nullptr;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use /= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;

divideByZero:;
	notifier.throwException("Cannot divide by zero in /= operation");
	return nullptr;
}

AObject *mod_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	void *ptr = notifier.vm->pointerVariable;
	ClassId ptrClassId = notifier.vm->pointerClassId;
	notifier.vm->pointerVariable = nullptr;
	notifier.vm->pointerClassId = 0;

	if (ptr != nullptr) {
		if (ptrClassId == Autolang::DefaultClass::intClassId) {
			auto valPtr = static_cast<int64_t *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					*valPtr %= obj2->i;
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					*valPtr = static_cast<int64_t>(std::fmod(*valPtr, obj2->f));
					if (obj1) obj1->i = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					return nullptr;
				default:
					break;
			}
		} else if (ptrClassId == Autolang::DefaultClass::floatClassId) {
			auto valPtr = static_cast<double *>(ptr);
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					*valPtr = std::fmod(*valPtr, obj2->i);
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					*valPtr = std::fmod(*valPtr, obj2->f);
					if (obj1) obj1->f = *valPtr;
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					return nullptr;
				default:
					break;
			}
		}
	}

	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					obj1->i %= obj2->i;
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					obj1->i = static_cast<int64_t>(std::fmod(obj1->i, obj2->f));
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					return nullptr;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					if (obj2->i == 0)
						goto divideByZero;
					obj1->f = std::fmod(obj1->f, obj2->i);
					return nullptr;
				case Autolang::DefaultClass::floatClassId:
					if (obj2->f == 0)
						goto divideByZero;
					obj1->f = std::fmod(obj1->f, obj2->f);
					return nullptr;
				case Autolang::DefaultClass::boolClassId:
					if (obj2->b == false)
						goto divideByZero;
					return nullptr;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use %= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;

divideByZero:;
	notifier.throwException("Cannot divide by zero in %= operation");
	return nullptr;
}

AObject *plus_plus(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			++obj->i;
			return obj;
		case Autolang::DefaultClass::floatClassId:
			++obj->f;
			return obj;
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use ++ operator on " +
	    notifier.vm->data.classes[obj->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *minus_minus(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			--obj->i;
			return obj;
		case Autolang::DefaultClass::floatClassId:
			--obj->f;
			return obj;
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use -- operator on " +
	    notifier.vm->data.classes[obj->type]->getName(notifier.vm->data));
	return nullptr;
}

} // namespace DefaultFunction
} // namespace Autolang
