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

inline bool char_equals_ignore_case(char a, char b) {
	return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
}

inline bool str_equals_ignore_case(std::string_view a, std::string_view b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i) {
		if (!char_equals_ignore_case(a[i], b[i])) return false;
	}
	return true;
}

inline size_t str_find_case(std::string_view full, std::string_view target, size_t pos = 0, bool ignoreCase = false) {
	if (target.empty()) return pos <= full.size() ? pos : std::string_view::npos;
	if (target.size() > full.size() || pos > full.size() - target.size()) return std::string_view::npos;
	if (!ignoreCase) return full.find(target, pos);
	for (size_t i = pos; i <= full.size() - target.size(); ++i) {
		bool match = true;
		for (size_t j = 0; j < target.size(); ++j) {
			if (!char_equals_ignore_case(full[i + j], target[j])) {
				match = false;
				break;
			}
		}
		if (match) return i;
	}
	return std::string_view::npos;
}

inline size_t str_rfind_case(std::string_view full, std::string_view target, size_t pos = std::string_view::npos, bool ignoreCase = false) {
	if (target.empty()) {
		return pos == std::string_view::npos ? full.size() : std::min(pos, full.size());
	}
	if (target.size() > full.size()) return std::string_view::npos;
	size_t start = (pos == std::string_view::npos || pos > full.size() - target.size()) ? (full.size() - target.size()) : pos;
	if (!ignoreCase) return full.rfind(target, start);
	for (size_t i = start + 1; i > 0; --i) {
		size_t idx = i - 1;
		bool match = true;
		for (size_t j = 0; j < target.size(); ++j) {
			if (!char_equals_ignore_case(full[idx + j], target[j])) {
				match = false;
				break;
			}
		}
		if (match) return idx;
	}
	return std::string_view::npos;
}

inline std::string_view str_arg_view(AObject *obj, std::string &buf) {
	if (obj->type == DefaultClass::intClassId) {
		buf = std::string(1, static_cast<char>(obj->i));
		return std::string_view(buf);
	}
	return std::string_view(obj->str->data, obj->str->size);
}

