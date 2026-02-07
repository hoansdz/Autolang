#ifndef FIXEDPOOL_LOADED_HPP
#define FIXEDPOOL_LOADED_HPP

#include <iostream>
#include <vector>
#include "shared/DefaultClass.hpp"
#include "shared/AString.hpp"

template<typename T, size_t size>
class FixedPoolLoaded {
public:
	T objects[size];
	uint32_t index = 0;
	FixedPoolLoaded() = default;
	FixedPoolLoaded(const FixedPoolLoaded&) = delete;
	FixedPoolLoaded& operator=(const FixedPoolLoaded&) = delete;
	inline T* push() {
		if (index == size) {
			assert(index == size && "HAS ERROR IN ESTIMATE");
		}
		return &objects[index++];
	}
	inline T* operator[](uint32_t idx) {
		return &objects[idx];
	}
	inline T* top() {
		return &objects[index - 1];
	}
	inline T* pop() {
		return &objects[--index];
	}
	inline uint32_t getSize() {
		return index;
	}
	inline uint32_t getMaxSize() {
		return size;
	}
};

#endif