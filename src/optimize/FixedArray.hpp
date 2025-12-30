#ifndef FIXEDARRAY_HPP
#define FIXEDARRAY_HPP

#include <iostream>
#include <vector>
#include <cstring>
#include <type_traits>
#include <memory>
#include <new>
#include <cassert>

template<typename T>
struct FixedArray {
    std::unique_ptr<T[]> data;
    size_t size;

    FixedArray() : data(nullptr), size(0) {}

    // Constructor từ vector
    FixedArray(const std::vector<T>& vecs) : size(vecs.size()), data(std::make_unique<T[]>(vecs.size())) {
        if constexpr (std::is_trivial_v<T>) {
            memcpy(data.get(), vecs.data(), sizeof(T) * size);
        } else {
            for (size_t i = 0; i < size; ++i)
                new (&data[i]) T(vecs[i]);
        }
    }

    // Disable copy, enable move
    FixedArray(const FixedArray&) = delete;
    FixedArray& operator=(const FixedArray&) = delete;

    FixedArray(FixedArray&& other) noexcept : data(std::move(other.data)), size(other.size) {
        other.size = 0;
    }

    FixedArray& operator=(FixedArray&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            size = other.size;
            other.size = 0;
        }
        return *this;
    }

    void allocate(size_t newSize) {
        data = std::make_unique<T[]>(newSize);
        size = newSize;
    }

    void allocate(const std::vector<T>& vecs) {
        allocate(vecs.size());
        if constexpr (std::is_trivial_v<T>) {
            memcpy(data.get(), vecs.data(), sizeof(T) * size);
        } else {
            for (size_t i = 0; i < size; ++i)
                new (&data[i]) T(vecs[i]);
        }
    }

    T& operator[](size_t idx) { 
        return data[idx]; 
    }
    const T& operator[](size_t idx) const { 
        return data[idx]; 
    }
};

// Specialization cho bool
template<>
struct FixedArray<bool> {
    std::unique_ptr<bool[]> data;
    size_t size = 0;

    FixedArray() : data(nullptr), size(0) {}

    FixedArray(const std::vector<bool>& vecs) : data(std::make_unique<bool[]>(vecs.size())), size(vecs.size()) {
        for (size_t i = 0; i < size; ++i)
            data[i] = vecs[i];
    }

    // Disable copy, enable move
    FixedArray(const FixedArray&) = delete;
    FixedArray& operator=(const FixedArray&) = delete;

    FixedArray(FixedArray&& other) noexcept : data(std::move(other.data)), size(other.size) {
        other.size = 0;
    }

    FixedArray& operator=(FixedArray&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            size = other.size;
            other.size = 0;
        }
        return *this;
    }

    void allocate(const std::vector<bool>& vecs) {
        allocate(vecs.size());
        for (size_t i = 0; i < size; ++i)
            data[i] = vecs[i];
    }

    void allocate(size_t newSize) {
        data = std::make_unique<bool[]>(newSize);
        size = newSize;
    }

    bool& operator[](size_t idx) { 
        return data[idx]; 
    }
    const bool& operator[](size_t idx) const {
        return data[idx]; 
    }
};

#endif
