#include "shared/DefaultOperator.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {
namespace DefaultFunction {

AObject *bitwise_and(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) & (obj2->i));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot bitwise and " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *bitwise_or(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) | (obj2->i));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot bitwise or " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_and_and(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];

	if (obj1->type == Autolang::DefaultClass::boolClassId &&
	    obj2->type == Autolang::DefaultClass::boolClassId) {
		return notifier.createBool(obj1->b && obj2->b);
	}

	notifier.throwException(
	    "Cannot use && between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_or_or(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];

	if (obj1->type == Autolang::DefaultClass::boolClassId &&
	    obj2->type == Autolang::DefaultClass::boolClassId) {
		return notifier.createBool(obj1->b || obj2->b);
	}

	notifier.throwException(
	    "Cannot use || between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *negative(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case Autolang::DefaultClass::intClassId:
			return notifier.createInt(-obj->i);
		case Autolang::DefaultClass::floatClassId:
			return notifier.createFloat(-obj->f);
		case Autolang::DefaultClass::boolClassId:
			return notifier.createInt(-static_cast<int64_t>(obj->b));
		default:
			break;
	}
	notifier.throwException(
	    "Cannot use negative operator on " +
	    notifier.vm->data.classes[obj->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_not(NativeFuncInData) {
	auto obj = args[0];
	if (obj->type == Autolang::DefaultClass::boolClassId) {
		return notifier.createBool(!obj->b);
	}
	notifier.throwException(
	    "Cannot use ! operator on " +
	    notifier.vm->data.classes[obj->type]->getName(notifier.vm->data));
	return nullptr;
}

} // namespace DefaultFunction
} // namespace Autolang
