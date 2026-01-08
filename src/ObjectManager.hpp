#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include "AreaPool.hpp"
#include "optimize/Array.hpp"

class ObjectManager
{
private:
	static constexpr size_t size = 8;
	AreaPool areaPool;
	Array<size, false> intObjects;
	Array<size, false> floatObjects;
	inline void add(AObject *obj)
	{
		switch (obj->type)
		{
		case AutoLang::DefaultClass::INTCLASSID:
			intObjects.addOrDelete(areaPool, obj);
			return;
		case AutoLang::DefaultClass::FLOATCLASSID:
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
			obj->type = AutoLang::DefaultClass::INTCLASSID;
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
			obj->type = AutoLang::DefaultClass::FLOATCLASSID;
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
		obj->ref = str;
		return obj;
	}
	inline void freeObjectData(AObject *obj)
	{ // As free but it push again
		switch (obj->type)
		{
		case AutoLang::DefaultClass::INTCLASSID:
		case AutoLang::DefaultClass::FLOATCLASSID:
		{
			return;
		}
		default:
			break;
		}
		if (obj->type == AutoLang::DefaultClass::nullClassId ||
			obj->type == AutoLang::DefaultClass::boolClassId)
		{
			assert("what wrong");
			return;
		}
		if (obj->type == AutoLang::DefaultClass::stringClassId)
		{
			delete static_cast<AString *>(obj->ref);
			return;
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
	~ObjectManager();
};

#endif