#ifndef ASTRING_HPP
#define ASTRING_HPP

#include <cstring>
#include <iostream>

class AString {
public:
    char* data;
    size_t size;

    AString(char* data, size_t size): data(data), size(size){}

	inline static AString* from(const char* value) {
		size_t size = strlen(value);
        char* str = new char[size + 1];
		strcpy(str, value);
        return new AString(str, size);
    }

    inline static AString* from(const std::string& value) {
        char* str = new char[value.size() + 1];
        memcpy(str, value.c_str(), value.size());
        str[value.size()] = '\0';
        return new AString(str, value.size());
    }

	inline static AString* from(char chr) {
        char* str = new char[2];
        str[0] = chr;
        str[1] = '\0';
        return new AString(str, 2);
    }

    template<typename T>
    inline static AString* from(T value) {
        std::string val = std::to_string(value);
        char* str = new char[val.size() + 1];
        memcpy(str, val.c_str(), val.size());
        str[val.size()] = '\0';
        return new AString(str, val.size());
    }

    inline static AString* copy(AString* other) {
        char* newStr = new char[other->size + 1];
        memcpy(newStr, other->data, other->size + 1);
        return new AString(newStr, other->size);
    }

    inline AString* operator+(AString* other) {
        size_t newSize = size + other->size;
        char* newStr = new char[newSize + 1];
        memcpy(newStr, data, size);
        memcpy(newStr + size, other->data, other->size);
        newStr[newSize] = '\0';
        return new AString(newStr, newSize);
    }

    template<typename T>
    inline AString* operator+(T value) {
        std::string other = std::to_string(value);
        size_t newSize = size + other.size();
        char* newStr = new char[newSize + 1];
        memcpy(newStr, data, size);
        memcpy(newStr + size, other.c_str(), other.size());
        newStr[newSize] = '\0';
        return new AString(newStr, newSize);
    }

	inline AString* operator+(const char* value) {
        std::string other = value;
        size_t newSize = size + other.size();
        char* newStr = new char[newSize + 1];
        memcpy(newStr, data, size);
        memcpy(newStr + size, other.c_str(), other.size());
        newStr[newSize] = '\0';
        return new AString(newStr, newSize);
    }

    inline bool operator ==(const AString* other) const {
        return size == other->size && memcmp(data, other->data, size) == 0;
    }

    inline bool operator !=(const AString* other) const {
        return size != other->size || memcmp(data, other->data, size) != 0;
    }

    template<typename T>
    inline static AString* plus(T value, AString* other) {
        std::string first = std::to_string(value);
        size_t newSize = first.size() + other->size;
        char* newStr = new char[newSize + 1];
        memcpy(newStr, first.c_str(), first.size());
        memcpy(&newStr[first.size()], other->data, other->size);
        newStr[newSize] = '\0';
        return new AString(newStr, newSize);
    }

	inline static AString* plus(const char* value, AString* other) {
        std::string first = value;
        size_t newSize = first.size() + other->size;
        char* newStr = new char[newSize + 1];
        memcpy(newStr, first.c_str(), first.size());
        memcpy(&newStr[first.size()], other->data, other->size);
        newStr[newSize] = '\0';
        return new AString(newStr, newSize);
    }

    ~AString() {
        delete[] data;
    }

    struct Hash {
        inline size_t operator()(const AString* s) const {
            size_t h = 0;
            for (size_t i = 0; i < s->size; ++i) {
                h = h * 31 + (unsigned char)s->data[i];
            }
            return h;
        }
    };

    struct Equal {
        inline bool operator()(const AString* a, const AString* b) const {
            return a->size == b->size && memcmp(a->data, b->data, a->size) == 0;
        }
    };
};

#endif