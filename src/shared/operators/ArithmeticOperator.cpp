#include "shared/DefaultOperator.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"
#include <cmath>

namespace Autolang {
namespace DefaultFunction {

AObject *plus(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) + (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) + (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) + (obj2->b));
				case Autolang::DefaultClass::stringClassId:
					return notifier.createString(
					    AString::plus((obj1->i), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) + (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) + (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) + (obj2->b));
				case Autolang::DefaultClass::stringClassId:
					return notifier.createString(
					    AString::plus((obj1->f), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) + (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) + (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) + (obj2->b));
				case Autolang::DefaultClass::stringClassId:
					return notifier.createString(AString::plus(
					    (obj1->b ? "true" : "false"), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::stringClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createString((*obj1->str) + (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createString((*obj1->str) + (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createString((*obj1->str) +
					                             (obj2->b ? "true" : "false"));
				case Autolang::DefaultClass::stringClassId:
					return notifier.createString((*obj1->str) + (obj2->str));
				default:
					break;
			}
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot plus " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *minus(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) - (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) - (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) - (obj2->b));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) - (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) - (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) - (obj2->b));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) - (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) - (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) - (obj2->b));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot minus " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *mul(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) * (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) * (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) * (obj2->b));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) * (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) * (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) * (obj2->b));
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) * (obj2->i));
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) * (obj2->f));
				case Autolang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) * (obj2->b));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot multiply " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *divide(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->i) / (obj2->i));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->i) / (obj2->f));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->i) / (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->i));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->f));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->b) / (obj2->i));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->b) / (obj2->f));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->b) / (obj2->b));
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
	    "Cannot divide " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
divideByZero:;
	notifier.throwException("Cannot divide by zero");
	return nullptr;
}

AObject *mod(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->i) % (obj2->i));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->i), (obj2->f)));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt(obj1->i);
				}
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->f), (obj2->i)));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->f), (obj2->f)));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createFloat(obj1->f);
				}
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->b) % (obj2->i));
				}
				case Autolang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->b), (obj2->f)));
				}
				case Autolang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->b) % (obj2->b));
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
	    "Cannot mod " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
divideByZero:;
	notifier.throwException("Cannot divide by zero");
	return nullptr;
}

} // namespace DefaultFunction
} // namespace Autolang
