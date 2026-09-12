#ifndef STRING_FUNCTIONS_HPP
#define STRING_FUNCTIONS_HPP

#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/default_functions/ConversionFunctions.hpp"
#include <cstring>
#include <string>
#include <string_view>

namespace Autolang {
namespace DefaultFunction {

inline AObject *str_trim(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	size_t start = s.find_first_not_of(" \n\r\t");
	if (start == std::string::npos) {
		return notifier.createString("");
	}
	size_t end = s.find_last_not_of(" \n\r\t");
	return notifier.createString(s.substr(start, end - start + 1));
}

inline AObject *string_constructor(NativeFuncInData) {
	switch (argSize) {
		case 0: {
			char *newStr = new char[1];
			newStr[0] = '\0';
			return notifier.createString(new AString(newStr, 0));
		}
		// To string
		case 1: {
			return to_string(notifier, args, argSize);
		}
		//"hi",3 => "hihihi"
		case 2: {
			int64_t count = args[1]->i;
			AString *oldAStr = args[0]->str;
			if (count <= 0 || oldAStr->size == 0) {
				char *newStr = new char[1];
				newStr[0] = '\0';
				return notifier.createString(new AString(newStr, 0));
			}
			size_t newSize = oldAStr->size * count;
			int64_t delta = static_cast<int64_t>(sizeof(AString) + newSize + 1);
			if (notifier.getCurrentManagedMemory() + delta >
			    notifier.getMaxManagedMemory()) {
				notifier.throwMemoryLimitExceeded(
				    notifier.getCurrentManagedMemory() + delta);
				return nullptr;
			}
			char *newStr = new char[newSize + 1];
			for (int i = 0; i < count; ++i) {
				memcpy(&newStr[i * oldAStr->size], oldAStr->data,
				       oldAStr->size);
			}
			newStr[newSize] = '\0';
			return notifier.createString(new AString(newStr, newSize));
		}
		default: {
			notifier.throwException("Cannot create String");
			return nullptr;
		}
	}
}

inline AObject *str_get(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t pos = args[1]->i;

	int64_t len = str->size;

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

inline AObject *str_starts_with(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *prefix = args[1]->str;
	if (prefix->size > str->size)
		return DefaultClass::falseObject;
	return notifier.createBool(
	    std::memcmp(str->data, prefix->data, prefix->size) == 0);
}

inline AObject *str_ends_with(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *suffix = args[1]->str;
	if (suffix->size > str->size)
		return DefaultClass::falseObject;
	return notifier.createBool(std::memcmp(str->data + str->size - suffix->size,
	                                       suffix->data, suffix->size) == 0);
}

inline AObject *str_last_index_of(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string_view target(args[1]->str->data, args[1]->str->size);
	auto pos = full.rfind(target);
	if (pos == std::string_view::npos)
		return notifier.createInt(-1);
	return notifier.createInt(static_cast<int64_t>(pos));
}

inline AObject *str_to_lower(NativeFuncInData) {
	AString *str = args[0]->str;
	char *newStr = new char[str->size + 1];
	for (size_t i = 0; i < str->size; ++i) {
		char c = str->data[i];
		newStr[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
	}
	newStr[str->size] = '\0';
	return notifier.createString(new AString(newStr, str->size));
}

inline AObject *str_to_upper(NativeFuncInData) {
	AString *str = args[0]->str;
	char *newStr = new char[str->size + 1];
	for (size_t i = 0; i < str->size; ++i) {
		char c = str->data[i];
		newStr[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
	}
	newStr[str->size] = '\0';
	return notifier.createString(new AString(newStr, str->size));
}

inline AObject *str_replace(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string_view oldStr(args[1]->str->data, args[1]->str->size);
	std::string_view newStr(args[2]->str->data, args[2]->str->size);

	if (oldStr.empty())
		return notifier.createString(AString::copy(args[0]->str));

	size_t count = 0;
	size_t pos = 0;
	while ((pos = full.find(oldStr, pos)) != std::string_view::npos) {
		++count;
		pos += oldStr.length();
	}

	if (count == 0)
		return notifier.createString(AString::copy(args[0]->str));

	size_t newSize =
	    full.length() + count * (newStr.length() - oldStr.length());
	char *resultStr = new char[newSize + 1];

	size_t writePos = 0;
	size_t readPos = 0;
	while ((pos = full.find(oldStr, readPos)) != std::string_view::npos) {
		size_t chunkLen = pos - readPos;
		std::memcpy(resultStr + writePos, full.data() + readPos, chunkLen);
		writePos += chunkLen;
		std::memcpy(resultStr + writePos, newStr.data(), newStr.length());
		writePos += newStr.length();
		readPos = pos + oldStr.length();
	}
	std::memcpy(resultStr + writePos, full.data() + readPos,
	            full.length() - readPos);
	resultStr[newSize] = '\0';

	return notifier.createString(new AString(resultStr, newSize));
}

inline AObject *str_char_at(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t pos = args[1]->i;

	int64_t len = str->size;

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

	return notifier.createInt(str->data[pos]);
}

inline AObject *str_contains(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *sub = args[1]->str;

	std::string_view full(str->data, str->size);
	std::string_view target(sub->data, sub->size);

	bool found = full.find(target) != std::string_view::npos;
	return found ? DefaultClass::trueObject : DefaultClass::falseObject;
}

inline AObject *str_index_of(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *sub = args[1]->str;

	std::string_view full(str->data, str->size);
	std::string_view target(sub->data, sub->size);

	auto pos = full.find(target);
	if (pos == std::string_view::npos) {
		return notifier.createInt(-1);
	}
	return notifier.createInt(static_cast<int64_t>(pos));
}

inline AObject *str_is_empty(NativeFuncInData) {
	AString *str = args[0]->str;
	return notifier.createBool(str->size == 0);
}

inline AObject *str_split(NativeFuncInData) {
	std::string full(args[0]->str->data, args[0]->str->size);
	std::string delim(args[1]->str->data, args[1]->str->size);
	ClassId classId = notifier.callFrame->func->returnId;

	AObject *arrayObj = notifier.createArray(classId);

	if (delim.empty()) {
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(AString::copy(args[0]->str)));
		return arrayObj;
	}

	size_t start = 0;
	size_t end = full.find(delim);
	while (end != std::string::npos) {
		std::string token = full.substr(start, end - start);
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(AString::from(token)));
		start = end + delim.length();
		end = full.find(delim, start);
	}
	notifier.arrayAdd(arrayObj,
	                  notifier.createString(AString::from(full.substr(start))));

	return arrayObj;
}

inline AObject *str_substr(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t len = str->size;

	if (argSize < 2 || argSize > 3) {
		notifier.throwException("substr expects 1 or 2 arguments");
		return nullptr;
	}

	int64_t from = args[1]->i;

	if (from < 0)
		from += len;

	if (from < 0 || from > len) {
		notifier.throwException("Index out of range");
		return nullptr;
	}

	// substr(from)
	if (argSize == 2) {
		int64_t newLen = len - from;

		char *newStr = new char[newLen + 1];
		memcpy(newStr, str->data + from, newLen);
		newStr[newLen] = '\0';

		return notifier.createString(new AString(newStr, newLen));
	}

	// substr(from, length)
	int64_t length = args[2]->i;

	if (length < 0) {
		notifier.throwException("Length cannot be negative");
		return nullptr;
	}

	if (from + length > len)
		length = len - from;

	char *newStr = new char[length + 1];
	memcpy(newStr, str->data + from, length);
	newStr[length] = '\0';

	return notifier.createString(new AString(newStr, length));
}

inline AObject *get_string_size(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->str->size));
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
