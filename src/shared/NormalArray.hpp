#ifndef NORMALARRAY_HPP
#define NORMALARRAY_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>


template <typename T> struct NormalArray {
	T *data;
	uint32_t size;
	uint32_t maxSize;

	explicit NormalArray(uint32_t initialCapacity)
	    : data(new T[initialCapacity]{}), size(0), maxSize(initialCapacity) {}

	// Rule of Three: Chống copy bậy bạ gây lỗi double free
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

	inline void reallocate(uint32_t newCapacity) {

		T *newData = new T[newCapacity]{};
		std::copy(data, data + size, newData);

		delete[] data;
		data = newData;
		maxSize = newCapacity;
	}
};

#endif