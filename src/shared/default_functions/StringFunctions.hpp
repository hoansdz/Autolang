#ifndef STRING_FUNCTIONS_HPP
#define STRING_FUNCTIONS_HPP

#include "backend/vm/ANotifier.hpp"
#include "shared/AString.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/default_functions/ConversionFunctions.hpp"
#include <algorithm>
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

	return notifier.createChar(static_cast<uint8_t>(str->data[pos]));
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
	if (obj->type == DefaultClass::charClassId) {
		buf = AString::codePointToUtf8(static_cast<uint32_t>(obj->chr));
		return std::string_view(buf);
	}
	if (obj->type == DefaultClass::intClassId) {
		buf = AString::codePointToUtf8(static_cast<uint32_t>(obj->i));
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
		switch (args[2]->type) {
			case DefaultClass::intClassId:
				startIndex = args[2]->i;
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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
		switch (args[2]->type) {
			case DefaultClass::intClassId: {
				int64_t s = args[2]->i;
				if (s >= 0) startPos = static_cast<size_t>(s);
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			}
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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

	return notifier.createChar(static_cast<uint8_t>(str->data[pos]));
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
		switch (args[2]->type) {
			case DefaultClass::intClassId:
				startIndex = args[2]->i;
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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
	ClassId classId = notifier.callFrame->func->returnId;
	AObject *arrayObj = notifier.createArray(classId, DefaultClass::stringClassId);

	if (argSize < 2) {
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(AString::copy(args[0]->str)));
		return arrayObj;
	}

	std::vector<std::string> delims;
	int64_t limit = 0;
	bool ignoreCase = false;

	if (args[1]->flags & AObject::Flags::OBJ_IS_ARRAY) {
		auto arr = args[1]->array;
		switch (arr->key) {
			case DefaultClass::intClassId:
				for (size_t i = 0; i < arr->size; ++i) {
					delims.push_back(AString::codePointToUtf8(static_cast<uint32_t>(arr->intData[i])));
				}
				break;
			case DefaultClass::floatClassId:
				for (size_t i = 0; i < arr->size; ++i) {
					delims.push_back(AString::codePointToUtf8(static_cast<uint32_t>(arr->floatData[i])));
				}
				break;
			default:
				if (arr->objData) {
					for (size_t i = 0; i < arr->size; ++i) {
						if (arr->objData[i]) {
							std::string buf;
							delims.emplace_back(str_arg_view(arr->objData[i], buf));
						}
					}
				}
				break;
		}
		if (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) {
			ignoreCase = args[2]->b;
		}
		if (argSize >= 4 && args[3]->type == DefaultClass::intClassId) {
			limit = args[3]->i;
		}
	} else if (argSize == 2) {
		std::string buf;
		delims.emplace_back(str_arg_view(args[1], buf));
	} else if (argSize >= 3 && (args[2]->type == DefaultClass::intClassId || args[2]->type == DefaultClass::boolClassId)) {
		std::string buf;
		delims.emplace_back(str_arg_view(args[1], buf));
		switch (args[2]->type) {
			case DefaultClass::intClassId:
				limit = args[2]->i;
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				if (argSize >= 4 && args[3]->type == DefaultClass::intClassId) {
					limit = args[3]->i;
				}
				break;
			default:
				break;
		}
	} else {
		for (size_t i = 1; i < argSize; ++i) {
			std::string buf;
			delims.emplace_back(str_arg_view(args[i], buf));
		}
	}

	if (delims.empty() || (delims.size() == 1 && delims[0].empty())) {
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(AString::copy(args[0]->str)));
		return arrayObj;
	}

	if (delims.size() == 1) {
		std::string_view delim = delims[0];
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

	size_t start = 0;
	int64_t count = 1;
	while (start < full.size() && (limit <= 0 || count < limit)) {
		size_t bestPos = std::string_view::npos;
		size_t bestLen = 0;
		for (const auto &d : delims) {
			if (d.empty()) continue;
			size_t pos = str_find_case(full, d, start, ignoreCase);
			if (pos != std::string_view::npos) {
				if (bestPos == std::string_view::npos || pos < bestPos || (pos == bestPos && d.size() > bestLen)) {
					bestPos = pos;
					bestLen = d.size();
				}
			}
		}
		if (bestPos == std::string_view::npos) break;
		std::string_view token = full.substr(start, bestPos - start);
		notifier.arrayAdd(arrayObj,
		                  notifier.createString(std::string(token)));
		start = bestPos + bestLen;
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

inline AObject *str_is_null_or_empty(NativeFuncInData) {
	if (argSize == 0 || !args[0] || args[0]->type == DefaultClass::nullClassId) {
		return DefaultClass::trueObject;
	}
	if (args[0]->type == DefaultClass::stringClassId && args[0]->str) {
		return args[0]->str->size == 0 ? DefaultClass::trueObject : DefaultClass::falseObject;
	}
	return DefaultClass::falseObject;
}

inline AObject *str_is_null_or_blank(NativeFuncInData) {
	if (argSize == 0 || !args[0] || args[0]->type == DefaultClass::nullClassId) {
		return DefaultClass::trueObject;
	}
	if (args[0]->type == DefaultClass::stringClassId && args[0]->str) {
		AString *str = args[0]->str;
		for (size_t i = 0; i < str->size; ++i) {
			char c = str->data[i];
			if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
				return DefaultClass::falseObject;
			}
		}
		return DefaultClass::trueObject;
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
	AObject *arrayObj = notifier.createArray(classId, DefaultClass::stringClassId);

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
	std::string buf;
	std::string_view padChar = (argSize >= 3 && args[2]) ? str_arg_view(args[2], buf) : " ";
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
	std::string buf;
	std::string_view padChar = (argSize >= 3 && args[2]) ? str_arg_view(args[2], buf) : " ";
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

inline AObject *str_to_bool(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 4 && (
	    (str->data[0] == 't' || str->data[0] == 'T') &&
	    (str->data[1] == 'r' || str->data[1] == 'R') &&
	    (str->data[2] == 'u' || str->data[2] == 'U') &&
	    (str->data[3] == 'e' || str->data[3] == 'E'))) {
		return DefaultClass::trueObject;
	}
	if (str->size == 5 && (
	    (str->data[0] == 'f' || str->data[0] == 'F') &&
	    (str->data[1] == 'a' || str->data[1] == 'A') &&
	    (str->data[2] == 'l' || str->data[2] == 'L') &&
	    (str->data[3] == 's' || str->data[3] == 'S') &&
	    (str->data[4] == 'e' || str->data[4] == 'E'))) {
		return DefaultClass::falseObject;
	}
	if (str->size == 1) {
		if (str->data[0] == '1') return DefaultClass::trueObject;
		if (str->data[0] == '0') return DefaultClass::falseObject;
	}
	notifier.throwException("Cannot parse '" + std::string(str->data, str->size) + "' to 'Bool'");
	return nullptr;
}

inline AObject *str_to_bool_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 4 && (
	    (str->data[0] == 't' || str->data[0] == 'T') &&
	    (str->data[1] == 'r' || str->data[1] == 'R') &&
	    (str->data[2] == 'u' || str->data[2] == 'U') &&
	    (str->data[3] == 'e' || str->data[3] == 'E'))) {
		return DefaultClass::trueObject;
	}
	if (str->size == 5 && (
	    (str->data[0] == 'f' || str->data[0] == 'F') &&
	    (str->data[1] == 'a' || str->data[1] == 'A') &&
	    (str->data[2] == 'l' || str->data[2] == 'L') &&
	    (str->data[3] == 's' || str->data[3] == 'S') &&
	    (str->data[4] == 'e' || str->data[4] == 'E'))) {
		return DefaultClass::falseObject;
	}
	if (str->size == 1) {
		if (str->data[0] == '1') return DefaultClass::trueObject;
		if (str->data[0] == '0') return DefaultClass::falseObject;
	}
	return DefaultClass::nullObject;
}

inline AObject *str_to_char(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) {
		notifier.throwException("Cannot convert empty String to 'Char'");
		return nullptr;
	}
	size_t idx = 0;
	AChar chr = AString::utf8ToCodePoint(std::string_view(str->data, str->size), idx);
	return notifier.createChar(chr);
}

inline AObject *str_to_char_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return DefaultClass::nullObject;
	size_t idx = 0;
	AChar chr = AString::utf8ToCodePoint(std::string_view(str->data, str->size), idx);
	return notifier.createChar(chr);
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
	std::string buf;
	std::string_view prefix = str_arg_view(args[1], buf);
	if (prefix.size() <= str->size && std::memcmp(str->data, prefix.data(), prefix.size()) == 0) {
		return notifier.createString(std::string(str->data + prefix.size(), str->size - prefix.size()));
	}
	return notifier.createString(AString::copy(str));
}

