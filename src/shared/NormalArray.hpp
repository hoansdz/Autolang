#ifndef NORMALARRAY_HPP
#define NORMALARRAY_HPP

#include <memory>
#include <cassert>

template<typename T>
struct NormalArray {
    T* data;
    size_t size;

    explicit NormalArray(size_t size) : data(new T[size]{}), size(size) {}

    inline T& operator[](size_t idx) {
        return data[idx]; 
    }
    ~NormalArray() {
        delete[] data;
    }
};

#endif