#ifndef SYSTEM_FUNCTIONS_HPP
#define SYSTEM_FUNCTIONS_HPP

#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/default_functions/ConversionFunctions.hpp"
#include <algorithm>
#include <iostream>
#include <string>

namespace Autolang {
namespace DefaultFunction {

inline AObject *assert_(NativeFuncInData) {
	auto condition = args[0];
	if (condition->b) {
		return nullptr;
	}
	if (argSize == 1) {
		notifier.throwException("Assertion failed");
	} else if (argSize == 2) {
		if (args[1] && args[1]->type == DefaultClass::stringClassId &&
		    args[1]->str) {
			notifier.throwException(std::string(args[1]->str->data));
		} else {
			notifier.throwException("Assertion failed");
		}
	} else {
		notifier.throwException(std::string("At ") + args[1]->str->data + ":" +
		                        std::to_string(args[2]->i) + ": Wrong");
	}
	return nullptr;
}

inline AObject *data_constructor(NativeFuncInData) {
	AObject *obj = args[0];
	for (size_t i = 1; i < argSize; ++i) {
		AObject **last = &obj->member->data[i - 1];
		*last = args[i];
		(*last)->retain();
	}
	return nullptr;
}

inline AObject *print(NativeFuncInData) {
	AObject *obj = args[0];
	std::cerr << to_string(notifier, obj);
	return nullptr;
}

inline AObject *println(NativeFuncInData) {
	AObject *obj = args[0];
	std::cerr << to_string(notifier, obj) << "\n";
	return nullptr;
}

inline AObject *get_refcount(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->refCount - 1));
}

inline AObject *input_str(NativeFuncInData) {
#ifdef __EMSCRIPTEN__

#else
	std::string s;
	std::getline(std::cin, s);
	notifier.input(notifier.createString(AString::from(s)));
#endif
	return nullptr;
}

inline AObject *repeat(NativeFuncInData) {
	int64_t times = args[0]->i;
	auto funcObject = args[1];
	for (int64_t i = 0; i < times; ++i) {
		auto item = notifier.createInt(i);
		item->retain();
		(void)notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) {
			return nullptr;
		}
	}
	return nullptr;
}

inline AObject *require_(NativeFuncInData) {
	if (!args[0]->b) {
		if (argSize > 1 && args[1] && args[1]->type == DefaultClass::stringClassId && args[1]->str) {
			notifier.throwException(std::string(args[1]->str->data));
		} else {
			notifier.throwException("Requirement failed.");
		}
		return nullptr;
	}
	return nullptr;
}

inline AObject *check_(NativeFuncInData) {
	if (!args[0]->b) {
		if (argSize > 1 && args[1] && args[1]->type == DefaultClass::stringClassId && args[1]->str) {
			notifier.throwException(std::string(args[1]->str->data));
		} else {
			notifier.throwException("Check failed.");
		}
		return nullptr;
	}
	return nullptr;
}

inline AObject *error_(NativeFuncInData) {
	if (argSize > 0 && args[0] && args[0]->type == DefaultClass::stringClassId && args[0]->str) {
		notifier.throwException(std::string(args[0]->str->data));
	} else {
		notifier.throwException("Error");
	}
	return nullptr;
}

inline AObject *todo_(NativeFuncInData) {
	std::string reason = "An operation is not implemented.";
	if (argSize > 0 && args[0] && args[0]->type == DefaultClass::stringClassId && args[0]->str) {
		reason = args[0]->str->data;
	}
	notifier.throwException(std::string("TODO: ") + reason);
	return nullptr;
}