inline AObject *str_starts_with(NativeFuncInData) {
	std::string_view str(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view prefix = str_arg_view(args[1], buf);
	int64_t startIndex = 0;
	bool ignoreCase = false;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::intClassId) {
			startIndex = args[2]->i;
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	if (startIndex < 0 || static_cast<size_t>(startIndex) + prefix.size() > str.size()) {
		return DefaultClass::falseObject;
	}
	std::string_view slice = str.substr(static_cast<size_t>(startIndex), prefix.size());
	if (ignoreCase) {
		return notifier.createBool(str_equals_ignore_case(slice, prefix));
	}
	return notifier.createBool(std::memcmp(slice.data(), prefix.data(), prefix.size()) == 0);
}

inline AObject *str_ends_with(NativeFuncInData) {
	std::string_view str(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view suffix = str_arg_view(args[1], buf);
	bool ignoreCase = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;
	if (suffix.size() > str.size())
		return DefaultClass::falseObject;
	std::string_view slice = str.substr(str.size() - suffix.size());
	if (ignoreCase) {
		return notifier.createBool(str_equals_ignore_case(slice, suffix));
	}
	return notifier.createBool(std::memcmp(slice.data(), suffix.data(), suffix.size()) == 0);
}

inline AObject *str_last_index_of(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	size_t startPos = std::string_view::npos;
	bool ignoreCase = false;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::intClassId) {
			int64_t s = args[2]->i;
			if (s >= 0) startPos = static_cast<size_t>(s);
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	auto pos = str_rfind_case(full, target, startPos, ignoreCase);
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
	std::string bufOld, bufNew;
	std::string_view oldStr = str_arg_view(args[1], bufOld);
	std::string_view newStr = str_arg_view(args[2], bufNew);
	bool ignoreCase = (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) ? args[3]->b : false;

	if (oldStr.empty())
		return notifier.createString(AString::copy(args[0]->str));

	size_t count = 0;
	size_t pos = 0;
	while ((pos = str_find_case(full, oldStr, pos, ignoreCase)) != std::string_view::npos) {
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
	while ((pos = str_find_case(full, oldStr, readPos, ignoreCase)) != std::string_view::npos) {
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
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	bool ignoreCase = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	size_t pos = str_find_case(full, target, 0, ignoreCase);
	return pos != std::string_view::npos ? DefaultClass::trueObject : DefaultClass::falseObject;
}

inline AObject *str_index_of(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	int64_t startIndex = 0;
	bool ignoreCase = false;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::intClassId) {
			startIndex = args[2]->i;
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	if (startIndex < 0) startIndex = 0;

	size_t pos = str_find_case(full, target, static_cast<size_t>(startIndex), ignoreCase);
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
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string bufDelim;
	std::string_view delim = str_arg_view(args[1], bufDelim);
	int64_t limit = 0;
	bool ignoreCase = false;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::intClassId) {
			limit = args[2]->i;
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	ClassId classId = notifier.callFrame->func->returnId;

	AObject *arrayObj = notifier.createArray(classId);

	if (delim.empty()) {
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(AString::copy(args[0]->str)));
		return arrayObj;
	}

	size_t start = 0;
	size_t end = str_find_case(full, delim, 0, ignoreCase);
	int64_t count = 1;
	while (end != std::string_view::npos && (limit <= 0 || count < limit)) {
		std::string_view token = full.substr(start, end - start);
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(std::string(token)));
		start = end + delim.length();
		end = str_find_case(full, delim, start, ignoreCase);
		++count;
	}
	notifier.arrayAdd(arrayObj,
	                  notifier.createString(std::string(full.substr(start))));

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

inline AObject *str_is_not_empty(NativeFuncInData) {
	return notifier.createBool(args[0]->str->size != 0);
}

inline AObject *str_is_blank(NativeFuncInData) {
	AString *str = args[0]->str;
	for (size_t i = 0; i < str->size; ++i) {
		char c = str->data[i];
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
			return DefaultClass::falseObject;
		}
	}
	return DefaultClass::trueObject;
}

inline AObject *str_is_not_blank(NativeFuncInData) {
	AString *str = args[0]->str;
	for (size_t i = 0; i < str->size; ++i) {
		char c = str->data[i];
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
			return DefaultClass::trueObject;
		}
	}
	return DefaultClass::falseObject;
}

inline AObject *str_substring_range(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t len = static_cast<int64_t>(str->size);
	int64_t start = args[1]->i;
	int64_t end = args[2]->i;
	if (start < 0) start = 0;
	if (end > len) end = len;
	if (start >= end) return notifier.createString("");
	int64_t subLen = end - start;
	char *newStr = new char[subLen + 1];
	std::memcpy(newStr, str->data + start, subLen);
	newStr[subLen] = '\0';
	return notifier.createString(new AString(newStr, subLen));
}

inline AObject *str_lines(NativeFuncInData) {
	AString *str = args[0]->str;
	std::string_view sv(str->data, str->size);
	ClassId classId = notifier.callFrame->func->returnId;
	AObject *arrayObj = notifier.createArray(classId);

	size_t start = 0;
	for (size_t i = 0; i < sv.size(); ++i) {
		if (sv[i] == '\r') {
			std::string token(sv.substr(start, i - start));
			notifier.arrayAdd(arrayObj, notifier.createString(token));
			if (i + 1 < sv.size() && sv[i + 1] == '\n') {
				++i;
			}
			start = i + 1;
		} else if (sv[i] == '\n') {
			std::string token(sv.substr(start, i - start));
			notifier.arrayAdd(arrayObj, notifier.createString(token));
			start = i + 1;
		}
	}
	std::string token(sv.substr(start));
	notifier.arrayAdd(arrayObj, notifier.createString(token));
	return arrayObj;
}

inline AObject *str_pad_start(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t targetLen = args[1]->i;
	std::string_view padChar = (argSize >= 3 && args[2] && args[2]->type == DefaultClass::stringClassId) 
		? std::string_view(args[2]->str->data, args[2]->str->size) : " ";
	if (padChar.empty()) padChar = " ";
	if (static_cast<int64_t>(str->size) >= targetLen) {
		return notifier.createString(AString::copy(str));
	}
	size_t padLen = targetLen - str->size;
	std::string padding;
	padding.reserve(padLen);
	while (padding.size() < padLen) {
		padding.append(padChar);
	}
	if (padding.size() > padLen) {
		padding.resize(padLen);
	}
	std::string result = padding + std::string(str->data, str->size);
	return notifier.createString(result);
}

inline AObject *str_pad_end(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t targetLen = args[1]->i;
	std::string_view padChar = (argSize >= 3 && args[2] && args[2]->type == DefaultClass::stringClassId) 
		? std::string_view(args[2]->str->data, args[2]->str->size) : " ";
	if (padChar.empty()) padChar = " ";
	if (static_cast<int64_t>(str->size) >= targetLen) {
		return notifier.createString(AString::copy(str));
	}
	size_t padLen = targetLen - str->size;
	std::string padding;
	padding.reserve(padLen);
	while (padding.size() < padLen) {
		padding.append(padChar);
	}
	if (padding.size() > padLen) {
		padding.resize(padLen);
	}
	std::string result = std::string(str->data, str->size) + padding;
	return notifier.createString(result);
}

inline AObject *str_to_int_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return DefaultClass::nullObject;
	char *end = nullptr;
	int64_t val = std::strtoll(str->data, &end, 10);
	if (end == str->data || *end != '\0') {
		return DefaultClass::nullObject;
	}
	return notifier.createInt(val);
}

