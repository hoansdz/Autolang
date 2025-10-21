#ifndef ASTRING_HPP
#define ASTRING_HPP

#include <cstring>
#include <iostream>

class AString {
public:
    char* data;
    size_t size;

    AString(char* data, size_t size): data(data), size(size){}

    static AString* from(const std::string& value) {
        char* str = new char[value.size() + 1];
        memcpy(str, value.c_str(), value.size());
        str[value.size()] = '\0';
        return new AString(str, value.size());
    }

    template<typename T>
    static AString* from(T value) {
        std::string val = std::to_string(value);
        char* str = new char[val.size() + 1];
        memcpy(str, val.c_str(), val.size());
        str[val.size()] = '\0';
        return new AString(str, val.size());
    }

    static AString* copy(AString* other) {
        char* newStr = new char[other->size + 1];
        memcpy(newStr, other->data, other->size + 1);
        return new AString(newStr, other->size);
    }

    AString* operator+(AString* other) {
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

    inline bool operator==(AString* other) {
        return size == other->size && memcmp(data, other->data, size) == 0;
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

    ~AString() {
        delete[] data;
    }
};

#endif