#ifndef FIXEDPOOL_HPP
#define FIXEDPOOL_HPP

#include <iostream>
#include <vector>
#include "shared/DefaultClass.hpp"
#include "shared/AString.hpp"

template<typename T>
class FixedPool {
public:
	T* objects = nullptr;
	uint32_t size = 0;
	uint32_t index = 0;
	FixedPool() = default;
	FixedPool(const FixedPool&) = delete;
	FixedPool& operator=(const FixedPool&) = delete;
	inline void allocate(uint32_t size) {
		if (objects) throw std::runtime_error("No reallocate");
		this->size = size;
		objects = static_cast<T*>(::operator new(sizeof(T) * size));
	}
	template<typename... Args>
	inline T* push(Args&&... args) {
		if (index == size) {
			assert(index == size && "HAS ERROR IN ESTIMATE");
		}
		T* newObj = &objects[index++];
		new (newObj) T(std::forward<Args>(args)...);
		return newObj;
	}
	inline T* operator[](uint32_t idx) {
		return &objects[idx];
	}
	inline void pop() {
		objects[index--].~T();
	}
	inline void refresh() {
		if (objects) {
			for (size_t i = 0; i < index; ++i)
				objects[i].~T();
			::operator delete(objects);
			objects = nullptr;
			index = 0;
		}
	}
	~FixedPool() {
		if (objects) {
			for (size_t i = 0; i < index; ++i)
				objects[i].~T();
			::operator delete(objects);
		}
	}
};

#endif