inline AObject *str_remove_suffix(NativeFuncInData) {
	AString *str = args[0]->str;
	std::string buf;
	std::string_view suffix = str_arg_view(args[1], buf);
	if (suffix.size() <= str->size && std::memcmp(str->data + str->size - suffix.size(), suffix.data(), suffix.size()) == 0) {
		return notifier.createString(std::string(str->data, str->size - suffix.size()));
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
		switch (args[2]->type) {
			case DefaultClass::stringClassId:
				missingVal = args[2];
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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
		switch (args[2]->type) {
			case DefaultClass::stringClassId:
				missingVal = args[2];
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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
		switch (args[2]->type) {
			case DefaultClass::stringClassId:
				missingVal = args[2];
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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
		switch (args[2]->type) {
			case DefaultClass::stringClassId:
				missingVal = args[2];
				if (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) {
					ignoreCase = args[3]->b;
				}
				break;
			case DefaultClass::boolClassId:
				ignoreCase = args[2]->b;
				break;
			default:
				break;
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

inline AObject *str_trim_end_char(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	std::string target = AString::codePointToUtf8(args[1]->chr);
	if (target.empty() || s.empty()) return args[0];
	size_t end = s.size();
	size_t tlen = target.size();
	while (end >= tlen && s.compare(end - tlen, tlen, target) == 0) {
		end -= tlen;
	}
	return notifier.createString(s.substr(0, end));
}

inline AObject *str_trim_start_char(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	std::string target = AString::codePointToUtf8(args[1]->chr);
	if (target.empty() || s.empty()) return args[0];
	size_t start = 0;
	size_t tlen = target.size();
	while (start + tlen <= s.size() && s.compare(start, tlen, target) == 0) {
		start += tlen;
	}
	return notifier.createString(s.substr(start));
}

inline AObject *str_trim_char(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	std::string target = AString::codePointToUtf8(args[1]->chr);
	if (target.empty() || s.empty()) return args[0];
	size_t start = 0;
	size_t tlen = target.size();
	while (start + tlen <= s.size() && s.compare(start, tlen, target) == 0) {
		start += tlen;
	}
	size_t end = s.size();
	while (end >= start + tlen && s.compare(end - tlen, tlen, target) == 0) {
		end -= tlen;
	}
	return notifier.createString(s.substr(start, end - start));
}

inline AObject *str_trim_end_str(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	const std::string &chars = args[1]->str->data;
	if (chars.empty() || s.empty()) return args[0];
	size_t end = s.find_last_not_of(chars);
	if (end == std::string::npos) return notifier.createString("");
	return notifier.createString(s.substr(0, end + 1));
}

inline AObject *str_trim_start_str(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	const std::string &chars = args[1]->str->data;
	if (chars.empty() || s.empty()) return args[0];
	size_t start = s.find_first_not_of(chars);
	if (start == std::string::npos) return notifier.createString("");
	return notifier.createString(s.substr(start));
}

inline AObject *str_trim_str(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	const std::string &chars = args[1]->str->data;
	if (chars.empty() || s.empty()) return args[0];
	size_t start = s.find_first_not_of(chars);
	if (start == std::string::npos) return notifier.createString("");
	size_t end = s.find_last_not_of(chars);
	return notifier.createString(s.substr(start, end - start + 1));
}

inline AObject *str_step(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	int64_t stp = args[1]->i;
	if (stp <= 0) {
		notifier.throwException("Step must be positive, was " + std::to_string(stp));
		return nullptr;
	}
	std::string res;
	for (size_t i = 0; i < s.size(); i += stp) {
		res += s[i];
	}
	return notifier.createString(res);
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
	AObject *firstChar = notifier.createChar(static_cast<uint8_t>(str->data[0]));
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
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
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
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
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
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
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
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
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

inline AObject *str_chunked(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t size = args[1]->i;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, DefaultClass::stringClassId);
	if (size <= 0) return newArr;
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; i += size) {
		int64_t subLen = (i + size > len) ? (len - i) : size;
		std::string sub(str->data + i, subLen);
		AObject *subStr = notifier.createString(sub);
		notifier.arrayAdd(newArr, subStr);
	}
	return newArr;
}

inline AObject *str_zip_with_next(NativeFuncInData) {
	AString *str = args[0]->str;
	ClassId pairClassId = (argSize >= 2 && args[1]->type == DefaultClass::intClassId)
	    ? static_cast<ClassId>(args[1]->i)
	    : DefaultClass::anyClassId;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, pairClassId);
	int64_t len = static_cast<int64_t>(str->size);
	if (len < 2) return newArr;
	for (int64_t i = 0; i < len - 1; ++i) {
		AObject *s1 = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		AObject *s2 = notifier.createChar(static_cast<uint8_t>(str->data[i + 1]));
		AObject *pairObj = notifier.createMemberObject(pairClassId, 2);
		s1->retain();
		pairObj->member->data[0] = s1;
		s2->retain();
		pairObj->member->data[1] = s2;
		notifier.arrayAdd(newArr, pairObj);
	}
	return newArr;
}

inline AObject *str_zip_with_next_transform(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *func = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, DefaultClass::anyClassId);
	int64_t len = static_cast<int64_t>(str->size);
	if (len < 2) return newArr;
	for (int64_t i = 0; i < len - 1; ++i) {
		AObject *s1 = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		AObject *s2 = notifier.createChar(static_cast<uint8_t>(str->data[i + 1]));
		s1->retain();
		s2->retain();
		AObject *res = notifier.callFunctionObject(func, s1, s2);
		notifier.release(s1);
		notifier.release(s2);
		if (notifier.hasException()) return nullptr;
		if (res) notifier.arrayAdd(newArr, res);
	}
	return newArr;
}

inline AObject *int_to_char_string(NativeFuncInData) {
	char c = static_cast<char>(args[0]->i);
	return notifier.createString(AString::from(c));
}

inline AObject *str_region_matches(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t thisOffset = args[1]->i;
	AString *other = args[2]->str;
	int64_t otherOffset = args[3]->i;
	int64_t length = args[4]->i;
	bool ignoreCase = (argSize >= 6 && args[5]->type == DefaultClass::boolClassId) ? args[5]->b : false;

	if (thisOffset < 0 || otherOffset < 0 || length < 0 ||
	    static_cast<size_t>(thisOffset + length) > str->size ||
	    static_cast<size_t>(otherOffset + length) > other->size) {
		return notifier.createBool(false);
	}

	if (!ignoreCase) {
		bool match = (std::memcmp(str->data + thisOffset, other->data + otherOffset, length) == 0);
		return notifier.createBool(match);
	} else {
		for (int64_t i = 0; i < length; ++i) {
			if (!char_equals_ignore_case(str->data[thisOffset + i], other->data[otherOffset + i])) {
				return notifier.createBool(false);
			}
		}
		return notifier.createBool(true);
	}
}

inline AObject *str_compare_to(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *other = args[1]->str;
	bool ignoreCase = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	size_t minLen = std::min(str->size, other->size);
	for (size_t i = 0; i < minLen; ++i) {
		unsigned char c1 = static_cast<unsigned char>(str->data[i]);
		unsigned char c2 = static_cast<unsigned char>(other->data[i]);
		if (ignoreCase) {
			c1 = std::tolower(c1);
			c2 = std::tolower(c2);
		}
		if (c1 != c2) {
			return notifier.createInt(c1 < c2 ? -1 : 1);
		}
	}
	if (str->size < other->size) return notifier.createInt(-1);
	if (str->size > other->size) return notifier.createInt(1);
	return notifier.createInt(0);
}

inline AObject *str_common_prefix_with(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *other = args[1]->str;
	bool ignoreCase = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	size_t minLen = std::min(str->size, other->size);
	size_t matchLen = 0;
	while (matchLen < minLen) {
		bool match = ignoreCase 
			? char_equals_ignore_case(str->data[matchLen], other->data[matchLen])
			: (str->data[matchLen] == other->data[matchLen]);
		if (!match) break;
		++matchLen;
	}
	return notifier.createString(std::string(str->data, matchLen));
}

inline AObject *str_common_suffix_with(NativeFuncInData) {
	AString *str = args[0]->str;
	AString *other = args[1]->str;
	bool ignoreCase = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	size_t minLen = std::min(str->size, other->size);
	size_t matchLen = 0;
	while (matchLen < minLen) {
		char c1 = str->data[str->size - 1 - matchLen];
		char c2 = other->data[other->size - 1 - matchLen];
		bool match = ignoreCase ? char_equals_ignore_case(c1, c2) : (c1 == c2);
		if (!match) break;
		++matchLen;
	}
	return notifier.createString(std::string(str->data + str->size - matchLen, matchLen));
}

inline AObject *str_map(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *func = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, DefaultClass::anyClassId);
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *s = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		s->retain();
		AObject *res = notifier.callFunctionObject(func, s);
		notifier.release(s);
		if (notifier.hasException()) return nullptr;
		if (res) notifier.arrayAdd(newArr, res);
	}
	return newArr;
}

inline AObject *str_map_indexed(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *func = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, DefaultClass::anyClassId);
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *idxObj = notifier.createInt(i);
		AObject *s = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		idxObj->retain();
		s->retain();
		AObject *res = notifier.callFunctionObject(func, idxObj, s);
		notifier.release(idxObj);
		notifier.release(s);
		if (notifier.hasException()) return nullptr;
		if (res) notifier.arrayAdd(newArr, res);
	}
	return newArr;
}

