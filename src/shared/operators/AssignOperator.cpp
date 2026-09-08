#include "shared/DefaultOperator.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"
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
			switch (obj2->type) {
				case Autolang::DefaultClass::stringClassId: {
					auto oldObj = obj1;
					auto newStrObj = notifier.createString((*obj1->str) + obj2->str);
					newStrObj->retain();
					(*static_cast<AObject **>(ptr)) = newStrObj;
					notifier.release(oldObj);
					return nullptr;
				}
				default:
					break;
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
			switch (obj2->type) {
				case Autolang::DefaultClass::stringClassId: {
					auto oldObj = obj1;
					auto newStrObj = notifier.createString((*obj1->str) + obj2->str);
					newStrObj->retain();
					if (ptr) {
						(*static_cast<AObject **>(ptr)) = newStrObj;
					}
					notifier.release(oldObj);
					return nullptr;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
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
		default:
			break;
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
