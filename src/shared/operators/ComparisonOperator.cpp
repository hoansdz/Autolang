#include "shared/DefaultOperator.hpp"
#include "backend/vm/ANotifier.hpp"
#include "backend/vm/AVM.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {
namespace DefaultFunction {

AObject *op_eqeq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i == obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i == obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i == obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f == obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f == obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f == obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b == obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b == obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b == obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::stringClassId: {
			if (obj2->type == Autolang::DefaultClass::stringClassId) {
				return notifier.createBool(*obj1->str == obj2->str);
			}
			break;
		}
		default:
			break;
	}
	return notifier.createBool(obj1 == obj2);
}

bool op_eqeq(AObject *obj1, AObject *obj2) {
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return obj1->i == obj2->i;
				case Autolang::DefaultClass::floatClassId:
					return obj1->i == obj2->f;
				case Autolang::DefaultClass::boolClassId:
					return obj1->i == obj2->b;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return obj1->f == obj2->i;
				case Autolang::DefaultClass::floatClassId:
					return obj1->f == obj2->f;
				case Autolang::DefaultClass::boolClassId:
					return obj1->f == obj2->b;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return obj1->b == obj2->i;
				case Autolang::DefaultClass::floatClassId:
					return obj1->b == obj2->f;
				case Autolang::DefaultClass::boolClassId:
					return obj1->b == obj2->b;
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::stringClassId: {
			if (obj2->type == Autolang::DefaultClass::stringClassId) {
				return *obj1->str == obj2->str;
			}
			break;
		}
		default:
			break;
	}

	return obj1 == obj2;
}

AObject *op_not_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i != obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i != obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i != obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f != obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f != obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f != obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b != obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b != obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b != obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::stringClassId: {
			if (obj2->type == Autolang::DefaultClass::stringClassId) {
				return notifier.createBool(*obj1->str != obj2->str);
			}
			break;
		}
		default:
			break;
	}
	return notifier.createBool(obj1 != obj2);
}

AObject *op_less_than(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i < obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i < obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i < obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f < obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f < obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f < obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b < obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b < obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b < obj2->b);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot compare < between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_greater_than(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i > obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i > obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i > obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f > obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f > obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f > obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b > obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b > obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b > obj2->b);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot compare > between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_less_than_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i <= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i <= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i <= obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f <= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f <= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f <= obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b <= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b <= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b <= obj2->b);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot compare <= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_greater_than_eq(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case Autolang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->i >= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->i >= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->i >= obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->f >= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->f >= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->f >= obj2->b);
				default:
					break;
			}
			break;
		}
		case Autolang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createBool(obj1->b >= obj2->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createBool(obj1->b >= obj2->f);
				case Autolang::DefaultClass::boolClassId:
					return notifier.createBool(obj1->b >= obj2->b);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot compare >= between " +
	    notifier.vm->data.classes[obj1->type]->getName(notifier.vm->data) + " and " +
	    notifier.vm->data.classes[obj2->type]->getName(notifier.vm->data));
	return nullptr;
}

AObject *op_eq_pointer(NativeFuncInData) {
	return notifier.createBool(args[0] == args[1]);
}

AObject *op_not_eq_pointer(NativeFuncInData) {
	return notifier.createBool(args[0] != args[1]);
}

} // namespace DefaultFunction
} // namespace Autolang