inline AObject *str_fold(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *accumulator = args[1];
	AObject *func = args[2];
	AObject *acc = accumulator;
	acc->retain();
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		AObject *nextAcc = notifier.callFunctionObject(func, acc, ch);
		notifier.release(ch);
		notifier.release(acc);
		if (notifier.hasException()) {
			return nullptr;
		}
		acc = nextAcc;
	}
	if (acc && !(acc->flags & AObject::Flags::OBJ_IS_CONST)) {
		--acc->refCount;
	}
	return acc;
}

inline AObject *str_reduce(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t len = static_cast<int64_t>(str->size);
	if (len == 0) {
		notifier.throwException("UnsupportedOperationException: Empty string cannot be reduced.");
		return nullptr;
	}
	AObject *func = args[1];
	AObject *acc = notifier.createString(std::string(1, str->data[0]));
	acc->retain();
	for (int64_t i = 1; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		AObject *nextAcc = notifier.callFunctionObject(func, acc, ch);
		notifier.release(ch);
		notifier.release(acc);
		if (notifier.hasException()) {
			return nullptr;
		}
		acc = nextAcc;
	}
	if (acc && !(acc->flags & AObject::Flags::OBJ_IS_CONST)) {
		--acc->refCount;
	}
	return acc;
}

inline AObject *str_running_fold(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *initial = args[1];
	AObject *func = args[2];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	ClassId elemClassId = DefaultClass::anyClassId;
	auto newArr = notifier.createArray(returnId, elemClassId);
	notifier.arrayAdd(newArr, initial);
	AObject *acc = initial;
	acc->retain();
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		AObject *nextAcc = notifier.callFunctionObject(func, acc, ch);
		notifier.release(acc);
		notifier.release(ch);
		if (notifier.hasException()) {
			return nullptr;
		}
		notifier.arrayAdd(newArr, nextAcc);
		acc = nextAcc;
		acc->retain();
	}
	notifier.release(acc);
	return newArr;
}

