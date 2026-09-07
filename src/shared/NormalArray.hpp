#ifndef NORMALARRAY_HPP
#define NORMALARRAY_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>

template <typename T> struct NormalArray {
	T *data;
	uint32_t size;

	explicit NormalArray(uint32_t initialCapacity)
	    : data(new T[initialCapacity]{}), size(initialCapacity) {}

	NormalArray(const NormalArray &) = delete;
	NormalArray &operator=(const NormalArray &) = delete;

	~NormalArray() { delete[] data; }

	inline T &operator[](uint32_t idx) {
		// assert(idx < size);
		return data[idx];
	}

	inline const T &operator[](uint32_t idx) const {
		assert(idx < size);
		return data[idx];
	}
};

#endif