inline AObject *str_to_float_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return DefaultClass::nullObject;
	char *end = nullptr;
	double val = std::strtod(str->data, &end);
	if (end == str->data || *end != '\0') {
		return DefaultClass::nullObject;
	}
	return notifier.createFloat(val);
}

inline AObject *str_take(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t n = args[1]->i;
	if (n <= 0) return notifier.createString("");
	if (n >= static_cast<int64_t>(str->size)) return notifier.createString(AString::copy(str));
	return notifier.createString(std::string(str->data, static_cast<size_t>(n)));
}

inline AObject *str_take_last(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t n = args[1]->i;
	if (n <= 0) return notifier.createString("");
	int64_t len = static_cast<int64_t>(str->size);
	if (n >= len) return notifier.createString(AString::copy(str));
	return notifier.createString(std::string(str->data + (len - n), static_cast<size_t>(n)));
}

inline AObject *str_drop(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t n = args[1]->i;
	if (n <= 0) return notifier.createString(AString::copy(str));
	int64_t len = static_cast<int64_t>(str->size);
	if (n >= len) return notifier.createString("");
	return notifier.createString(std::string(str->data + n, static_cast<size_t>(len - n)));
}

inline AObject *str_drop_last(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t n = args[1]->i;
	if (n <= 0) return notifier.createString(AString::copy(str));
	int64_t len = static_cast<int64_t>(str->size);
	if (n >= len) return notifier.createString("");
	return notifier.createString(std::string(str->data, static_cast<size_t>(len - n)));
}

inline AObject *str_remove_prefix(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *prefix = args[1]->str;
	if (prefix->size <= str->size && std::memcmp(str->data, prefix->data, prefix->size) == 0) {
		return notifier.createString(std::string(str->data + prefix->size, str->size - prefix->size));
	}
	return notifier.createString(AString::copy(str));
}

inline AObject *str_remove_suffix(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *suffix = args[1]->str;
	if (suffix->size <= str->size && std::memcmp(str->data + str->size - suffix->size, suffix->data, suffix->size) == 0) {
		return notifier.createString(std::string(str->data, str->size - suffix->size));
	}
	return notifier.createString(AString::copy(str));
}

inline AObject *str_reversed(NativeFuncInData) {
	AString *str = args[0]->str;
	size_t len = str->size;
	char *newStr = new char[len + 1];
	for (size_t i = 0; i < len; ++i) {
		newStr[i] = str->data[len - 1 - i];
	}
	newStr[len] = '\0';
	return notifier.createString(new AString(newStr, len));
}

inline AObject *str_replace_first(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string_view oldStr(args[1]->str->data, args[1]->str->size);
	std::string_view newStr(args[2]->str->data, args[2]->str->size);
	bool ignoreCase = (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) ? args[3]->b : false;
	if (oldStr.empty()) return notifier.createString(AString::copy(args[0]->str));
	auto pos = str_find_case(full, oldStr, 0, ignoreCase);
	if (pos == std::string_view::npos) {
		return notifier.createString(AString::copy(args[0]->str));
	}
	std::string res;
	res.reserve(full.size() - oldStr.size() + newStr.size());
	res.append(full.substr(0, pos));
	res.append(newStr);
	res.append(full.substr(pos + oldStr.size()));
	return notifier.createString(res);
}

