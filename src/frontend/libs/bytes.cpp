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

inline AObject *alloc_bytes(NativeFuncInData) {
	int64_t size = args[0]->i;
	if (size < 0) {
		notifier.throwException("Bytes size cannot be negative");
		return nullptr;
	}
	return notifier.createBytes(size);
}

inline AObject *from_string(NativeFuncInData) {
	const std::string &str = args[0]->str->data;
	AObject *obj = notifier.createBytes(str.size());
	if (str.size() > 0) {
		std::memcpy(obj->bytes->data, str.c_str(), str.size());
		obj->bytes->size = str.size();
	}
	return obj;
}

inline AObject *append(NativeFuncInData) {
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

inline AObject *size(NativeFuncInData) {
	return notifier.createInt(args[0]->bytes->size);
}

inline AObject *is_empty(NativeFuncInData) {
	return notifier.createBool(args[0]->bytes->size == 0);
}

inline AObject *get(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	int64_t index = args[1]->i;

	if (index < 0 || index >= b->size) {
		notifier.throwException("Bytes Index out of bounds");
		return nullptr;
	}

	return notifier.createInt(b->data[index]);
}

inline AObject *set(NativeFuncInData) {
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

inline AObject *clear(NativeFuncInData) {
	args[0]->bytes->size = 0;
	return nullptr;
}

inline AObject *slice(NativeFuncInData) {
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

inline AObject *copy_from(NativeFuncInData) {
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

inline AObject *equals(NativeFuncInData) {
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

inline AObject *to_string(NativeFuncInData) {
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

inline AObject *to_utf8_string(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0) {
		return notifier.createString("");
	}
	return notifier.createString(
	    std::string(reinterpret_cast<char *>(b->data), b->size));
}

inline AObject *to_hex(NativeFuncInData) {
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

inline AObject *ext_string_to_bytes(NativeFuncInData) {
	const std::string &str = args[0]->str->data;
	AObject *obj = notifier.createBytes(str.size());
	if (str.size() > 0) {
		std::memcpy(obj->bytes->data, str.c_str(), str.size());
		obj->bytes->size = str.size();
	}
	return obj;
}

inline AObject *ext_string_from_bytes(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	if (b->size == 0) {
		return notifier.createString("");
	}
	return notifier.createString(
	    std::string(reinterpret_cast<char *>(b->data), b->size));
}

inline AObject *ext_int_to_bytes(NativeFuncInData) {
	int64_t val = args[0]->i;
	AObject *obj = notifier.createBytes(8);
	obj->bytes->size = 8;
	for (int i = 0; i < 8; ++i) {
		obj->bytes->data[i] = (val >> ((7 - i) * 8)) & 0xFF;
	}
	return obj;
}

inline AObject *fill(NativeFuncInData) {
	ABytes *b = args[0]->bytes;
	uint8_t value = static_cast<uint8_t>(args[1]->i);
	if (b->size > 0) {
		std::memset(b->data, value, b->size);
	}
	return nullptr;
}

inline AObject *index_of(NativeFuncInData) {
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

inline AObject *read_int64_le(NativeFuncInData) {
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

inline AObject *write_int64_le(NativeFuncInData) {
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

inline AObject *read_float_le(NativeFuncInData) {
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

inline AObject *to_base64(NativeFuncInData) {
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

inline AObject *read_int32_be(NativeFuncInData) {
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

inline AObject *write_int32_be(NativeFuncInData) {
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

inline AObject *xor_with(NativeFuncInData) {
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

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/bytes", R"###(
@native("bytes_constructor")
static fun Bytes.Bytes(initialSize: Int = 0): Bytes

@native("bytes_from_string_static")
static fun Bytes.fromString(str: String): Bytes

@native("bytes_append")
fun Bytes.append(value: Int)

@native("bytes_size")
fun Bytes.size(): Int

@native("bytes_is_empty")
fun Bytes.isEmpty(): Bool

@native("bytes_get")
fun Bytes.get(index: Int): Int

@native("bytes_set")
fun Bytes.set(index: Int, value: Int)

@native("bytes_clear")
fun Bytes.clear()

@native("bytes_slice")
fun Bytes.slice(from: Int, to: Int): Bytes

@native("bytes_copy_from")
fun Bytes.copyFrom(src: Bytes, destOffset: Int, srcOffset: Int, length: Int)

@native("bytes_equals")
fun Bytes.equals(other: Bytes): Bytes

@native("bytes_to_string")
fun Bytes.toString(): String

@native("bytes_to_utf8_string")
fun Bytes.toUtf8String(): String

@native("bytes_to_hex")
fun Bytes.toHex(): String

@native("bytes_fill")
fun Bytes.fill(value: Int)

@native("bytes_index_of")
fun Bytes.indexOf(byteValue: Int, fromIndex: Int = 0): Int

@native("bytes_read_int64_le")
fun Bytes.readInt64LE(offset: Int): Int

@native("bytes_write_int64_le")
fun Bytes.writeInt64LE(offset: Int, value: Int)

@native("bytes_read_float_le")
fun Bytes.readFloatLE(offset: Int): Float

@native("bytes_to_base64")
fun Bytes.toBase64(): String

@native("bytes_read_int32_be")
fun Bytes.readInt32BE(offset: Int): Int

@native("bytes_write_int32_be")
fun Bytes.writeInt32BE(offset: Int, value: Int)

@native("bytes_xor_with")
fun Bytes.xorWith(other: Bytes, length: Int)

@native("bytes_ext_string_to_bytes")
fun String.toBytes(): Bytes

@native("bytes_ext_string_from_bytes")
static fun String.fromBytes(bytes: Bytes): String

@native("bytes_ext_int_to_bytes")
fun Int.toBigEndianBytes(): Bytes
        )###",
	    LibraryConfig(),
	    ANativeMap({
	        {"bytes_constructor", &bytes::alloc_bytes},
	        {"bytes_from_string_static", &bytes::from_string},
	        {"bytes_append", &bytes::append},
	        {"bytes_size", &bytes::size},
	        {"bytes_is_empty", &bytes::is_empty},
	        {"bytes_get", &bytes::get},
	        {"bytes_set", &bytes::set},
	        {"bytes_clear", &bytes::clear},
	        {"bytes_slice", &bytes::slice},
	        {"bytes_copy_from", &bytes::copy_from},
	        {"bytes_equals", &bytes::equals},
	        {"bytes_to_string", &bytes::to_string},
	        {"bytes_to_utf8_string", &bytes::to_utf8_string},
	        {"bytes_to_hex", &bytes::to_hex},
	        {"bytes_fill", &bytes::fill},
	        {"bytes_index_of", &bytes::index_of},
	        {"bytes_read_int64_le", &bytes::read_int64_le},
	        {"bytes_write_int64_le", &bytes::write_int64_le},
	        {"bytes_read_float_le", &bytes::read_float_le},
	        {"bytes_to_base64", &bytes::to_base64},
	        {"bytes_read_int32_be", &bytes::read_int32_be},
	        {"bytes_write_int32_be", &bytes::write_int32_be},
	        {"bytes_xor_with", &bytes::xor_with},
	        {"bytes_ext_string_to_bytes", &bytes::ext_string_to_bytes},
	        {"bytes_ext_string_from_bytes", &bytes::ext_string_from_bytes},
	        {"bytes_ext_int_to_bytes", &bytes::ext_int_to_bytes},
	    }));
}

} // namespace bytes
} // namespace Libs
} // namespace Autolang

#endif