inline AObject *str_running_reduce(NativeFuncInData) {
	AString *str = args[0]->str;
	int64_t len = static_cast<int64_t>(str->size);
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	ClassId elemClassId = DefaultClass::stringClassId;
	auto newArr = notifier.createArray(returnId, elemClassId);
	if (len == 0) return newArr;
	AObject *func = args[1];
	AObject *first = notifier.createString(std::string(1, str->data[0]));
	notifier.arrayAdd(newArr, first);
	AObject *acc = first;
	acc->retain();
	for (int64_t i = 1; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		AObject *nextAcc = notifier.callFunctionObject(func, acc, ch);
		notifier.release(acc);
		notifier.release(ch);
		if (notifier.hasException()) {
			return nullptr;
		}
		notifier.arrayAdd(newArr, nextAcc);
		acc = nextAcc;
		acc->retain();
	}
	notifier.release(acc);
	return newArr;
}

inline AObject *str_first(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) {
		notifier.throwException("NoSuchElementException: Char sequence is empty.");
		return nullptr;
	}
	return notifier.createChar(static_cast<uint8_t>(str->data[0]));
}

inline AObject *str_last(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) {
		notifier.throwException("NoSuchElementException: Char sequence is empty.");
		return nullptr;
	}
	return notifier.createChar(static_cast<uint8_t>(str->data[str->size - 1]));
}

inline AObject *str_first_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) {
		return DefaultClass::nullObject;
	}
	return notifier.createChar(static_cast<uint8_t>(str->data[0]));
}

inline AObject *str_last_or_null(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) {
		return DefaultClass::nullObject;
	}
	return notifier.createChar(static_cast<uint8_t>(str->data[str->size - 1]));
}

inline AObject *str_last_index(NativeFuncInData) {
	return notifier.createInt(static_cast<int64_t>(args[0]->str->size) - 1);
}

inline AObject *str_is_digit(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	for (size_t i = 0; i < str->size; ++i) {
		unsigned char c = static_cast<unsigned char>(str->data[i]);
		if (c < '0' || c > '9') return notifier.createBool(false);
	}
	return notifier.createBool(true);
}

inline AObject *str_is_letter(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	for (size_t i = 0; i < str->size; ++i) {
		unsigned char c = static_cast<unsigned char>(str->data[i]);
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return notifier.createBool(false);
	}
	return notifier.createBool(true);
}

inline AObject *str_is_letter_or_digit(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	for (size_t i = 0; i < str->size; ++i) {
		unsigned char c = static_cast<unsigned char>(str->data[i]);
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return notifier.createBool(false);
	}
	return notifier.createBool(true);
}

inline AObject *str_is_whitespace(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	for (size_t i = 0; i < str->size; ++i) {
		unsigned char c = static_cast<unsigned char>(str->data[i]);
		if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r')) return notifier.createBool(false);
	}
	return notifier.createBool(true);
}

inline AObject *str_is_uppercase(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	unsigned char c = static_cast<unsigned char>(str->data[0]);
	return notifier.createBool(c >= 'A' && c <= 'Z');
}

inline AObject *str_is_lowercase(NativeFuncInData) {
	AString *str = args[0]->str;
	if (str->size == 0) return notifier.createBool(false);
	unsigned char c = static_cast<unsigned char>(str->data[0]);
	return notifier.createBool(c >= 'a' && c <= 'z');
}

inline AObject *str_content_equals(NativeFuncInData) {
	AString *s1 = args[0]->str;
	AString *s2 = args[1]->str;
	if (s1->size != s2->size) return notifier.createBool(false);
	return notifier.createBool(memcmp(s1->data, s2->data, s1->size) == 0);
}

inline AObject *str_remove_surrounding(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	const std::string &prefix = args[1]->str->data;
	const std::string &suffix = args[2]->str->data;
	if (s.size() >= prefix.size() + suffix.size() &&
	    s.rfind(prefix, 0) == 0 &&
	    s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
		return notifier.createString(s.substr(prefix.size(), s.size() - prefix.size() - suffix.size()));
	}
	return notifier.createString(AString::copy(args[0]->str));
}

inline AObject *str_remove_surrounding_single(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	const std::string &del = args[1]->str->data;
	if (s.size() >= del.size() * 2 &&
	    s.rfind(del, 0) == 0 &&
	    s.compare(s.size() - del.size(), del.size(), del) == 0) {
		return notifier.createString(s.substr(del.size(), s.size() - del.size() * 2));
	}
	return notifier.createString(AString::copy(args[0]->str));
}

inline AObject *str_remove_surrounding_char(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	std::string prefix = AString::codePointToUtf8(args[1]->chr);
	std::string suffix = AString::codePointToUtf8(args[2]->chr);
	if (s.size() >= prefix.size() + suffix.size() &&
	    s.rfind(prefix, 0) == 0 &&
	    s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
		return notifier.createString(s.substr(prefix.size(), s.size() - prefix.size() - suffix.size()));
	}
	return notifier.createString(AString::copy(args[0]->str));
}

inline AObject *str_remove_surrounding_char_single(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	std::string del = AString::codePointToUtf8(args[1]->chr);
	if (s.size() >= del.size() * 2 &&
	    s.rfind(del, 0) == 0 &&
	    s.compare(s.size() - del.size(), del.size(), del) == 0) {
		return notifier.createString(s.substr(del.size(), s.size() - del.size() * 2));
	}
	return notifier.createString(AString::copy(args[0]->str));
}

