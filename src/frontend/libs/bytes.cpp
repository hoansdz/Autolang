#ifndef LIBS_BYTES_CPP
#define LIBS_BYTES_CPP

#include "backend/vm/ANotifier.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Type.hpp"
#include <cstring>
#include <string>

namespace Autolang {
namespace Libs {
namespace bytes {

AObject *alloc_bytes(NativeFuncInData) {
	int64_t size = args[0]->i;
	if (size < 0) {
		notifier.throwException("Bytes size cannot be negative");
		return nullptr;
	}
	return notifier.createBytes(size);
}

AObject *from_string(NativeFuncInData) {
	const std::string &str = args[0]->str->data;
	AObject *obj = notifier.createBytes(str.size());
	if (str.size() > 0) {
		std::memcpy(obj->bytes->data, str.c_str(), str.size());
		obj->bytes->size = str.size();
	}
	return obj;
}

AObject *append(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);

	if (b->size >= b->capacity) {
		b->capacity = b->capacity == 0 ? 16 : b->capacity * 2;
		uint8_t *newData = new uint8_t[b->capacity];
		if (b->size > 0) {
			std::memcpy(newData, b->data, b->size);
		}
		delete[] b->data;
		b->data = newData;
	}

	b->data[b->size++] = value;
	return nullptr;
}

AObject *size(NativeFuncInData) {
	return notifier.createInt(args[0]->bytes->size);
}

AObject *is_empty(NativeFuncInData) {
	return notifier.createBool(args[0]->bytes->size == 0);
}

AObject *get(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t index = args[1]->i;

	if (index < 0 || index >= b->size) {
		notifier.throwException("Bytes Index out of bounds");
		return nullptr;
	}

	return notifier.createInt(b->data[index]);
}

AObject *set(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t index = args[1]->i;
	uint8_t value = static_cast<uint8_t>(args[2]->i);

	if (index < 0 || index >= b->size) {
		notifier.throwException("Bytes Index out of bounds");
		return nullptr;
	}

	b->data[index] = value;
	return nullptr;
}

AObject *clear(NativeFuncInData) {
	args[0]->bytes->size = 0;
	return nullptr;
}

AObject *slice(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t from = args[1]->i;
	int64_t to = args[2]->i;

	if (from < 0)
		from = 0;
	if (to > b->size)
		to = b->size;
	if (from > to)
		from = to;

	int64_t newSize = to - from;
	AObject *newObj = notifier.createBytes(newSize);
	ABytes *newB = newObj->bytes;

	if (newSize > 0) {
		std::memcpy(newB->data, b->data + from, newSize);
		newB->size = newSize;
	}

	return newObj;
}

AObject *copy_from(NativeFuncInData) {
	ABytes *dest = args[0]->bytes;
	ABytes *src = args[1]->bytes;
	int64_t destOffset = args[2]->i;
	int64_t srcOffset = args[3]->i;
	int64_t length = args[4]->i;

	if (srcOffset < 0 || srcOffset + length > src->size) {
		notifier.throwException("Source bounds out of range");
		return nullptr;
	}
	if (destOffset < 0 || destOffset + length > dest->size) {
		notifier.throwException("Destination bounds out of range");
		return nullptr;
	}
	if (length > 0) {
		std::memcpy(dest->data + destOffset, src->data + srcOffset, length);
	}
	return nullptr;
}

AObject *equals(NativeFuncInData) {
	ABytes *b1 = args[0]->bytes;
	ABytes *b2 = args[1]->bytes;

	if (b1->size != b2->size) {
		return notifier.createBool(false);
	}
	if (b1->size == 0) {
		return notifier.createBool(true);
	}
	return notifier.createBool(std::memcmp(b1->data, b2->data, b1->size) == 0);
}

AObject *to_string(NativeFuncInData) {
	ABytes *b = args[0]->bytes;

	if (b->size == 0) {
		return notifier.createString("[]");
	}

	std::string str = "[";
	for (int64_t i = 0; i < b->size; ++i) {
		str += std::to_string(b->data[i]);
		if (i != b->size - 1) {
			str += ", ";
		}
	}
	str += "]";

	return notifier.createString(str);
}

AObject *to_utf8_string(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0) {
		return notifier.createString("");
	}
	return notifier.createString(
	    std::string(reinterpret_cast<char *>(b->data), b->size));
}

AObject *to_hex(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0) {
		return notifier.createString("");
	}

