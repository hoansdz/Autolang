#ifndef STACKALLOCATOR_HPP
#define STACKALLOCATOR_HPP

#include <string>
#include <vector>
#include <cstring>
#include "shared/AObject.hpp"
#include "ObjectManager.hpp"

namespace AutoLang {

class AVM;

class StackAllocator {
private:
	size_t sizeNow;
public:
	size_t maxSize;
	size_t top;
	AObject** args;
	AObject** currentPtr;
	StackAllocator(size_t maxSize):
		maxSize(maxSize), sizeNow(maxSize), top(0), args(new AObject*[maxSize]()), currentPtr(args){}
	~StackAllocator() {
		// for (size_t i = 0; i < sizeNow; ++i) {
		// 	AObject* obj = args[i];
		// 	if (obj == nullptr) continue;
		// 	if (obj->refCount > 0) --obj->refCount;
		// 	if (obj->refCount != 0) continue;
		// 	obj->free();
		// 	delete obj;
		// }
		delete[] args;
	}

	inline void setTop(size_t top) {
		this->top = top;
		currentPtr = args + top;
	}

	inline size_t getTop() {
		return top;
	}
	
	inline void ensure(size_t size) {
		if (top + size <= sizeNow) return;
		size_t i = sizeNow;
		sizeNow = sizeNow + maxSize / 2;
		AObject** newArgs = new AObject*[sizeNow];
		memcpy(newArgs, args, sizeof(AObject*) * i);
		for (; i < sizeNow; ++i)
			newArgs[i] = nullptr;
		delete[] args;
		args = newArgs;
	}
	
	inline void freeTo(size_t top) {
		if (this->top > maxSize && top <= maxSize * 3 / 4) {
			AObject** newArgs = new AObject*[maxSize];
			memcpy(newArgs, args, sizeof(AObject*) * maxSize);
			sizeNow = maxSize;
			delete[] args;
			args = newArgs;
		}
		this->top = top;
		currentPtr = args + top;
	}
	
	inline void clear(ObjectManager& manager, int from, int to) {
		for (; from<=to; ++from) {
			AObject** obj = &args[from];
			if (*obj == nullptr) continue;
			manager.release(*obj);
			*obj = nullptr;
		}
	}
	
	template<size_t size>
	inline void clearTemp(ObjectManager& manager) {
		if constexpr (size > 0) {
			AObject** obj = &args[std::integral_constant<size_t, size - 1>{}];
			manager.release(*obj);
			*obj = nullptr;
			clearTemp<size - 1>(manager);
		}
	}
	
	inline void set(ObjectManager& manager, size_t index, AObject* object) {
		AObject** last = &args[top + index];
		if (*last != nullptr) {
			manager.release(*last);
		}
		*last = object;
	}
	
	inline AObject*& operator[](size_t index){ return currentPtr[index]; }
};

}

#endif