inline AObject *str_remove_range(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	int64_t start = args[1]->i;
	int64_t end = args[2]->i;
	if (start < 0 || end < start || static_cast<size_t>(end) > s.size()) {
		notifier.throwException("Index out of range");
		return nullptr;
	}
	std::string res = s.substr(0, start) + s.substr(end);
	return notifier.createString(res);
}

inline AObject *str_replace_range(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	int64_t start = args[1]->i;
	int64_t end = args[2]->i;
	const std::string &repl = args[3]->str->data;
	if (start < 0 || end < start || static_cast<size_t>(end) > s.size()) {
		notifier.throwException("Index out of range");
		return nullptr;
	}
	std::string res = s.substr(0, start) + repl + s.substr(end);
	return notifier.createString(res);
}

inline AObject *str_indices(NativeFuncInData) {
	AString *str = args[0]->str;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	auto arr = notifier.createArray(returnId, DefaultClass::intClassId);
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		notifier.arrayAdd(arr, notifier.createInt(i));
	}
	return arr;
}

inline AObject *str_any_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res == DefaultClass::trueObject) return DefaultClass::trueObject;
	}
	return DefaultClass::falseObject;
}

inline AObject *str_all_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res != DefaultClass::trueObject) return DefaultClass::falseObject;
	}
	return DefaultClass::trueObject;
}

inline AObject *str_none_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res == DefaultClass::trueObject) return DefaultClass::falseObject;
	}
	return DefaultClass::trueObject;
}

inline AObject *str_count_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	int64_t count = 0;
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res == DefaultClass::trueObject) ++count;
	}
	return notifier.createInt(count);
}

inline AObject *str_index_of_first(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res == DefaultClass::trueObject) return notifier.createInt(i);
	}
	return notifier.createInt(-1);
}

inline AObject *str_index_of_last(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (res == DefaultClass::trueObject) return notifier.createInt(i);
	}
	return notifier.createInt(-1);
}

inline AObject *str_first_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			if (res) notifier.release(res);
			return nullptr;
		}
		bool matched = (res == DefaultClass::trueObject);
		if (res) notifier.release(res);
		notifier.release(ch);
		if (matched) {
			return notifier.createChar(static_cast<uint8_t>(str->data[i]));
		}
	}
	notifier.throwException("NoSuchElementException: String contains no character matching the predicate.");
	return nullptr;
}

inline AObject *str_last_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			if (res) notifier.release(res);
			return nullptr;
		}
		bool matched = (res == DefaultClass::trueObject);
		if (res) notifier.release(res);
		notifier.release(ch);
		if (matched) {
			return notifier.createChar(static_cast<uint8_t>(str->data[i]));
		}
	}
	notifier.throwException("NoSuchElementException: String contains no character matching the predicate.");
	return nullptr;
}

inline AObject *str_first_or_null_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			if (res) notifier.release(res);
			return nullptr;
		}
		bool matched = (res == DefaultClass::trueObject);
		if (res) notifier.release(res);
		notifier.release(ch);
		if (matched) {
			return notifier.createChar(static_cast<uint8_t>(str->data[i]));
		}
	}
	return DefaultClass::nullObject;
}

inline AObject *str_last_or_null_fn(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto res = notifier.callFunctionObject(predicate, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			if (res) notifier.release(res);
			return nullptr;
		}
		bool matched = (res == DefaultClass::trueObject);
		if (res) notifier.release(res);
		notifier.release(ch);
		if (matched) {
			return notifier.createChar(static_cast<uint8_t>(str->data[i]));
		}
	}
	return DefaultClass::nullObject;
}

inline AObject *str_for_each(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *action = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		ch->retain();
		auto ret = notifier.callFunctionObject(action, ch);
		if (ret) notifier.release(ret);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
	}
	return nullptr;
}

inline AObject *str_for_each_indexed(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *action = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		AObject *idx = notifier.createInt(i);
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		idx->retain();
		ch->retain();
		auto ret = notifier.callFunctionObject(action, idx, ch);
		if (ret) notifier.release(ret);
		notifier.release(idx);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
	}
	return nullptr;
}

inline AObject *str_on_each(NativeFuncInData) {
	str_for_each(notifier, args, argSize);
	if (notifier.hasException()) return nullptr;
	return args[0];
}

inline AObject *str_on_each_indexed(NativeFuncInData) {
	str_for_each_indexed(notifier, args, argSize);
	if (notifier.hasException()) return nullptr;
	return args[0];
}

inline AObject *str_filter_indexed(NativeFuncInData) {
	AString *str = args[0]->str;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(str->size);
	std::string res;
	for (int64_t i = 0; i < len; ++i) {
		AObject *idx = notifier.createInt(i);
		AObject *ch = notifier.createChar(static_cast<uint8_t>(str->data[i]));
		idx->retain();
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, idx, ch);
		notifier.release(idx);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match == DefaultClass::trueObject) {
			res += str->data[i];
		}
	}
	return notifier.createString(res);
}

inline AObject *str_take_last_while(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) {
			return notifier.createString(s.substr(i + 1));
		}
	}
	return notifier.createString(AString::copy(args[0]->str));
}

inline AObject *str_drop_last_while(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) {
			return notifier.createString(s.substr(0, i + 1));
		}
	}
	return notifier.createString("");
}

inline AObject *str_windowed(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	int64_t size = args[1]->i;
	int64_t step = argSize > 2 ? args[2]->i : 1;
	bool partial = argSize > 3 ? (args[3] == DefaultClass::trueObject) : false;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	auto arr = notifier.createArray(returnId, DefaultClass::stringClassId);
	int64_t len = static_cast<int64_t>(s.size());
	if (size <= 0 || step <= 0) return arr;
	for (int64_t i = 0; i < len; i += step) {
		int64_t end = i + size;
		if (end <= len) {
			notifier.arrayAdd(arr, notifier.createString(s.substr(i, size)));
		} else if (partial) {
			notifier.arrayAdd(arr, notifier.createString(s.substr(i)));
		} else {
			break;
		}
	}
	return arr;
}