	std::string hex;
	hex.reserve(b->size * 2);
	static const char hexChars[] = "0123456789abcdef";
	for (int64_t i = 0; i < b->size; ++i) {
		hex.push_back(hexChars[(b->data[i] >> 4) & 0x0F]);
		hex.push_back(hexChars[b->data[i] & 0x0F]);
	}
	return notifier.createString(hex);
}

AObject *ext_string_to_bytes(NativeFuncInData) {
	const std::string &str = args[0]->str->data;
	AObject *obj = notifier.createBytes(str.size());
	if (str.size() > 0) {
		std::memcpy(obj->bytes->data, str.c_str(), str.size());
		obj->bytes->size = str.size();
	}
	return obj;
}

AObject *ext_string_from_bytes(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0) {
		return notifier.createString("");
	}
	return notifier.createString(
	    std::string(reinterpret_cast<char *>(b->data), b->size));
}

AObject *decode_to_string(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (!b || b->size == 0) {
		return notifier.createString("");
	}
	int64_t start = (argSize >= 2 && args[1]->type == DefaultClass::intClassId) ? args[1]->i : 0;
	int64_t end = (argSize >= 3 && args[2]->type == DefaultClass::intClassId && args[2]->i >= 0) ? args[2]->i : b->size;
	if (start < 0) start = 0;
	if (end > b->size) end = b->size;
	if (start >= end) return notifier.createString("");
	return notifier.createString(std::string(reinterpret_cast<char *>(b->data + start), end - start));
}

AObject *encode_to_byte_array(NativeFuncInData) {
	const std::string &str = args[0]->str->data;
	int64_t start = (argSize >= 2 && args[1]->type == DefaultClass::intClassId) ? args[1]->i : 0;
	int64_t end = (argSize >= 3 && args[2]->type == DefaultClass::intClassId && args[2]->i >= 0) ? args[2]->i : static_cast<int64_t>(str.size());
	if (start < 0) start = 0;
	if (end > static_cast<int64_t>(str.size())) end = static_cast<int64_t>(str.size());
	if (start >= end) return notifier.createBytes(0);
	int64_t len = end - start;
	AObject *obj = notifier.createBytes(len);
	std::memcpy(obj->bytes->data, str.data() + start, len);
	obj->bytes->size = len;
	return obj;
}

AObject *ext_int_to_bytes(NativeFuncInData) {
	int64_t val = args[0]->i;
	AObject *obj = notifier.createBytes(8);
	obj->bytes->size = 8;
	for (int i = 0; i < 8; ++i) {
		obj->bytes->data[i] = (val >> ((7 - i) * 8)) & 0xFF;
	}
	return obj;
}

AObject *fill(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);
	if (b->size > 0) {
		std::memset(b->data, value, b->size);
	}
	return nullptr;
}

AObject *index_of(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);
	int64_t fromIndex = args[2]->i;

	if (fromIndex < 0)
		fromIndex = 0;
	if (fromIndex >= b->size)
		return notifier.createInt(-1);

	void *match = std::memchr(b->data + fromIndex, value, b->size - fromIndex);
	if (match) {
		return notifier.createInt(static_cast<uint8_t *>(match) - b->data);
	}
	return notifier.createInt(-1);
}

AObject *read_int64_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;

	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}

	int64_t result = 0;
	for (int i = 0; i < 8; ++i) {
		result |= static_cast<int64_t>(b->data[offset + i]) << (i * 8);
	}
	return notifier.createInt(result);
}

AObject *write_int64_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;

	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}

	for (int i = 0; i < 8; ++i) {
		b->data[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
	}
	return nullptr;
}

AObject *read_float_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;

	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}

	uint64_t rawValue = 0;
	for (int i = 0; i < 8; ++i) {
		rawValue |= static_cast<uint64_t>(b->data[offset + i]) << (i * 8);
	}

	double result;
	std::memcpy(&result, &rawValue, 8);
	return notifier.createFloat(result);
}

