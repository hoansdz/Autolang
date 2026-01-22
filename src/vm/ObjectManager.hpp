#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include "AreaAllocator.hpp"
#include "../optimize/Array.hpp"

class ObjectManager
{
private:
	static constexpr size_t size = 8;
	AreaAllocator areaPool;
	Array<size, false> intObjects;
	Array<size, false> floatObjects;
	inline void add(AObject *obj)
	{
		switch (obj->type)
		{
		case AutoLang::DefaultClass::intClassId:
			intObjects.addOrDelete(areaPool, obj);
			return;
		case AutoLang::DefaultClass::floatClassId:
			floatObjects.addOrDelete(areaPool, obj);
			return;
		default:
			areaPool.release(obj);
			return;
		}
	}
	inline AObject *get(int64_t i)
	{
		if (intObjects.index == 0)
		{
			AObject *obj = areaPool.getObject();
			obj->type = AutoLang::DefaultClass::intClassId;
			obj->i = i;
			return obj;
		}
		auto obj = intObjects.objects[--intObjects.index];
		obj->i = i;
		return obj;
	}
	inline AObject *get(double f)
	{
		if (floatObjects.index == 0)
		{
			AObject *obj = areaPool.getObject();
			obj->type = AutoLang::DefaultClass::floatClassId;
			obj->f = f;
			return obj;
		}
		auto obj = floatObjects.objects[--floatObjects.index];
		obj->f = f;
		return obj;
	}
	inline AObject *get(AString *str)
	{
		auto obj = areaPool.getObject();
		obj->type = AutoLang::DefaultClass::stringClassId;
		obj->str = str;
		return obj;
	}
	inline void freeObjectData(AObject *obj)
	{ // As free but it push again
		switch (obj->type)
		{
		case AutoLang::DefaultClass::intClassId:
		case AutoLang::DefaultClass::floatClassId:
		{
			return;
		}
		case AutoLang::DefaultClass::nullClassId:
		case AutoLang::DefaultClass::boolClassId:
		{
			assert("Critial Bug");
			return;
		}
		case AutoLang::DefaultClass::stringClassId:
		{
			delete obj->str;
			return;
		}
		default:
			break;
		}
		for (int i = 0; i < obj->member->size; ++i)
		{
			auto *mem = (*obj->member)[i];
			if (!mem)
				continue;
			release(mem);
		}
		delete obj->member;
	}

public:
	ObjectManager()
	{
	}
	inline void release(AObject *obj)
	{
		if (obj->refCount > 0)
			--obj->refCount;
		if (obj->refCount != 0)
			return;
		freeObjectData(obj);
		add(obj);
	}
	static inline AObject *create(bool b) {
		return b ? AutoLang::DefaultClass::trueObject : AutoLang::DefaultClass::falseObject;
	}
	static inline AObject *createBoolObject(bool b) { return create(b); }
	inline AObject *get(uint32_t type, size_t memberCount)
	{
		AObject *obj = areaPool.getObject();
		obj->type = type;
		obj->member = new NormalArray<AObject *>(memberCount);
		return obj;
	}
	inline AObject *create(int64_t i) { return get(i); }
	inline AObject *create(double f) { return get(f); }
	inline AObject *create(AString *str) { return get(str); }
	inline AObject *createIntObject(int64_t i) { return get(i); }
	inline AObject *createFloatObject(double f) { return get(f); }
	inline AObject *createStringObject(AString *str) { return get(str); }
	inline AObject *createString(int64_t i) { return get(AString::from(i)); }
	inline AObject *createString(double f) { return get(AString::from(f)); }
	void destroy();
};

#endif