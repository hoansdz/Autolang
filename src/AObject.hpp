#ifndef AOBJECT_HPP
#define AOBJECT_HPP

#include <iostream>
#include "optimize/NormalArray.hpp"
#include "DefaultClass.hpp"
#include "AString.hpp"

class AVM;

struct AObject {
	uint32_t type;
	uint32_t refCount;
	union {
		long long i;
		double f;
		bool b;
		NormalArray<AObject*>* member;
		void* ref;
	};
	AObject() : refCount(0) {}
	AObject(uint32_t type):
		type(type), refCount(0){}
	AObject(uint32_t type, uint32_t memberCount):
		type(type), refCount(0), member(new NormalArray<AObject*>(memberCount)){}
	AObject(long long i):
		type(AutoLang::DefaultClass::INTCLASSID), refCount(0), i(i){}
	AObject(double f):
		type(AutoLang::DefaultClass::FLOATCLASSID), refCount(0), f(f){}
	AObject(AString* str):
		type(AutoLang::DefaultClass::stringClassId), refCount(0), ref(str){}
	inline void retain(){ ++refCount; };
	template<bool checkRefCount = false>
	inline void free() {
		switch (type) {
			case AutoLang::DefaultClass::INTCLASSID:
			case AutoLang::DefaultClass::FLOATCLASSID: {
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
		if (type == AutoLang::DefaultClass::stringClassId) {
			if constexpr (checkRefCount) {
				if (refCount > 0) --refCount;
				if (refCount != 0) return;
			}
			delete static_cast<AString*>(ref);
			return;
		}
		for (size_t i=0; i < member->size; ++i) { //Support delete data
			auto* obj = (*member)[i];
			if (!obj) continue;
			if (obj->refCount > 0) --obj->refCount;
			// obj->free();
		}
		delete member;
	}
};

#endif