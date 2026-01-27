#ifndef FIXEDARRAY_HPP
#define FIXEDARRAY_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <cassert>

template<typename T>
struct FixedArray {
    std::unique_ptr<T[]> data;
    size_t size;

    FixedArray() : data(nullptr), size(0) {}

    // Constructor từ vector
    FixedArray(const std::vector<T>& vecs) : size(vecs.size()) {
        if (size > 0) {
            // make_unique đã khởi tạo mặc định các phần tử
            data = std::make_unique<T[]>(size);
            // std::copy tự động xử lý việc gán giá trị an toàn
            // Nó cũng tự động tối ưu thành memcpy nếu T là kiểu đơn giản (trivial)
            // Và nó tự động xử lý luôn vector<bool> mà không cần specialization
            std::copy(vecs.begin(), vecs.end(), data.get());
        }
    }

    // Disable copy
    FixedArray(const FixedArray&) = delete;
    FixedArray& operator=(const FixedArray&) = delete;

    // Move constructor
    FixedArray(FixedArray&& other) noexcept : data(std::move(other.data)), size(other.size) {
        other.size = 0;
    }

    // Move assignment
    FixedArray& operator=(FixedArray&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            size = other.size;
            other.size = 0;
        }
        return *this;
    }

    // Allocate theo kích thước mới
    void allocate(size_t newSize) {
        size = newSize;
        if (size > 0) {
            data = std::make_unique<T[]>(size);
        } else {
            data.reset();
        }
    }

    // Allocate từ vector
    void allocate(const std::vector<T>& vecs) {
        allocate(vecs.size());
        if (size > 0) {
            std::copy(vecs.begin(), vecs.end(), data.get());
        }
    }

    T& operator[](size_t idx) { 
        //assert(idx >= size);
        return data[idx]; 
    }
    const T& operator[](size_t idx) const { 
        assert(idx >= size);
        return data[idx]; 
    }
    
    // Thêm begin/end để hỗ trợ duyệt mảng (optional nhưng rất nên có)
    T* begin() { return data.get(); }
    T* end() { return data.get() + size; }
    const T* begin() const { return data.get(); }
    const T* end() const { return data.get() + size; }
};

// Đã xóa specialization FixedArray<bool> vì template ở trên
// đã xử lý được bool một cách hoàn hảo nhờ std::copy.

#endif