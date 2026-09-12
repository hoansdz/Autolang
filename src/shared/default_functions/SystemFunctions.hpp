#ifndef SYSTEM_FUNCTIONS_HPP
#define SYSTEM_FUNCTIONS_HPP

#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/default_functions/ConversionFunctions.hpp"
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

} // namespace DefaultFunction
} // namespace Autolang

#endif