AObject *to_base64(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0)
		return notifier.createString("");

	static const char base64_chars[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string ret;
	int i = 0, j = 0;
	uint8_t char_array_3[3];
	uint8_t char_array_4[4];

	for (int64_t k = 0; k < b->size; ++k) {
		char_array_3[i++] = b->data[k];
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
			                  ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
			                  ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}
	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] =
		    ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] =
		    ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];
		while ((i++ < 3))
			ret += '=';
	}
	return notifier.createString(ret);
}

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

AObject *from_base64(NativeFuncInData) {
	static const std::string base64_chars =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const std::string &encoded_string = args[0]->str->data;
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::vector<uint8_t> ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
	}

	AObject *obj = notifier.createBytes(ret.size());
	if (!ret.empty()) {
		std::memcpy(obj->bytes->data, ret.data(), ret.size());
		obj->bytes->size = ret.size();
	}
	return obj;
}

AObject *from_hex(NativeFuncInData) {
	const std::string &hex = args[0]->str->data;
	size_t len = hex.length();
	AObject *obj = notifier.createBytes(len / 2);
	auto b = obj->bytes;
	b->size = 0;
	for (size_t i = 0; i + 1 < len; i += 2) {
		char byteStr[3] = {hex[i], hex[i + 1], '\0'};
		uint8_t byteVal = static_cast<uint8_t>(std::strtoul(byteStr, nullptr, 16));
		b->data[b->size++] = byteVal;
	}
	return obj;
}

AObject *to_byte_array(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	ClassId arrayClassId = notifier.callFrame->func->returnId;
	AObject *arr = notifier.createArray(arrayClassId);
	for (size_t i = 0; i < b->size; ++i) {
		notifier.arrayAdd(arr, notifier.createInt(b->data[i]));
	}
	return arr;
}

AObject *read_int32_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;

	if (offset < 0 || offset + 4 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}

	int32_t result = (b->data[offset] << 24) | (b->data[offset + 1] << 16) |
	                 (b->data[offset + 2] << 8) | b->data[offset + 3];
	return notifier.createInt(result);
}

AObject *write_int32_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;

	if (offset < 0 || offset + 4 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}

	b->data[offset] = (value >> 24) & 0xFF;
	b->data[offset + 1] = (value >> 16) & 0xFF;
	b->data[offset + 2] = (value >> 8) & 0xFF;
	b->data[offset + 3] = value & 0xFF;
	return nullptr;
}

AObject *xor_with(NativeFuncInData) {
	ABytes *dest = args[0]->bytes;
	ABytes *src = args[1]->bytes;
	int64_t length = args[2]->i;

	if (length < 0 || length > dest->size || length > src->size) {
		notifier.throwException("XOR length out of bounds");
		return nullptr;
	}

	for (int64_t i = 0; i < length; ++i) {
		dest->data[i] ^= src->data[i];
	}
	return nullptr;
}

AObject *copy_of(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t newSize = (argSize >= 2 && args[1]->type == DefaultClass::intClassId) ? args[1]->i : b->size;
	if (newSize < 0) {
		notifier.throwException("Negative size for copyOf");
		return nullptr;
	}
	AObject *newObj = notifier.createBytes(newSize);
	ABytes *newB = newObj->bytes;
	int64_t copyLen = std::min(static_cast<int64_t>(b->size), newSize);
	if (copyLen > 0) {
		std::memcpy(newB->data, b->data, copyLen);
	}
	if (newSize > copyLen) {
		std::memset(newB->data + copyLen, 0, newSize - copyLen);
	}
	return newObj;
}

AObject *copy_of_range(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t from = args[1]->i;
	int64_t to = args[2]->i;
	if (from < 0 || from > to || to > b->size) {
		notifier.throwException("Index out of bounds in copyOfRange");
		return nullptr;
	}
	int64_t newSize = to - from;
	AObject *newObj = notifier.createBytes(newSize);
	if (newSize > 0) {
		std::memcpy(newObj->bytes->data, b->data + from, newSize);
	}
	return newObj;
}

