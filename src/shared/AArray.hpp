#ifndef AARRAY_HPP
#define AARRAY_HPP

#include "shared/DefaultClass.hpp"
#include "shared/Type.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

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
			switch (key) {
				case DefaultClass::intClassId:
					intData = new int64_t[initialCapacity]{};
					break;
				case DefaultClass::floatClassId:
					floatData = new double[initialCapacity]{};
					break;
				default:
					objData = new AObject *[initialCapacity]{};
					break;
			}
		} else {
			raw = nullptr;
		}
	}

	AArray(const AArray &) = delete;
	AArray &operator=(const AArray &) = delete;

	~AArray() {
		if (raw) {
			switch (key) {
				case DefaultClass::intClassId:
					delete[] intData;
					break;
				case DefaultClass::floatClassId:
					delete[] floatData;
					break;
				default:
					delete[] objData;
					break;
			}
		}
	}

	void reallocate(uint32_t newCapacity) {
		switch (key) {
			case DefaultClass::intClassId: {
				int64_t *newData = new int64_t[newCapacity]{};
				if (intData && size > 0) {
					std::copy(intData, intData + std::min(size, newCapacity),
					          newData);
				}
				delete[] intData;
				intData = newData;
				break;
			}
			case DefaultClass::floatClassId: {
				double *newData = new double[newCapacity]{};
				if (floatData && size > 0) {
					std::copy(floatData, floatData + std::min(size, newCapacity),
					          newData);
				}
				delete[] floatData;
				floatData = newData;
				break;
			}
			default: {
				AObject **newData = new AObject *[newCapacity]{};
				if (objData && size > 0) {
					std::copy(objData, objData + std::min(size, newCapacity),
					          newData);
				}
				delete[] objData;
				objData = newData;
				break;
			}
		}
		maxSize = newCapacity;
	}
};

} // namespace Autolang

#endif