inline AObject *str_substring_before(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view delim = str_arg_view(args[1], buf);
	bool ignoreCase = false;
	AObject *missingVal = nullptr;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::stringClassId) {
			missingVal = args[2];
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	auto pos = str_find_case(full, delim, 0, ignoreCase);
	if (pos == std::string_view::npos) {
		if (missingVal) {
			return notifier.createString(AString::copy(missingVal->str));
		}
		return notifier.createString(AString::copy(args[0]->str));
	}
	return notifier.createString(std::string(full.substr(0, pos)));
}

inline AObject *str_substring_after(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view delim = str_arg_view(args[1], buf);
	bool ignoreCase = false;
	AObject *missingVal = nullptr;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::stringClassId) {
			missingVal = args[2];
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	auto pos = str_find_case(full, delim, 0, ignoreCase);
	if (pos == std::string_view::npos) {
		if (missingVal) {
			return notifier.createString(AString::copy(missingVal->str));
		}
		return notifier.createString(AString::copy(args[0]->str));
	}
	return notifier.createString(std::string(full.substr(pos + delim.size())));
}

inline AObject *str_substring_before_last(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view delim = str_arg_view(args[1], buf);
	bool ignoreCase = false;
	AObject *missingVal = nullptr;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::stringClassId) {
			missingVal = args[2];
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	auto pos = str_rfind_case(full, delim, std::string_view::npos, ignoreCase);
	if (pos == std::string_view::npos) {
		if (missingVal) {
			return notifier.createString(AString::copy(missingVal->str));
		}
		return notifier.createString(AString::copy(args[0]->str));
	}
	return notifier.createString(std::string(full.substr(0, pos)));
}

inline AObject *str_substring_after_last(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string buf;
	std::string_view delim = str_arg_view(args[1], buf);
	bool ignoreCase = false;
	AObject *missingVal = nullptr;
	if (argSize >= 3) {
		if (args[2]->type == DefaultClass::stringClassId) {
			missingVal = args[2];
			if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
				ignoreCase = args[3]->b;
			}
		} else if (args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
	}
	auto pos = str_rfind_case(full, delim, std::string_view::npos, ignoreCase);
	if (pos == std::string_view::npos) {
		if (missingVal) {
			return notifier.createString(AString::copy(missingVal->str));
		}
		return notifier.createString(AString::copy(args[0]->str));
	}
	return notifier.createString(std::string(full.substr(pos + delim.size())));
}

inline AObject *str_trim_start(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	size_t start = s.find_first_not_of(" \n\r\t");
	if (start == std::string::npos) return notifier.createString("");
	return notifier.createString(s.substr(start));
}

inline AObject *str_trim_end(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	size_t end = s.find_last_not_of(" \n\r\t");
	if (end == std::string::npos) return notifier.createString("");
	return notifier.createString(s.substr(0, end + 1));
}

inline AObject *str_trim_indent(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::vector<std::string_view> lines;
	size_t start = 0;
	for (size_t i = 0; i < full.size(); ++i) {
		if (full[i] == '\r') {
			lines.push_back(full.substr(start, i - start));
			if (i + 1 < full.size() && full[i + 1] == '\n') ++i;
			start = i + 1;
		} else if (full[i] == '\n') {
			lines.push_back(full.substr(start, i - start));
			start = i + 1;
		}
	}
	lines.push_back(full.substr(start));

	auto isBlank = [](std::string_view sv) {
		for (char c : sv) if (c != ' ' && c != '\t') return false;
		return true;
	};

	size_t minIndent = std::string_view::npos;
	for (size_t i = 0; i < lines.size(); ++i) {
		if (i == 0 && lines.size() > 1 && isBlank(lines[0])) continue;
		if (i + 1 == lines.size() && lines.size() > 1 && isBlank(lines.back())) continue;
		if (isBlank(lines[i])) continue;
		size_t indent = 0;
		while (indent < lines[i].size() && (lines[i][indent] == ' ' || lines[i][indent] == '\t')) {
			++indent;
		}
		if (indent < minIndent) minIndent = indent;
	}
	if (minIndent == std::string_view::npos) minIndent = 0;

	size_t lineStart = (lines.size() > 1 && isBlank(lines[0])) ? 1 : 0;
	size_t lineEnd = (lines.size() > 1 && isBlank(lines.back())) ? lines.size() - 1 : lines.size();

	std::string result;
	for (size_t i = lineStart; i < lineEnd; ++i) {
		if (i > lineStart) result += '\n';
		if (isBlank(lines[i])) continue;
		size_t drop = std::min(minIndent, lines[i].size());
		result.append(lines[i].substr(drop));
	}
	return notifier.createString(result);
}

