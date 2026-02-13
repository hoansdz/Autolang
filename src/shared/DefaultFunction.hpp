#ifndef DEFAULT_FUNCTION_HPP
#define DEFAULT_FUNCTION_HPP

#include "backend/vm/ANotifier.hpp"
#include "frontend/libs/Math.hpp"
#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include <cstdlib>
#include <iostream>

namespace AutoLang {
class ACompiler;
namespace DefaultFunction {

void init(ACompiler &compiler);
inline AObject *data_constructor(NativeFuncInData);
inline AObject *print(NativeFuncInData);
inline AObject *println(NativeFuncInData);
inline AObject *get_refcount(NativeFuncInData);
inline AObject *string_constructor(NativeFuncInData);
inline AObject *to_int(NativeFuncInData);
inline AObject *to_float(NativeFuncInData);
inline AObject *to_string(NativeFuncInData);
inline AObject *get_string_size(NativeFuncInData);
inline AObject *input_str(NativeFuncInData);
inline AObject *str_get(NativeFuncInData);

AObject *data_constructor(NativeFuncInData) {
	AObject *obj = args[0];
	for (size_t i = 1; i < size; ++i) {
		AObject **last = &obj->member->data[i - 1];
		*last = args[i];
		(*last)->retain();
	}
	return nullptr;
}

AObject *print(NativeFuncInData) {
	AObject *obj = args[0];
	uint32_t type = obj->type;
	switch (type) {
		case AutoLang::DefaultClass::intClassId:
			std::cout << obj->i;
			break;
		case AutoLang::DefaultClass::floatClassId:
			std::cout << obj->f;
			break;
		case AutoLang::DefaultClass::stringClassId:
			std::cout << obj->str->data;
			break;
		case AutoLang::DefaultClass::nullClassId:
			std::cout << "null";
			break;
		case DefaultClass::boolClassId:
			std::cout << (obj == DefaultClass::trueObject ? "true" : "false");
			break;
		default:
			std::cout << (uintptr_t)(obj);
			break;
	}
	return nullptr;
}

AObject *println(NativeFuncInData) {
	print(notifier, args, size);
	std::cout << '\n';
	return nullptr;
}

AObject *get_refcount(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->refCount - 1));
}

AObject *to_int(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(obj->i);
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createInt(static_cast<int64_t>(obj->f));
		case AutoLang::DefaultClass::boolClassId:
			return notifier.createInt(static_cast<int64_t>(obj->b));
		case AutoLang::DefaultClass::stringClassId: {
			char *end;
			return notifier.createInt(std::strtoll(obj->str->data, &end, 10));
		}
		default:
			break;
	}
	notifier.throwException("Cannot cast " +
	                        notifier.vm->data.classes[obj->type]->name +
	                        " to Int");
	return nullptr;
}

AObject *to_float(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createFloat(static_cast<double>(obj->i));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createFloat(obj->f);
		case AutoLang::DefaultClass::boolClassId:
			return notifier.createFloat(static_cast<double>(obj->b));
		case AutoLang::DefaultClass::stringClassId: {
			// Check error but unused
			char *end;
			return notifier.createFloat(std::strtod(obj->str->data, &end));
		}
		default:
			break;
	}
	notifier.throwException("Cannot cast " +
	                        notifier.vm->data.classes[obj->type]->name +
	                        " to Float");
	return nullptr;
}

AObject *to_string(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createString(AString::from(obj->i));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createString(AString::from(obj->f));
		case AutoLang::DefaultClass::stringClassId:
			return notifier.createString(AString::copy(obj->str));
		default:
			break;
	}
	notifier.throwException("Cannot cast " +
	                        notifier.vm->data.classes[obj->type]->name +
	                        " to String");
	return nullptr;
}

AObject *string_constructor(NativeFuncInData) {
	// To string
	if (size == 1) {
		return to_string(notifier, args, size);
	}
	//"hi",3 => "hihihi"
	if (size == 2) {
		int64_t count = args[1]->i;
		AString *oldAStr = args[0]->str;
		if (count <= 0 || oldAStr->size == 0) {
			char *newStr = new char[1];
			newStr[0] = '\0';
			return notifier.createString(new AString(newStr, 0));
		}
		size_t newSize = oldAStr->size * count;
		char *newStr = new char[newSize + 1];
		for (int i = 0; i < count; ++i) {
			memcpy(&newStr[i * oldAStr->size], oldAStr->data, oldAStr->size);
		}
		newStr[newSize] = '\0';
		return notifier.createString(new AString(newStr, newSize));
	}
	notifier.throwException("Cannot cast to String");
	return nullptr;
}

AObject *str_get(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t pos = args[1]->i;

	auto len = str->size;

	if (len == 0) {
		notifier.throwException("Empty string");
		return nullptr;
	}

	if (pos < 0)
		pos += len;

	if (pos < 0 || pos >= len) {
		notifier.throwException("Index out of range");
		return nullptr;
	}

	return notifier.createString(AString::from(str->data[pos]));
}

AObject *get_string_size(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->str->size));
}

AObject *input_str(NativeFuncInData) {
#ifdef __EMSCRIPTEN__

#else
	std::string s;
	std::getline(std::cin, s);
	notifier.input(notifier.createString(AString::from(s)));
#endif
	return nullptr;
}

} // namespace DefaultFunction
} // namespace AutoLang

#endif