inline AObject *min_of(NativeFuncInData) {
	if (argSize == 2) {
		auto a = args[0];
		auto b = args[1];
		if (a->type == DefaultClass::intClassId && b->type == DefaultClass::intClassId) {
			return notifier.createInt(std::min(a->i, b->i));
		}
		double valA = (a->type == DefaultClass::intClassId) ? static_cast<double>(a->i) : a->f;
		double valB = (b->type == DefaultClass::intClassId) ? static_cast<double>(b->i) : b->f;
		return notifier.createFloat(std::min(valA, valB));
	} else if (argSize == 3) {
		auto a = args[0];
		auto b = args[1];
		auto c = args[2];
		if (a->type == DefaultClass::intClassId && b->type == DefaultClass::intClassId && c->type == DefaultClass::intClassId) {
			return notifier.createInt(std::min({a->i, b->i, c->i}));
		}
		double valA = (a->type == DefaultClass::intClassId) ? static_cast<double>(a->i) : a->f;
		double valB = (b->type == DefaultClass::intClassId) ? static_cast<double>(b->i) : b->f;
		double valC = (c->type == DefaultClass::intClassId) ? static_cast<double>(c->i) : c->f;
		return notifier.createFloat(std::min({valA, valB, valC}));
	}
	return nullptr;
}

inline AObject *max_of(NativeFuncInData) {
	if (argSize == 2) {
		auto a = args[0];
		auto b = args[1];
		if (a->type == DefaultClass::intClassId && b->type == DefaultClass::intClassId) {
			return notifier.createInt(std::max(a->i, b->i));
		}
		double valA = (a->type == DefaultClass::intClassId) ? static_cast<double>(a->i) : a->f;
		double valB = (b->type == DefaultClass::intClassId) ? static_cast<double>(b->i) : b->f;
		return notifier.createFloat(std::max(valA, valB));
	} else if (argSize == 3) {
		auto a = args[0];
		auto b = args[1];
		auto c = args[2];
		if (a->type == DefaultClass::intClassId && b->type == DefaultClass::intClassId && c->type == DefaultClass::intClassId) {
			return notifier.createInt(std::max({a->i, b->i, c->i}));
		}
		double valA = (a->type == DefaultClass::intClassId) ? static_cast<double>(a->i) : a->f;
		double valB = (b->type == DefaultClass::intClassId) ? static_cast<double>(b->i) : b->f;
		double valC = (c->type == DefaultClass::intClassId) ? static_cast<double>(c->i) : c->f;
		return notifier.createFloat(std::max({valA, valB, valC}));
	}
	return nullptr;
}

inline AObject *clamp(NativeFuncInData) {
	auto val = args[0];
	auto minVal = args[1];
	auto maxVal = args[2];
	if (val->type == DefaultClass::intClassId && minVal->type == DefaultClass::intClassId && maxVal->type == DefaultClass::intClassId) {
		return notifier.createInt(std::clamp(val->i, minVal->i, maxVal->i));
	}
	double v = (val->type == DefaultClass::intClassId) ? static_cast<double>(val->i) : val->f;
	double mn = (minVal->type == DefaultClass::intClassId) ? static_cast<double>(minVal->i) : minVal->f;
	double mx = (maxVal->type == DefaultClass::intClassId) ? static_cast<double>(maxVal->i) : maxVal->f;
	return notifier.createFloat(std::clamp(v, mn, mx));
}

inline AObject *require_not_null(NativeFuncInData) {
	if (args[0]->type == DefaultClass::nullClassId) {
		if (argSize > 1 && args[1] && args[1]->type == DefaultClass::stringClassId && args[1]->str) {
			notifier.throwException(std::string(args[1]->str->data));
		} else {
			notifier.throwException("Required value was null.");
		}
		return nullptr;
	}
	return args[0];
}

inline AObject *check_not_null(NativeFuncInData) {
	if (args[0]->type == DefaultClass::nullClassId) {
		if (argSize > 1 && args[1] && args[1]->type == DefaultClass::stringClassId && args[1]->str) {
			notifier.throwException(std::string(args[1]->str->data));
		} else {
			notifier.throwException("Required value was null.");
		}
		return nullptr;
	}
	return args[0];
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
