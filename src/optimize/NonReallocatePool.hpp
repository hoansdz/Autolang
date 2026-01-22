#ifndef NONREALLOCATEPOOL_HPP
#define NONREALLOCATEPOOL_HPP

#include <iostream>
#include <vector>
#include "DefaultClass.hpp"
#include "../vm/AString.hpp"

class AVM;

template<typename T>
class NonReallocatePool {
public:
	T* objects = nullptr;
	std::vector<T*> vecs;
	size_t size = 0;
	size_t index = 0;
	NonReallocatePool() = default;
	NonReallocatePool(const NonReallocatePool&) = delete;
	NonReallocatePool& operator=(const NonReallocatePool&) = delete;
	inline void allocate(size_t size) {
		if (objects) throw std::runtime_error("No reallocate");
		this->size = size;
		objects = static_cast<T*>(::operator new(sizeof(T) * size));
	}
	template<typename... Args>
	inline T* push(Args&&... args) {
		if (index == size) {
			T* newObj = new T(std::forward<Args>(args)...);
			vecs.push_back(newObj);
			return newObj;
		}
		T* newObj = &objects[index++];
		new (newObj) T(std::forward<Args>(args)...);
		return newObj;
	}
	inline size_t getSize() {
		return index + vecs.size();
	}
	inline T* operator[](size_t idx) {
		return idx < size ? &objects[idx] : vecs[idx - size];
	}
	inline void refresh() {
		if (objects) {
			for (size_t i = 0; i < index; ++i)
				objects[i].~T();
			for (auto* object : vecs)
				delete object;
			::operator delete(objects);
			objects = nullptr;
		}
	}
	~NonReallocatePool() {
		if (objects) {
			for (size_t i = 0; i < index; ++i)
				objects[i].~T();
			for (auto* object : vecs)
				delete object;
			::operator delete(objects);
		}
	}
};

#endif