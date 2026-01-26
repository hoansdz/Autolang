#ifndef ARRAY_HPP
#define ARRAY_HPP

#include "shared/AObject.hpp"

template <size_t size, bool callFree = true>
class Array {
public:
	size_t index = 0;
	AObject* objects[size];
	inline AObject*& top() {
		if (index == 0) 
			throw std::runtime_error("Floor");
		return objects[index - 1];
	}
	inline void push(AObject* object) {
		if (index == size) 
			throw std::runtime_error("Overflow");
		objects[index++] = object;
	}
	inline AObject* pop() {
		if (index == 0) 
			throw std::runtime_error("Floor");
		return objects[--index];
	}
	inline void addOrDelete(AreaAllocator& areaPool, AObject* obj) {
		if (index == size) {
			areaPool.release(obj);
			return;
		}
		objects[index++] = obj;
	}
	~Array() {
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