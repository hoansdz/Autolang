#ifndef STACK_HPP
#define STACK_HPP

#include "shared/AObject.hpp"

template <typename T, uint32_t size>
class Stack {
public:
	uint32_t index = 0;
	T objects[size];
	inline T& top() {
		if (index == 0) 
			throw std::runtime_error("Floor");
		return objects[index - 1];
	}
	inline void push(T object) {
		if (index == size) 
			throw std::runtime_error("Overflow");
		objects[index++] = object;
	}
	inline T pop() {
		if (index == 0) 
			throw std::runtime_error("Floor");
		return objects[--index];
	}
	~Stack() {
		// for (size_t i = 0; i < index; ++i) {
		// 	AObject* obj = objects[i];
		// 	if (obj->refCount > 0) --obj->refCount;
		// 	if (obj->refCount != 0) continue;
		// 	if constexpr (callFree) obj->free();
		// 	delete obj;
		// }
	}
};

#endif