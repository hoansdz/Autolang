#ifndef AOBJECT_HPP
#define AOBJECT_HPP

#include <iostream>
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
		AObject** member;
		void* ref;
	};
	AObject(uint32_t type):
		type(type), refCount(0){}
	AObject(uint32_t type, uint32_t memberCount):
		type(type), refCount(0), member(new AObject*[memberCount]()){}
	AObject(long long i):
		type(AutoLang::DefaultClass::INTCLASSID), refCount(0), i(i){}
	AObject(double f):
		type(AutoLang::DefaultClass::FLOATCLASSID), refCount(0), f(f){}
	AObject(AString* str):
		type(AutoLang::DefaultClass::stringClassId), refCount(0), ref(str){}
	inline void retain(){ ++refCount; };
	inline void free() {
		switch (type) {
			case AutoLang::DefaultClass::INTCLASSID:
			case AutoLang::DefaultClass::FLOATCLASSID: {
				return;
			}
			default:
				break;
		}
		if (!ref) return;
		if (type == AutoLang::DefaultClass::stringClassId) {
			delete static_cast<AString*>(ref);
			return;
		}
		delete[] member;
		//for (size_t i = 0; i<)
	}
};

template <size_t size>
class Array {
public:
	size_t index = 0;
	AObject* objects[size];
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
	inline void addOrDelete(AObject* obj) {
		if (index == size) {
			delete obj;
			return;
		}
		objects[index++] = obj;
	}
	~Array() {
		for (size_t i = 0; i < index; ++i)
			delete objects[i];
	}
};

class ObjectManager {
private:
	static constexpr size_t size = 8;
	Array<size> intObjects;
	Array<size> floatObjects;
	Array<size> otherObjects;
	inline void add(AObject* obj) {
		switch (obj->type) {
			case AutoLang::DefaultClass::INTCLASSID:
				intObjects.addOrDelete(obj);
				return;
			case AutoLang::DefaultClass::FLOATCLASSID:
				floatObjects.addOrDelete(obj);
				return;
			default:
				otherObjects.addOrDelete(obj);
				return;
		}
	}
	inline AObject* get(long long i) {
		if (intObjects.index == 0)
			return new AObject(i);
		auto obj = intObjects.objects[--intObjects.index];
		obj->i = i;
		return obj;
	}
	inline AObject* get(double f) {
		if (floatObjects.index == 0)
			return new AObject(f);
		auto obj = floatObjects.objects[--floatObjects.index];
		obj->f = f;
		return obj;
	}
	inline AObject* get(AString* str) {
		if (otherObjects.index == 0)
			return new AObject(str);
		auto obj = otherObjects.objects[--otherObjects.index];
		obj->type = AutoLang::DefaultClass::stringClassId;
		obj->ref = str;
		return obj;
	}
public:
	ObjectManager()
		{}
	inline void release(AObject* obj) {
		if (obj->refCount > 0) --obj->refCount;
		if (obj->refCount != 0) return;
		obj->free();
		add(obj);
	}
	static inline AObject* create(bool b) { 
		return b ? AutoLang::DefaultClass::trueObject : 
						AutoLang::DefaultClass::falseObject; 
	}
	inline AObject* create(long long i){ return get(i); }
	inline AObject* create(double f){ return get(f); }
	inline AObject* create(AString* str){ return get(str); }
	inline AObject* createString(long long i){ return get(AString::from(i)); }
	inline AObject* createString(double f){ return get(AString::from(f)); }
	//~ObjectManager();
};

#endif