AObject *reversed(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	AObject *newObj = notifier.createBytes(b->size);
	for (int64_t i = 0; i < b->size; ++i) {
		newObj->bytes->data[i] = b->data[b->size - 1 - i];
	}
	return newObj;
}

AObject *reverse(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	for (int64_t i = 0, j = b->size - 1; i < j; ++i, --j) {
		std::swap(b->data[i], b->data[j]);
	}
	return args[0];
}

AObject *contains(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);
	for (int64_t i = 0; i < b->size; ++i) {
		if (b->data[i] == value) return DefaultClass::trueObject;
	}
	return DefaultClass::falseObject;
}

AObject *last_index_of(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);
	int64_t startIndex = (argSize >= 3 && args[2]->type == DefaultClass::intClassId && args[2]->i >= 0)
	    ? args[2]->i
	    : (b->size - 1);
	if (startIndex >= b->size) startIndex = b->size - 1;
	for (int64_t i = startIndex; i >= 0; --i) {
		if (b->data[i] == value) return notifier.createInt(i);
	}
	return notifier.createInt(-1);
}

AObject *last_index(NativeFuncInData) {
	return notifier.createInt(args[0]->bytes->size - 1);
}

AObject *indices(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	ClassId arrayClassId = notifier.callFrame && notifier.callFrame->func ? notifier.callFrame->func->returnId : 0;
	AObject *arr = notifier.createArray(arrayClassId);
	for (int64_t i = 0; i < b->size; ++i) {
		notifier.arrayAdd(arr, notifier.createInt(i));
	}
	return arr;
}

AObject *for_each(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	AObject *action = args[1];
	for (int64_t i = 0; i < b->size; ++i) {
		AObject *byteVal = notifier.createInt(b->data[i]);
		byteVal->retain();
		[[maybe_unused]] auto res = notifier.callFunctionObject(action, byteVal);
		notifier.release(byteVal);
		if (notifier.hasException()) return nullptr;
	}
	return nullptr;
}

AObject *for_each_indexed(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	AObject *action = args[1];
	for (int64_t i = 0; i < b->size; ++i) {
		AObject *idx = notifier.createInt(i);
		AObject *byteVal = notifier.createInt(b->data[i]);
		idx->retain();
		byteVal->retain();
		[[maybe_unused]] auto res = notifier.callFunctionObject(action, idx, byteVal);
		notifier.release(idx);
		notifier.release(byteVal);
		if (notifier.hasException()) return nullptr;
	}
	return nullptr;
}

AObject *read_int16_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	if (offset < 0 || offset + 2 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}
	int16_t result = static_cast<int16_t>(b->data[offset] | (b->data[offset + 1] << 8));
	return notifier.createInt(result);
}

AObject *write_int16_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;
	if (offset < 0 || offset + 2 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	b->data[offset] = value & 0xFF;
	b->data[offset + 1] = (value >> 8) & 0xFF;
	return nullptr;
}

AObject *read_int16_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	if (offset < 0 || offset + 2 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}
	int16_t result = static_cast<int16_t>((b->data[offset] << 8) | b->data[offset + 1]);
	return notifier.createInt(result);
}

AObject *write_int16_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;
	if (offset < 0 || offset + 2 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	b->data[offset] = (value >> 8) & 0xFF;
	b->data[offset + 1] = value & 0xFF;
	return nullptr;
}

AObject *read_int32_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	if (offset < 0 || offset + 4 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}
	int32_t result = static_cast<int32_t>(b->data[offset] | (b->data[offset + 1] << 8) |
	                                      (b->data[offset + 2] << 16) | (b->data[offset + 3] << 24));
	return notifier.createInt(result);
}

AObject *write_int32_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;
	if (offset < 0 || offset + 4 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	b->data[offset] = value & 0xFF;
	b->data[offset + 1] = (value >> 8) & 0xFF;
	b->data[offset + 2] = (value >> 16) & 0xFF;
	b->data[offset + 3] = (value >> 24) & 0xFF;
	return nullptr;
}

AObject *read_int64_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}
	int64_t result = 0;
	for (int i = 0; i < 8; ++i) {
		result = (result << 8) | static_cast<int64_t>(b->data[offset + i]);
	}
	return notifier.createInt(result);
}

