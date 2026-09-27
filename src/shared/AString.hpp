#ifndef ASTRING_HPP
#define ASTRING_HPP

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

class AString {
public:
	static constexpr uint32_t FLAG_IS_REF = 1u << 0;

	char *data;
	uint32_t size;
	uint32_t flags;

	AString(char *data, uint32_t size, uint32_t flags = 0)
	    : data(data), size(size), flags(flags) {}

	inline bool isRef() const { return (flags & FLAG_IS_REF) != 0; }
	inline void setRef(bool ref) {
		if (ref)
			flags |= FLAG_IS_REF;
		else
			flags &= ~FLAG_IS_REF;
	}

	inline static AString *ref(const char *data, uint32_t size) {
		return new AString(const_cast<char *>(data), size, FLAG_IS_REF);
	}

	inline static AString *from(const char *value) {
		size_t size = strlen(value);
		char *str = new char[size + 1];
		memcpy(str, value, size + 1);
		return new AString(str, static_cast<uint32_t>(size));
	}

	inline static AString *from(const std::string &value) {
		char *str = new char[value.size() + 1];
		memcpy(str, value.c_str(), value.size());
		str[value.size()] = '\0';
		return new AString(str, static_cast<uint32_t>(value.size()));
	}

	inline static AString *from(char chr) {
		char *str = new char[2];
		str[0] = chr;
		str[1] = '\0';
		return new AString(str, 1);
	}

	template <typename T>
	inline static AString *from(T value) {
		std::string val = std::to_string(value);
		char *str = new char[val.size() + 1];
		memcpy(str, val.c_str(), val.size());
		str[val.size()] = '\0';
		return new AString(str, static_cast<uint32_t>(val.size()));
	}

	inline static std::string codePointToUtf8(uint32_t cp) {
		std::string out;
		if (cp <= 0x7F) {
			out.push_back(static_cast<char>(cp));
		} else if (cp <= 0x7FF) {
			out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		} else if (cp <= 0xFFFF) {
			out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		} else if (cp <= 0x10FFFF) {
			out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		return out;
	}

	inline static uint32_t utf8ToCodePoint(std::string_view sv, size_t &idx) {
		if (idx >= sv.size()) return 0;
		unsigned char c = static_cast<unsigned char>(sv[idx]);
		if (c < 0x80) {
			idx += 1;
			return c;
		} else if ((c >> 5) == 0x06 && idx + 1 < sv.size()) {
			uint32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(sv[idx + 1]) & 0x3F);
			idx += 2;
			return cp;
		} else if ((c >> 4) == 0x0E && idx + 2 < sv.size()) {
			uint32_t cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(sv[idx + 1]) & 0x3F) << 6) | (static_cast<unsigned char>(sv[idx + 2]) & 0x3F);
			idx += 3;
			return cp;
		} else if ((c >> 3) == 0x1E && idx + 3 < sv.size()) {
			uint32_t cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(sv[idx + 1]) & 0x3F) << 12) | ((static_cast<unsigned char>(sv[idx + 2]) & 0x3F) << 6) | (static_cast<unsigned char>(sv[idx + 3]) & 0x3F);
			idx += 4;
			return cp;
		}
		idx += 1;
		return c;
	}

	inline static AString *copy(AString *other) {
		char *newStr = new char[other->size + 1];
		memcpy(newStr, other->data, other->size + 1);
		return new AString(newStr, other->size);
	}

	inline AString *operator+(AString *other) {
		uint32_t newSize = size + other->size;
		char *newStr = new char[newSize + 1];
		memcpy(newStr, data, size);
		memcpy(newStr + size, other->data, other->size);
		newStr[newSize] = '\0';
		return new AString(newStr, newSize);
	}

	template <typename T>
	inline AString *operator+(T value) {
		std::string other = std::to_string(value);
		uint32_t newSize = size + static_cast<uint32_t>(other.size());
		char *newStr = new char[newSize + 1];
		memcpy(newStr, data, size);
		memcpy(newStr + size, other.c_str(), other.size());
		newStr[newSize] = '\0';
		return new AString(newStr, newSize);
	}

	inline AString *operator+(const char *value) {
		std::string other = value;
		uint32_t newSize = size + static_cast<uint32_t>(other.size());
		char *newStr = new char[newSize + 1];
		memcpy(newStr, data, size);
		memcpy(newStr + size, other.c_str(), other.size());
		newStr[newSize] = '\0';
		return new AString(newStr, newSize);
	}

	inline bool operator==(const AString *other) const {
		return size == other->size && memcmp(data, other->data, size) == 0;
	}

	inline bool operator!=(const AString *other) const {
		return size != other->size || memcmp(data, other->data, size) != 0;
	}

	inline bool operator==(const AString &other) const {
		return size == other.size && memcmp(data, other.data, size) == 0;
	}

	inline bool operator!=(const AString &other) const {
		return size != other.size || memcmp(data, other.data, size) != 0;
	}

	template <typename T>
	inline static AString *plus(T value, AString *other) {
		std::string first = std::to_string(value);
		uint32_t newSize = static_cast<uint32_t>(first.size()) + other->size;
		char *newStr = new char[newSize + 1];
		memcpy(newStr, first.c_str(), first.size());
		memcpy(&newStr[first.size()], other->data, other->size);
		newStr[newSize] = '\0';
		return new AString(newStr, newSize);
	}

	inline static AString *plus(const char *value, AString *other) {
		std::string first = value;
		uint32_t newSize = static_cast<uint32_t>(first.size()) + other->size;
		char *newStr = new char[newSize + 1];
		memcpy(newStr, first.c_str(), first.size());
		memcpy(&newStr[first.size()], other->data, other->size);
		newStr[newSize] = '\0';
		return new AString(newStr, newSize);
	}

	~AString() {
		if (!(flags & FLAG_IS_REF)) {
			delete[] data;
		}
	}

	struct Hash {
		inline size_t operator()(const AString *s) const {
			size_t h = 0;
			for (size_t i = 0; i < s->size; ++i) {
				h = h * 31 + (unsigned char)s->data[i];
			}
			return h;
		}
	};

	struct Equal {
		inline bool operator()(const AString *a, const AString *b) const {
			return a->size == b->size && memcmp(a->data, b->data, a->size) == 0;
		}
	};
};

#endif