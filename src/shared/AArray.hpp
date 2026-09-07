#ifndef AARRAY_HPP
#define AARRAY_HPP

#include "shared/DefaultClass.hpp"
#include "shared/Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace Autolang {

struct AObject;

struct AArray {
	ClassId key;
	uint32_t size;
	uint32_t maxSize;
	union {
		int64_t *intData;
		double *floatData;
		AObject **objData;
		void *raw;
	};

	explicit AArray(ClassId key, uint32_t initialCapacity = 0)
	    : key(key), size(initialCapacity), maxSize(initialCapacity) {
		if (initialCapacity > 0) {
			if (key == DefaultClass::intClassId) {
				intData = new int64_t[initialCapacity]{};
			} else if (key == DefaultClass::floatClassId) {
				floatData = new double[initialCapacity]{};
			} else {
				objData = new AObject *[initialCapacity]{};
			}
		} else {
			raw = nullptr;
		}
	}

	AArray(const AArray &) = delete;
	AArray &operator=(const AArray &) = delete;

	~AArray() {
		if (raw) {
			if (key == DefaultClass::intClassId) {
				delete[] intData;
			} else if (key == DefaultClass::floatClassId) {
				delete[] floatData;
			} else {
				delete[] objData;
			}
		}
	}

	void reallocate(uint32_t newCapacity) {
		if (key == DefaultClass::intClassId) {
			int64_t *newData = new int64_t[newCapacity]{};
			if (intData && size > 0) {
				std::copy(intData, intData + std::min(size, newCapacity),
				          newData);
			}
			delete[] intData;
			intData = newData;
		} else if (key == DefaultClass::floatClassId) {
			double *newData = new double[newCapacity]{};
			if (floatData && size > 0) {
				std::copy(floatData, floatData + std::min(size, newCapacity),
				          newData);
			}
			delete[] floatData;
			floatData = newData;
		} else {
			AObject **newData = new AObject *[newCapacity]{};
			if (objData && size > 0) {
				std::copy(objData, objData + std::min(size, newCapacity),
				          newData);
			}
			delete[] objData;
			objData = newData;
		}
		maxSize = newCapacity;
	}
};

} // namespace Autolang

#endif
