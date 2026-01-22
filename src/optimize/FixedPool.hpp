#ifndef FIXEDPOOL_HPP
#define FIXEDPOOL_HPP

#include <iostream>
#include <vector>
#include "DefaultClass.hpp"
#include "../vm/AString.hpp"

template<typename T>
class FixedPool {
public:
	T* objects = nullptr;
	size_t size = 0;
	size_t index = 0;
	FixedPool() = default;
	FixedPool(const FixedPool&) = delete;
	FixedPool& operator=(const FixedPool&) = delete;
	inline void allocate(size_t size) {
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
	inline T* operator[](size_t idx) {
		return &objects[idx];
	}
	inline void refresh() {
		if (objects) {
			for (size_t i = 0; i < index; ++i)
				objects[i].~T();
			::operator delete(objects);
			objects = nullptr;
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