AObject *write_int64_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	int64_t value = args[2]->i;
	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	for (int i = 0; i < 8; ++i) {
		b->data[offset + i] = static_cast<uint8_t>((value >> ((7 - i) * 8)) & 0xFF);
	}
	return nullptr;
}

AObject *read_float_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Read out of bounds");
		return nullptr;
	}
	uint64_t rawValue = 0;
	for (int i = 0; i < 8; ++i) {
		rawValue = (rawValue << 8) | static_cast<uint64_t>(b->data[offset + i]);
	}
	double result;
	std::memcpy(&result, &rawValue, 8);
	return notifier.createFloat(result);
}

AObject *write_float_le(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	double val = (args[2]->type == DefaultClass::floatClassId) ? args[2]->f : static_cast<double>(args[2]->i);
	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	uint64_t rawValue;
	std::memcpy(&rawValue, &val, 8);
	for (int i = 0; i < 8; ++i) {
		b->data[offset + i] = static_cast<uint8_t>((rawValue >> (i * 8)) & 0xFF);
	}
	return nullptr;
}

AObject *write_float_be(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t offset = args[1]->i;
	double val = (args[2]->type == DefaultClass::floatClassId) ? args[2]->f : static_cast<double>(args[2]->i);
	if (offset < 0 || offset + 8 > b->size) {
		notifier.throwException("Write out of bounds");
		return nullptr;
	}
	uint64_t rawValue;
	std::memcpy(&rawValue, &val, 8);
	for (int i = 0; i < 8; ++i) {
		b->data[offset + i] = static_cast<uint8_t>((rawValue >> ((7 - i) * 8)) & 0xFF);
	}
	return nullptr;
}