inline AObject *str_to_list(NativeFuncInData) {
	AString *str = args[0]->str;
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	auto arr = notifier.createArray(returnId, DefaultClass::charClassId);
	int64_t len = static_cast<int64_t>(str->size);
	for (int64_t i = 0; i < len; ++i) {
		notifier.arrayAdd(arr, notifier.createChar(static_cast<uint8_t>(str->data[i])));
	}
	return arr;
}

inline AObject *str_trim_start_fn(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) {
			return notifier.createString(s.substr(i));
		}
	}
	return notifier.createString("");
}

inline AObject *str_trim_end_fn(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = len - 1; i >= 0; --i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) {
			return notifier.createString(s.substr(0, i + 1));
		}
	}
	return notifier.createString("");
}

inline AObject *str_trim_fn(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	int64_t start = 0;
	while (start < len) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[start]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) break;
		++start;
	}
	if (start == len) return notifier.createString("");
	int64_t end = len - 1;
	while (end >= start) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[end]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match != DefaultClass::trueObject) break;
		--end;
	}
	return notifier.createString(s.substr(start, end - start + 1));
}

inline AObject *build_list(NativeFuncInData) {
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	ClassId elemKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			elemKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto list = notifier.createArray(returnId, elemKey);
	auto action = args[0];
	list->retain();
	auto ret = notifier.callFunctionObject(action, list);
	--list->refCount;
	if (ret) notifier.release(ret);
	return list;
}

inline AObject *build_map(NativeFuncInData) {
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::mapClassId;
	ClassId keyKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size >= 2) {
			keyKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto map = Libs::map::constructor(notifier, returnId, keyKey);
	map->flags |= AObject::Flags::OBJ_IS_MAP;
	auto action = args[0];
	map->retain();
	auto ret = notifier.callFunctionObject(action, map);
	--map->refCount;
	if (ret) notifier.release(ret);
	return map;
}

inline AObject *build_set(NativeFuncInData) {
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::setClassId;
	ClassId elemKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			elemKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto set = Libs::set::constructor(notifier, returnId, elemKey);
	set->flags |= AObject::Flags::OBJ_IS_SET;
	auto action = args[0];
	set->retain();
	auto ret = notifier.callFunctionObject(action, set);
	--set->refCount;
	if (ret) notifier.release(ret);
	return set;
}

inline AObject *string_builder_append(NativeFuncInData) {
	auto self = args[0];
	auto val = args[1];
	std::string addStr;
	if (val == nullptr || val == DefaultClass::nullObject) {
		addStr = "null";
	} else {
		addStr = to_string(notifier, val);
	}
	auto curContent = self->member->data[0];
	std::string newStr = std::string(curContent->str->data) + addStr;
	auto newContent = notifier.createString(newStr);
	newContent->retain();
	notifier.release(self->member->data[0]);
	self->member->data[0] = newContent;
	return self;
}

inline AObject *string_builder_append_line(NativeFuncInData) {
	auto self = args[0];
	std::string addStr;
	if (argSize > 1 && args[1] != nullptr && args[1] != DefaultClass::nullObject) {
		addStr = to_string(notifier, args[1]);
	}
	addStr += "\n";
	auto curContent = self->member->data[0];
	std::string newStr = std::string(curContent->str->data) + addStr;
	auto newContent = notifier.createString(newStr);
	newContent->retain();
	notifier.release(self->member->data[0]);
	self->member->data[0] = newContent;
	return self;
}

inline AObject *string_builder_length(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	return notifier.createInt(curContent->str->size);
}

inline AObject *string_builder_clear(NativeFuncInData) {
	auto self = args[0];
	auto emptyStr = notifier.createString("");
	emptyStr->retain();
	notifier.release(self->member->data[0]);
	self->member->data[0] = emptyStr;
	return self;
}

inline AObject *string_builder_to_string(NativeFuncInData) {
	auto self = args[0];
	return self->member->data[0];
}

inline AObject *string_builder_is_empty(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	return notifier.createBool(curContent->str->size == 0);
}

inline AObject *string_builder_is_not_empty(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	return notifier.createBool(curContent->str->size > 0);
}

inline void string_builder_set_content(ANotifier &notifier, AObject *self, const std::string &str) {
	auto newContent = notifier.createString(str);
	newContent->retain();
	notifier.release(self->member->data[0]);
	self->member->data[0] = newContent;
}

inline AObject *string_builder_append_range(NativeFuncInData) {
	auto self = args[0];
	std::string src = to_string(notifier, args[1]);
	int64_t start = args[2]->i;
	int64_t end = args[3]->i;
	int64_t srcLen = src.size();
	if (start < 0) start = 0;
	if (end > srcLen) end = srcLen;
	if (start < end) {
		auto curContent = self->member->data[0];
		std::string newStr = std::string(curContent->str->data, curContent->str->size) + src.substr(start, end - start);
		string_builder_set_content(notifier, self, newStr);
	}
	return self;
}

inline AObject *string_builder_get(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t pos = args[1]->i;
	int64_t len = curContent->str->size;
	if (pos < 0) pos += len;
	if (pos < 0 || pos >= len) {
		notifier.throwException("Index out of range: " + std::to_string(args[1]->i));
		return nullptr;
	}
	return notifier.createChar(static_cast<uint8_t>(curContent->str->data[pos]));
}

