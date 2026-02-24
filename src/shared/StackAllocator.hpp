#ifndef STACKALLOCATOR_HPP
#define STACKALLOCATOR_HPP

#include "ObjectManager.hpp"
#include "shared/AObject.hpp"
#include <cstring>
#include <string>
#include <vector>


namespace AutoLang {

class AVM;

class StackAllocator {
  private:
	size_t sizeNow;

  public:
	size_t maxSize;
	size_t top;
	int peak;
	AObject **args;
	AObject **currentPtr;
	StackAllocator(size_t maxSize)
	    : maxSize(maxSize), sizeNow(maxSize), top(0), peak(0),
	      args(new AObject *[maxSize] {}), currentPtr(args) {}
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

	inline size_t getTop() { return top; }

	inline void ensure(size_t size) {
		if (top + size <= sizeNow)
			return;
		sizeNow *= 2;
		AObject **newArgs = new AObject *[sizeNow] {};
		memcpy(newArgs, args, sizeof(AObject *) * top);
		delete[] args;
		args = newArgs;
	}

	inline void freeTo(size_t top) {
		if (this->top > maxSize && top <= sizeNow / 2) {
			sizeNow /= 2;
			AObject **newArgs = new AObject *[sizeNow];
			memcpy(newArgs, args, sizeof(AObject *) * top);
			delete[] args;
			args = newArgs;
		}
		this->top = top;
		currentPtr = args + top;
	}

	inline void clear(ObjectManager &manager, int from, int to) {
		AObject **base = &args[from];
		int count = to - from + 1;
		int j = 0;

		// Trải vòng lặp 4 lần để giảm số lần kiểm tra điều kiện lặp j < count
		for (; j <= count - 4; j += 4) {
			if (base[j]) {
				manager.release(base[j]);
				base[j] = nullptr;
			}
			if (base[j + 1]) {
				manager.release(base[j + 1]);
				base[j + 1] = nullptr;
			}
			if (base[j + 2]) {
				manager.release(base[j + 2]);
				base[j + 2] = nullptr;
			}
			if (base[j + 3]) {
				manager.release(base[j + 3]);
				base[j + 3] = nullptr;
			}
		}

		// Xử lý nốt các phần tử dư
		for (; j < count; ++j) {
			if (base[j]) {
				manager.release(base[j]);
				base[j] = nullptr;
			}
		}
	}

	template <size_t size> inline void clearTemp(ObjectManager &manager) {
		if constexpr (size > 0) {
			AObject **obj = &args[std::integral_constant<size_t, size - 1>{}];
			manager.release(*obj);
			*obj = nullptr;
			clearTemp<size - 1>(manager);
		}
	}

	inline void set(ObjectManager &manager, size_t index, AObject *object) {
		AObject *&last = currentPtr[index];
		if (last != nullptr) {
			manager.release(last);
		}
		last = object;
	}

	inline AObject *&operator[](size_t index) { return currentPtr[index]; }
};

} // namespace AutoLang

#endif