AObject *from_array(NativeFuncInData) {
	AObject *arrObj = args[0];
	if (!arrObj || !arrObj->array) {
		return notifier.createBytes(0);
	}
	AArray *arr = arrObj->array;
	AObject *obj = notifier.createBytes(arr->size);
	if (arr->key == DefaultClass::intClassId && arr->intData) {
		for (size_t i = 0; i < arr->size; ++i) {
			int64_t val = arr->intData[i];
			if (val < -128 || val > 255) {
				notifier.throwException("Byte value out of range: " + std::to_string(val));
				return nullptr;
			}
			obj->bytes->data[i] = static_cast<uint8_t>(val);
		}
	} else if (arr->raw) {
		for (size_t i = 0; i < arr->size; ++i) {
			if (arr->objData[i]) {
				int64_t val = arr->objData[i]->i;
				if (val < -128 || val > 255) {
					notifier.throwException("Byte value out of range: " + std::to_string(val));
					return nullptr;
				}
				obj->bytes->data[i] = static_cast<uint8_t>(val);
			} else {
				obj->bytes->data[i] = 0;
			}
		}
	}
	return obj;
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/bytes", R"###(
@native("bytes_constructor")
static fun Bytes.Bytes(initialSize: Int = 0): Bytes

@implicit
@native("bytes_from_array")
static fun Bytes.Bytes(arr: Array<Int>): Bytes

@native("bytes_from_string_static")
static fun Bytes.fromString(str: String): Bytes

@native("bytes_append")
fun Bytes.append(value: Int)

@native("bytes_append")
fun Bytes.add(value: Int)

@native("bytes_append")
fun Bytes.push(value: Int)

@native("bytes_append")
fun Bytes.push_back(value: Int)

@native("bytes_size")
fun Bytes.size(): Int

@native("bytes_size")
fun Bytes.length(): Int

@native("bytes_size")
fun Bytes.len(): Int

@native("bytes_is_empty")
fun Bytes.isEmpty(): Bool

@native("bytes_is_empty")
fun Bytes.empty(): Bool

@native("bytes_is_empty")
fun Bytes.is_empty(): Bool

@native("bytes_get")
operator fun Bytes.get(index: Int): Int

@native("bytes_get")
fun Bytes.at(index: Int): Int

@native("bytes_get")
fun Bytes.readByte(index: Int): Int

@native("bytes_set")
operator fun Bytes.set(index: Int, value: Int)

@native("bytes_set")
fun Bytes.put(index: Int, value: Int)

@native("bytes_set")
fun Bytes.writeByte(index: Int, value: Int)

@native("bytes_clear")
fun Bytes.clear()

@native("bytes_slice")
fun Bytes.slice(from: Int, to: Int): Bytes

@native("bytes_slice")
fun Bytes.subArray(from: Int, to: Int): Bytes

@native("bytes_copy_from")
fun Bytes.copyFrom(src: Bytes, destOffset: Int, srcOffset: Int, lenBytes: Int)

@native("bytes_copy_of")
fun Bytes.copyOf(): Bytes

@native("bytes_copy_of")
fun Bytes.copyOf(newSize: Int): Bytes

@native("bytes_copy_of_range")
fun Bytes.copyOfRange(fromIndex: Int, toIndex: Int): Bytes

@native("bytes_reversed")
fun Bytes.reversed(): Bytes

@native("bytes_reverse")
fun Bytes.reverse(): Bytes

@native("bytes_contains")
fun Bytes.contains(byteValue: Int): Bool

@native("bytes_index_of")
fun Bytes.indexOf(byteValue: Int, fromIndex: Int = 0): Int

@native("bytes_last_index_of")
fun Bytes.lastIndexOf(byteValue: Int, startIndex: Int = -1): Int

@native("bytes_last_index")
fun Bytes.lastIndex(): Int

@native("bytes_indices")
fun Bytes.indices(): Array<Int>

@native("bytes_equals")
fun Bytes.equals(other: Bytes): Bool

@native("bytes_equals")
fun Bytes.contentEquals(other: Bytes): Bool

@native("bytes_to_string")
fun Bytes.toString(): String

@native("bytes_to_utf8_string")
fun Bytes.toUtf8String(): String

@native("bytes_to_hex")
fun Bytes.toHex(): String

@native("bytes_fill")
fun Bytes.fill(value: Int)

@native("bytes_read_int16_le")
fun Bytes.readInt16LE(offset: Int): Int

@native("bytes_write_int16_le")
fun Bytes.writeInt16LE(offset: Int, value: Int)

@native("bytes_read_int16_be")
fun Bytes.readInt16BE(offset: Int): Int

@native("bytes_write_int16_be")
fun Bytes.writeInt16BE(offset: Int, value: Int)

@native("bytes_read_int32_le")
fun Bytes.readInt32LE(offset: Int): Int

@native("bytes_write_int32_le")
fun Bytes.writeInt32LE(offset: Int, value: Int)

@native("bytes_read_int32_be")
fun Bytes.readInt32BE(offset: Int): Int

@native("bytes_write_int32_be")
fun Bytes.writeInt32BE(offset: Int, value: Int)

@native("bytes_read_int64_le")
fun Bytes.readInt64LE(offset: Int): Int

@native("bytes_write_int64_le")
fun Bytes.writeInt64LE(offset: Int, value: Int)

@native("bytes_read_int64_be")
fun Bytes.readInt64BE(offset: Int): Int

@native("bytes_write_int64_be")
fun Bytes.writeInt64BE(offset: Int, value: Int)

@native("bytes_read_float_le")
fun Bytes.readFloatLE(offset: Int): Float

@native("bytes_read_float_be")
fun Bytes.readFloatBE(offset: Int): Float

@native("bytes_write_float_le")
fun Bytes.writeFloatLE(offset: Int, value: Float)

@native("bytes_write_float_be")
fun Bytes.writeFloatBE(offset: Int, value: Float)

@native("bytes_to_base64")
fun Bytes.toBase64(): String

fun Bytes.encodeBase64(): String = this.toBase64()

@native("bytes_from_base64")
static fun Bytes.fromBase64(base64: String): Bytes

@native("bytes_from_hex")
static fun Bytes.fromHex(hex: String): Bytes

@native("bytes_to_byte_array")
fun Bytes.toByteArray(): Array<Int>

@native("bytes_to_byte_array")
fun Bytes.toList(): Array<Int>

@native("bytes_decode_to_string")
fun Bytes.decodeToString(startIndex: Int = 0, endIndex: Int = -1): String

@native("bytes_xor_with")
fun Bytes.xorWith(other: Bytes, lenBytes: Int)

@native("bytes_for_each")
fun Bytes.forEach(action: (Int) -> Void)

@native("bytes_for_each_indexed")
fun Bytes.forEachIndexed(action: (Int, Int) -> Void)

@native("bytes_ext_string_to_bytes")
fun String.toBytes(): Bytes

@native("bytes_encode_to_byte_array")
fun String.encodeToByteArray(startIndex: Int = 0, endIndex: Int = -1): Bytes

@native("bytes_ext_string_from_bytes")
static fun String.fromBytes(bytes: Bytes): String

@native("bytes_ext_int_to_bytes")
fun Int.toBigEndianBytes(): Bytes
        )###",
	    LibraryConfig(true),
	    ANativeMap({
	        {"bytes_constructor", &bytes::alloc_bytes},
	        {"bytes_from_string_static", &bytes::from_string},
	        {"bytes_from_hex", &bytes::from_hex},
	        {"bytes_from_base64", &bytes::from_base64},
	        {"bytes_to_byte_array", &bytes::to_byte_array},
	        {"bytes_append", &bytes::append},
	        {"bytes_size", &bytes::size},
	        {"bytes_is_empty", &bytes::is_empty},
	        {"bytes_get", &bytes::get},
	        {"bytes_set", &bytes::set},
	        {"bytes_clear", &bytes::clear},
	        {"bytes_slice", &bytes::slice},
	        {"bytes_copy_from", &bytes::copy_from},
	        {"bytes_copy_of", &bytes::copy_of},
	        {"bytes_copy_of_range", &bytes::copy_of_range},
	        {"bytes_reversed", &bytes::reversed},
	        {"bytes_reverse", &bytes::reverse},
	        {"bytes_contains", &bytes::contains},
	        {"bytes_last_index_of", &bytes::last_index_of},
	        {"bytes_last_index", &bytes::last_index},
	        {"bytes_indices", &bytes::indices},
	        {"bytes_equals", &bytes::equals},
	        {"bytes_to_string", &bytes::to_string},
	        {"bytes_to_utf8_string", &bytes::to_utf8_string},
	        {"bytes_to_hex", &bytes::to_hex},
	        {"bytes_fill", &bytes::fill},
	        {"bytes_index_of", &bytes::index_of},
	        {"bytes_read_int16_le", &bytes::read_int16_le},
	        {"bytes_write_int16_le", &bytes::write_int16_le},
	        {"bytes_read_int16_be", &bytes::read_int16_be},
	        {"bytes_write_int16_be", &bytes::write_int16_be},
	        {"bytes_read_int32_le", &bytes::read_int32_le},
	        {"bytes_write_int32_le", &bytes::write_int32_le},
	        {"bytes_read_int32_be", &bytes::read_int32_be},
	        {"bytes_write_int32_be", &bytes::write_int32_be},
	        {"bytes_read_int64_le", &bytes::read_int64_le},
	        {"bytes_write_int64_le", &bytes::write_int64_le},
	        {"bytes_read_int64_be", &bytes::read_int64_be},
	        {"bytes_write_int64_be", &bytes::write_int64_be},
	        {"bytes_read_float_le", &bytes::read_float_le},
	        {"bytes_read_float_be", &bytes::read_float_be},
	        {"bytes_write_float_le", &bytes::write_float_le},
	        {"bytes_write_float_be", &bytes::write_float_be},
	        {"bytes_to_base64", &bytes::to_base64},
	        {"bytes_xor_with", &bytes::xor_with},
	        {"bytes_for_each", &bytes::for_each},
	        {"bytes_for_each_indexed", &bytes::for_each_indexed},
	        {"bytes_ext_string_to_bytes", &bytes::ext_string_to_bytes},
	        {"bytes_ext_string_from_bytes", &bytes::ext_string_from_bytes},
	        {"bytes_ext_int_to_bytes", &bytes::ext_int_to_bytes},
	        {"bytes_decode_to_string", &bytes::decode_to_string},
	        {"bytes_encode_to_byte_array", &bytes::encode_to_byte_array},
	        {"bytes_from_array", &bytes::from_array},
	    }));
}

} // namespace bytes
} // namespace Libs
} // namespace Autolang

#endif