inline AObject *string_builder_set(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t pos = args[1]->i;
	int64_t len = curContent->str->size;
	if (pos < 0) pos += len;
	if (pos < 0 || pos >= len) {
		notifier.throwException("Index out of range: " + std::to_string(args[1]->i));
		return nullptr;
	}
	std::string s(curContent->str->data, curContent->str->size);
	switch (args[2]->type) {
		case DefaultClass::charClassId: {
			std::string chStr = AString::codePointToUtf8(static_cast<uint32_t>(args[2]->chr));
			s.replace(pos, 1, chStr);
			break;
		}
		case DefaultClass::stringClassId:
			s.replace(pos, 1, std::string_view(args[2]->str->data, args[2]->str->size));
			break;
		default: {
			std::string chStr = to_string(notifier, args[2]);
			s.replace(pos, 1, chStr);
			break;
		}
	}
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_last_index(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	return notifier.createInt(static_cast<int64_t>(curContent->str->size) - 1);
}

inline AObject *string_builder_insert(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t pos = args[1]->i;
	int64_t len = curContent->str->size;
	if (pos < 0) pos += len;
	if (pos < 0 || pos > len) {
		notifier.throwException("Index out of range: " + std::to_string(args[1]->i));
		return nullptr;
	}
	std::string valStr;
	if (argSize > 2 && args[2] != nullptr && args[2] != DefaultClass::nullObject) {
		valStr = to_string(notifier, args[2]);
	}
	std::string s(curContent->str->data, curContent->str->size);
	s.insert(pos, valStr);
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_delete(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t start = args[1]->i;
	int64_t end = args[2]->i;
	int64_t len = curContent->str->size;
	if (start < 0) start = 0;
	if (end > len) end = len;
	if (start <= len && start < end) {
		std::string s(curContent->str->data, curContent->str->size);
		s.erase(start, end - start);
		string_builder_set_content(notifier, self, s);
	}
	return self;
}

inline AObject *string_builder_delete_at(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t pos = args[1]->i;
	int64_t len = curContent->str->size;
	if (pos < 0) pos += len;
	if (pos < 0 || pos >= len) {
		notifier.throwException("Index out of range: " + std::to_string(args[1]->i));
		return nullptr;
	}
	std::string s(curContent->str->data, curContent->str->size);
	s.erase(pos, 1);
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_replace(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t start = args[1]->i;
	int64_t end = args[2]->i;
	int64_t len = curContent->str->size;
	if (start < 0) start = 0;
	if (end > len) end = len;
	if (start > len || start > end) {
		notifier.throwException("Index out of range");
		return nullptr;
	}
	std::string repStr = (argSize > 3 && args[3]) ? to_string(notifier, args[3]) : "";
	std::string s(curContent->str->data, curContent->str->size);
	s.replace(start, end - start, repStr);
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_reverse(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string s(curContent->str->data, curContent->str->size);
	std::reverse(s.begin(), s.end());
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_set_length(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t newLen = args[1]->i;
	if (newLen < 0) {
		notifier.throwException("Length cannot be negative");
		return nullptr;
	}
	std::string s(curContent->str->data, curContent->str->size);
	if (newLen < static_cast<int64_t>(s.size())) {
		s.resize(newLen);
	} else if (newLen > static_cast<int64_t>(s.size())) {
		s.resize(newLen, '\0');
	}
	string_builder_set_content(notifier, self, s);
	return self;
}

inline AObject *string_builder_substring(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	int64_t start = args[1]->i;
	int64_t len = curContent->str->size;
	if (start < 0) start += len;
	if (start < 0 || start > len) {
		notifier.throwException("Index out of range: " + std::to_string(args[1]->i));
		return nullptr;
	}
	int64_t end = len;
	if (argSize > 2 && args[2]) {
		end = args[2]->i;
		if (end < 0) end += len;
		if (end < start || end > len) {
			notifier.throwException("Index out of range: " + std::to_string(args[2]->i));
			return nullptr;
		}
	}
	std::string_view sv(curContent->str->data, curContent->str->size);
	return notifier.createString(std::string(sv.substr(start, end - start)));
}

inline AObject *string_builder_index_of(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string_view full(curContent->str->data, curContent->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	int64_t start = (argSize > 2 && args[2]) ? args[2]->i : 0;
	if (start < 0) start = 0;
	if (start > static_cast<int64_t>(full.size())) return notifier.createInt(-1);
	size_t pos = full.find(target, static_cast<size_t>(start));
	return notifier.createInt(pos == std::string_view::npos ? -1 : static_cast<int64_t>(pos));
}

inline AObject *string_builder_last_index_of(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string_view full(curContent->str->data, curContent->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	int64_t start = (argSize > 2 && args[2] && args[2]->i >= 0) ? args[2]->i : full.size();
	if (start > static_cast<int64_t>(full.size())) start = full.size();
	size_t pos = full.rfind(target, static_cast<size_t>(start));
	return notifier.createInt(pos == std::string_view::npos ? -1 : static_cast<int64_t>(pos));
}

inline AObject *string_builder_contains(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string_view full(curContent->str->data, curContent->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	return notifier.createBool(full.find(target) != std::string_view::npos);
}

inline AObject *string_builder_starts_with(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string_view full(curContent->str->data, curContent->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	return notifier.createBool(full.rfind(target, 0) == 0);
}

inline AObject *string_builder_ends_with(NativeFuncInData) {
	auto self = args[0];
	auto curContent = self->member->data[0];
	std::string_view full(curContent->str->data, curContent->str->size);
	std::string buf;
	std::string_view target = str_arg_view(args[1], buf);
	if (target.size() > full.size()) return notifier.createBool(false);
	return notifier.createBool(full.compare(full.size() - target.size(), target.size(), target) == 0);
}

inline AObject *build_string(NativeFuncInData) {
	auto action = args[0];
	ClassId sbId = 0;
	for (ClassId i = 0; i < notifier.vm->data.classes.size(); ++i) {
		if (notifier.getClassName(i) == "StringBuilder") {
			sbId = i;
			break;
		}
	}
	auto sb = notifier.createMemberObject(sbId, 1);
	auto emptyStr = notifier.createString("");
	emptyStr->retain();
	sb->member->data[0] = emptyStr;
	sb->retain();
	auto ret = notifier.callFunctionObject(action, sb);
	if (ret) notifier.release(ret);
	auto res = sb->member->data[0];
	res->retain();
	notifier.release(sb);
	--res->refCount;
	return res;
}

inline AObject *str_partition(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *predicate = args[1];
	int64_t len = static_cast<int64_t>(s.size());
	std::string first;
	std::string second;
	for (int64_t i = 0; i < len; ++i) {
		AObject *ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto match = notifier.callFunctionObject(predicate, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (match == DefaultClass::trueObject) {
			first += s[i];
		} else {
			second += s[i];
		}
	}
	ClassId pairId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : 0;
	if (pairId < DefaultClass::builtInObjectSize || pairId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[pairId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Pair");
		if (it != notifier.vm->data.classMap.end()) pairId = it->second;
	}
	auto pairObj = notifier.createMemberObject(pairId, 2);
	auto fStr = notifier.createString(first);
	auto sStr = notifier.createString(second);
	pairObj->member->data[0] = fStr;
	pairObj->member->data[1] = sStr;
	fStr->retain();
	sStr->retain();
	return pairObj;
}

inline AObject *str_zip(NativeFuncInData) {
	const std::string &s1 = args[0]->str->data;
	const std::string &s2 = args[1]->str->data;
	size_t minLen = std::min(s1.size(), s2.size());
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	ClassId pairId = 0;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			pairId = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	if (pairId < DefaultClass::builtInObjectSize || pairId >= notifier.vm->data.classes.size() || notifier.vm->data.classes[pairId]->memberMap.empty()) {
		auto it = notifier.vm->data.classMap.find("Pair");
		if (it != notifier.vm->data.classMap.end()) pairId = it->second;
	}
	auto arr = notifier.createArray(returnId, pairId);
	for (size_t i = 0; i < minLen; ++i) {
		auto pairObj = notifier.createMemberObject(pairId, 2);
		auto c1 = notifier.createChar(static_cast<uint8_t>(s1[i]));
		auto c2 = notifier.createChar(static_cast<uint8_t>(s2[i]));
		pairObj->member->data[0] = c1;
		pairObj->member->data[1] = c2;
		c1->retain();
		c2->retain();
		notifier.arrayAdd(arr, pairObj);
	}
	return arr;
}

inline AObject *str_zip_transform(NativeFuncInData) {
	const std::string &s1 = args[0]->str->data;
	const std::string &s2 = args[1]->str->data;
	AObject *transform = args[2];
	size_t minLen = std::min(s1.size(), s2.size());
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	ClassId elemKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			elemKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto arr = notifier.createArray(returnId, elemKey);
	for (size_t i = 0; i < minLen; ++i) {
		auto c1 = notifier.createChar(static_cast<uint8_t>(s1[i]));
		auto c2 = notifier.createChar(static_cast<uint8_t>(s2[i]));
		c1->retain();
		c2->retain();
		auto res = notifier.callFunctionObject(transform, c1, c2);
		notifier.release(c1);
		notifier.release(c2);
		if (notifier.hasException()) return nullptr;
		notifier.arrayAdd(arr, res);
	}
	return arr;
}

inline AObject *str_associate(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *transform = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::mapClassId;
	ClassId keyKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size >= 2) {
			keyKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto map = Libs::map::constructor(notifier, returnId, keyKey);
	map->flags |= AObject::Flags::OBJ_IS_MAP;
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = 0; i < len; ++i) {
		auto ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto pair = notifier.callFunctionObject(transform, ch);
		notifier.release(ch);
		if (notifier.hasException()) return nullptr;
		if (pair && (pair->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && pair->member && pair->member->size >= 2) {
			AObject *setArgs[3] = {map, pair->member->data[0], pair->member->data[1]};
			Libs::map::set(notifier, setArgs, 3);
		}
	}
	return map;
}

inline AObject *str_associate_by(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *keySelector = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::mapClassId;
	ClassId keyKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size >= 2) {
			keyKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto map = Libs::map::constructor(notifier, returnId, keyKey);
	map->flags |= AObject::Flags::OBJ_IS_MAP;
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = 0; i < len; ++i) {
		auto ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto key = notifier.callFunctionObject(keySelector, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			return nullptr;
		}
		AObject *setArgs[3] = {map, key, ch};
		Libs::map::set(notifier, setArgs, 3);
		notifier.release(ch);
	}
	return map;
}

inline AObject *str_associate_with(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *valueSelector = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::mapClassId;
	ClassId keyKey = DefaultClass::charClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size >= 2) {
			keyKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto map = Libs::map::constructor(notifier, returnId, keyKey);
	map->flags |= AObject::Flags::OBJ_IS_MAP;
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = 0; i < len; ++i) {
		auto ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto val = notifier.callFunctionObject(valueSelector, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			return nullptr;
		}
		AObject *setArgs[3] = {map, ch, val};
		Libs::map::set(notifier, setArgs, 3);
		notifier.release(ch);
	}
	return map;
}

inline AObject *str_group_by(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	AObject *keySelector = args[1];
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::mapClassId;
	ClassId keyKey = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size >= 2) {
			keyKey = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	auto map = Libs::map::constructor(notifier, returnId, keyKey);
	map->flags |= AObject::Flags::OBJ_IS_MAP;
	int64_t len = static_cast<int64_t>(s.size());
	for (int64_t i = 0; i < len; ++i) {
		auto ch = notifier.createChar(static_cast<uint8_t>(s[i]));
		ch->retain();
		auto key = notifier.callFunctionObject(keySelector, ch);
		if (notifier.hasException()) {
			notifier.release(ch);
			return nullptr;
		}
		AObject *getArgs[2] = {map, key};
		auto existingArr = Libs::map::get(notifier, getArgs, 2);
		if (!existingArr || existingArr == DefaultClass::nullObject) {
			auto newArr = notifier.createArray(DefaultClass::arrayClassId, DefaultClass::charClassId);
			notifier.arrayAdd(newArr, ch);
			AObject *setArgs[3] = {map, key, newArr};
			Libs::map::set(notifier, setArgs, 3);
		} else {
			notifier.arrayAdd(existingArr, ch);
		}
		notifier.release(ch);
	}
	return map;
}

inline AObject *str_join(NativeFuncInData) {
	if (argSize < 2 || !args[1] || !(args[1]->flags & AObject::Flags::OBJ_IS_ARRAY)) {
		notifier.throwException("String.join: expected an Array");
		return nullptr;
	}
	AObject *callArgs[2] = {args[1], args[0]};
	return Libs::array::join_to_string(notifier, callArgs, 2);
}

inline AObject *str_count_str(NativeFuncInData) {
	if (argSize < 2 || !args[1] || args[1]->type != DefaultClass::stringClassId) {
		notifier.throwException("String.count: expected a String");
		return nullptr;
	}
	const std::string &s = args[0]->str->data;
	const std::string &sub = args[1]->str->data;
	if (sub.empty()) {
		return notifier.createInt(static_cast<int64_t>(s.size() + 1));
	}
	int64_t count = 0;
	size_t pos = 0;
	while ((pos = s.find(sub, pos)) != std::string::npos) {
		++count;
		pos += sub.size();
	}
	return notifier.createInt(count);
}

inline AObject *str_capitalize(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	if (s.empty()) {
		return notifier.createString("");
	}
	std::string res = s;
	res[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[0])));
	return notifier.createString(res);
}

inline AObject *str_char_code(NativeFuncInData) {
	const std::string &s = args[0]->str->data;
	if (s.empty()) {
		notifier.throwException("ord: empty string");
		return nullptr;
	}
	return notifier.createInt(static_cast<uint8_t>(s[0]));
}

} // namespace DefaultFunction
} // namespace Autolang

#endif