inline AObject *str_trim_margin(NativeFuncInData) {
	std::string_view full(args[0]->str->data, args[0]->str->size);
	std::string_view prefix = (argSize >= 2 && args[1]->type == DefaultClass::stringClassId)
	    ? std::string_view(args[1]->str->data, args[1]->str->size) : "|";
	if (prefix.empty()) prefix = "|";

	std::vector<std::string_view> lines;
	size_t start = 0;
	for (size_t i = 0; i < full.size(); ++i) {
		if (full[i] == '\r') {
			lines.push_back(full.substr(start, i - start));
			if (i + 1 < full.size() && full[i + 1] == '\n') ++i;
			start = i + 1;
		} else if (full[i] == '\n') {
			lines.push_back(full.substr(start, i - start));
			start = i + 1;
		}
	}
	lines.push_back(full.substr(start));

	auto isBlank = [](std::string_view sv) {
		for (char c : sv) if (c != ' ' && c != '\t') return false;
		return true;
	};

	size_t lineStart = (lines.size() > 1 && isBlank(lines[0])) ? 1 : 0;
	size_t lineEnd = (lines.size() > 1 && isBlank(lines.back())) ? lines.size() - 1 : lines.size();

	std::string result;
	for (size_t i = lineStart; i < lineEnd; ++i) {
		if (i > lineStart) result += '\n';
		auto line = lines[i];
		size_t pos = 0;
		while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
		if (pos + prefix.size() <= line.size() && line.substr(pos, prefix.size()) == prefix) {
			result.append(line.substr(pos + prefix.size()));
		} else {
			result.append(line);
		}
	}
	return notifier.createString(result);
}

inline AObject *str_replace_first_char(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createString(AString::copy(str));
	AObject *firstChar = notifier.createString(std::string(1, str->data[0]));
	firstChar->retain();
	AObject *res = notifier.callFunctionObject(args[1], firstChar);
	notifier.release(firstChar);
	if (notifier.hasException()) return nullptr;
	std::string newPrefix = DefaultFunction::to_string(notifier, res);
	std::string remainder(str->data + 1, str->size - 1);
	return notifier.createString(newPrefix + remainder);
}

inline AObject *str_take_while(NativeFuncInData) {
	AString *str = args[0]->str;
	size_t count = 0;
	for (size_t i = 0; i < str->size; ++i) {
		AObject *ch = notifier.createString(std::string(1, str->data[i]));
		ch->retain();
		AObject *res = notifier.callFunctionObject(args[1], ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (!res || res->type != DefaultClass::boolClassId || !res->b) break;
		++count;
	}
	return notifier.createString(std::string(str->data, count));
}

inline AObject *str_drop_while(NativeFuncInData) {
	AString *str = args[0]->str;
	size_t start = 0;
	for (size_t i = 0; i < str->size; ++i) {
		AObject *ch = notifier.createString(std::string(1, str->data[i]));
		ch->retain();
		AObject *res = notifier.callFunctionObject(args[1], ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (!res || res->type != DefaultClass::boolClassId || !res->b) {
			start = i;
			break;
		}
		if (i + 1 == str->size) start = str->size;
	}
	return notifier.createString(std::string(str->data + start, str->size - start));
}

inline AObject *str_filter(NativeFuncInData) {
	AString *str = args[0]->str;
	std::string result;
	result.reserve(str->size);
	for (size_t i = 0; i < str->size; ++i) {
		AObject *ch = notifier.createString(std::string(1, str->data[i]));
		ch->retain();
		AObject *res = notifier.callFunctionObject(args[1], ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res && res->type == DefaultClass::boolClassId && res->b) {
			result.push_back(str->data[i]);
		}
	}
	return notifier.createString(result);
}

inline AObject *str_filter_not(NativeFuncInData) {
	AString *str = args[0]->str;
	std::string result;
	result.reserve(str->size);
	for (size_t i = 0; i < str->size; ++i) {
		AObject *ch = notifier.createString(std::string(1, str->data[i]));
		ch->retain();
		AObject *res = notifier.callFunctionObject(args[1], ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res && res->type == DefaultClass::boolClassId && !res->b) {
			result.push_back(str->data[i]);
		}
	}
	return notifier.createString(result);
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
