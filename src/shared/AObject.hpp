#ifndef AOBJECT_HPP
#define AOBJECT_HPP

#include <iostream>
#include "shared/NormalArray.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/Type.hpp"
#include "AString.hpp"

namespace AutoLang {

class AVM;

struct AObject
{
	ClassId type;
	uint32_t refCount;
	union
	{
		int64_t i;
		double f;
		uint8_t b;
		NormalArray<AObject *> *member;
		AString* str;
	};
	AObject() : refCount(0) {}
	AObject(uint32_t type) : type(type), refCount(0) {}
	AObject(uint32_t type, uint32_t memberCount) : type(type), refCount(0), member(new NormalArray<AObject *>(memberCount)) {}
	AObject(int64_t i) : type(AutoLang::DefaultClass::intClassId), refCount(0), i(i) {}
	AObject(double f) : type(AutoLang::DefaultClass::floatClassId), refCount(0), f(f) {}
	AObject(AString *str) : type(AutoLang::DefaultClass::stringClassId), refCount(0), str(str) {}
	inline void retain() { ++refCount; };
	template <bool checkRefCount = false>
	inline void free()
	{
		switch (type)
		{
		case AutoLang::DefaultClass::intClassId:
		case AutoLang::DefaultClass::floatClassId:
		{
			return;
		}
		case AutoLang::DefaultClass::stringClassId:
		{
			if constexpr (checkRefCount)
			{
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
		for (size_t i = 0; i < member->size; ++i)
		{ // Support delete data
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

}

#endif