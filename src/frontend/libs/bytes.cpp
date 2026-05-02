#ifndef LIBS_BYTES_CPP
#define LIBS_BYTES_CPP

#include "frontend/ACompiler.hpp"
#include "backend/vm/ANotifier.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Type.hpp"
#include <cstring>
#include <string>

namespace AutoLang {
namespace Libs {
namespace bytes {

AObject *constructor(NativeFuncInData) {
    int64_t size = args[0]->i;
    AObject* obj = notifier.createBytes(size);
    return obj;
}

AObject *append(NativeFuncInData) {
    ABytes* b = args[0]->bytes; 
    uint8_t value = static_cast<uint8_t>(args[1]->i);
    
    if (b->size >= b->capacity) {
        b->capacity = b->capacity == 0 ? 16 : b->capacity * 2;
        uint8_t* newData = new uint8_t[b->capacity];
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
    ABytes* b = args[0]->bytes;
    int64_t index = args[1]->i;
    
    if (index < 0 || index >= b->size) {
        notifier.throwException("Bytes.get: Index out of bounds");
        return nullptr;
    }
    
    return notifier.createInt(b->data[index]);
}

AObject *set(NativeFuncInData) {
    ABytes* b = args[0]->bytes;
    int64_t index = args[1]->i;
    uint8_t value = static_cast<uint8_t>(args[2]->i);
    
    if (index < 0 || index >= b->size) {
        notifier.throwException("Bytes.set: Index out of bounds");
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
    ABytes* b = args[0]->bytes;
    int64_t from = args[1]->i;
    int64_t to = args[2]->i;

    if (from < 0) from = 0;
    if (to > b->size) to = b->size;
    if (from > to) from = to;

    int64_t newSize = to - from;

    AObject* newObj = notifier.createBytes(newSize);
    ABytes* newB = newObj->bytes;
    
    if (newSize > 0) {
        std::memcpy(newB->data, b->data + from, newSize);
        
        newB->size = newSize; 
    }
    
    return newObj;
}

AObject *to_string(NativeFuncInData) {
    ABytes* b = args[0]->bytes;
    
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

} // namespace bytes
} // namespace Libs
} // namespace AutoLang

#endif