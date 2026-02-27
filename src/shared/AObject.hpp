#ifndef AOBJECT_HPP
#define AOBJECT_HPP

#include "AString.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/NormalArray.hpp"
#include "shared/Type.hpp"
#include <iostream>

namespace AutoLang {

class AVM;

using DestructorParameters = void (*)(ANotifier &notifier, void* data);

struct ANativeData {
	void* data;
	DestructorParameters destructor;
};

struct AObject {
	enum Flags : uint32_t { OBJ_IS_FREE = 1u << 0, OBJ_IS_CONST = 1u << 1, OBJ_IS_NATIVE_DATA = 1u << 2 };
	ClassId type;
	uint32_t refCount;
	uint32_t flags = 0;
	union {
		int64_t i;
		double f;
		uint8_t b;
		NormalArray<AObject *> *member;
		AString *str;
		ANativeData *data;
	};
	AObject() : type(0), refCount(0) {}
	AObject(uint32_t type) : type(type), refCount(0) {}
	AObject(uint32_t type, uint32_t memberCount)
	    : type(type), refCount(0),
	      member(new NormalArray<AObject *>(memberCount)) {}
	AObject(int64_t i)
	    : type(AutoLang::DefaultClass::intClassId), refCount(0), i(i) {}
	AObject(double f)
	    : type(AutoLang::DefaultClass::floatClassId), refCount(0), f(f) {}
	AObject(AString *str)
	    : type(AutoLang::DefaultClass::stringClassId), refCount(0), str(str) {}
	AObject(uint32_t type, ANativeData *data)
	    : type(type), refCount(0), data(data) {}
	inline void retain() {
		if (flags & AObject::Flags::OBJ_IS_CONST)
			return;
		++refCount;
	};
	template <bool checkRefCount = false> inline void free(ANotifier& notifier) {
		switch (type) {
			case AutoLang::DefaultClass::intClassId:
			case AutoLang::DefaultClass::floatClassId:
			case AutoLang::DefaultClass::boolClassId:
			case AutoLang::DefaultClass::nullClassId: {
				return;
			}
			case AutoLang::DefaultClass::stringClassId: {
				if constexpr (checkRefCount) {
					if (refCount > 0)
						--refCount;
					if (refCount != 0)
						return;
				}
				delete str;
				return;
			}
			default:
				break;
		}
		// if (type == AutoLang::DefaultClass::nullClassId ||
		// 	type == AutoLang::DefaultClass::boolClassId) {
		// 	assert("what wrong");
		// 	return;
		// }
		if (flags & Flags::OBJ_IS_NATIVE_DATA) {
			if (data->destructor) {
				data->destructor(notifier, data->data);
			}
			delete data;
			return;
		}
		for (size_t i = 0; i < member->size; ++i) { // Support delete data
			auto *obj = (*member)[i];
			if (!obj)
				continue;
			if (obj->refCount > 0)
				--obj->refCount;
			// obj->free();
		}
		delete member;
	}
};

} // namespace AutoLang

#endif