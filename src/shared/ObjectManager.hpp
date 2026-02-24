#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include <iostream>
#include "backend/optimize/Stack.hpp"
#include "shared/AreaAllocator.hpp"

namespace AutoLang {

class ObjectManager {
  private:
	static constexpr uint32_t size = 8;
	AreaAllocator<128> areaAllocator;
	Stack<AObject*, size> intObjects;
	Stack<AObject*, size> floatObjects;
	inline void add(AObject *obj) {
		switch (obj->type) {
			case AutoLang::DefaultClass::intClassId: {
				if (intObjects.index == size) {
					areaAllocator.release(obj);
					return;
				}
				intObjects.objects[intObjects.index++] = obj;
				return;
			}
			case AutoLang::DefaultClass::floatClassId:{
				if (floatObjects.index == size) {
					areaAllocator.release(obj);
					return;
				}
				floatObjects.objects[floatObjects.index++] = obj;
				return;
			}
			default: {
				areaAllocator.release(obj);
				return;
			}
		}
	}
	inline AObject *get(int64_t i) {
		if (intObjects.index == 0) {
			AObject *obj = areaAllocator.getObject();
			obj->type = AutoLang::DefaultClass::intClassId;
			obj->i = i;
			return obj;
		}
		auto obj = intObjects.objects[--intObjects.index];
		obj->i = i;
		return obj;
	}
	inline AObject *get(double f) {
		if (floatObjects.index == 0) {
			AObject *obj = areaAllocator.getObject();
			obj->type = AutoLang::DefaultClass::floatClassId;
			obj->f = f;
			return obj;
		}
		auto obj = floatObjects.objects[--floatObjects.index];
		obj->f = f;
		return obj;
	}
	inline AObject *get(AString *str) {
		auto obj = areaAllocator.getObject();
		obj->type = AutoLang::DefaultClass::stringClassId;
		obj->str = str;
		return obj;
	}
	inline void freeObjectData(AObject *obj) { // As free but it push again
		switch (obj->type) {
			case AutoLang::DefaultClass::intClassId:
			case AutoLang::DefaultClass::floatClassId: {
				return;
			}
			case AutoLang::DefaultClass::nullClassId:
			case AutoLang::DefaultClass::boolClassId: {
				assert(false && "Critical Bug: free bool/null object");
				return;
			}
			case AutoLang::DefaultClass::stringClassId: {
				delete obj->str;
				return;
			}
			default:
				break;
		}
		for (int i = 0; i < obj->member->size; ++i) {
			auto *mem = (*obj->member)[i];
			if (!mem)
				continue;
			release(mem);
		}
		delete obj->member;
	}

  public:
	ObjectManager() {}
	inline void release(AObject *obj) {
		if (obj->flags & AObject::Flags::OBJ_IS_CONST)
			return;
		if (obj->refCount > 0)
			--obj->refCount;
		if (obj->refCount != 0)
			return;
		freeObjectData(obj);
		add(obj);
	}
	inline void tryRelease(AObject *obj) {
		if (obj->refCount != 0)
			return;
		freeObjectData(obj);
		add(obj);
	}
	inline void refresh() {
		areaAllocator.destroy();
		intObjects.refresh();
		floatObjects.refresh();
	}
	static inline AObject *create(bool b) {
		return b ? AutoLang::DefaultClass::trueObject
		         : AutoLang::DefaultClass::falseObject;
	}
	static inline AObject *createBoolObject(bool b) { return create(b); }
	inline AObject *get(uint32_t type, size_t memberCount) {
		AObject *obj = areaAllocator.getObject();
		obj->type = type;
		obj->member = new NormalArray<AObject *>(memberCount);
		return obj;
	}
	inline AObject *createEmptyObject() { return areaAllocator.getObject(); }
	inline AObject *create(int64_t i) { return get(i); }
	inline AObject *create(double f) { return get(f); }
	inline AObject *create(AString *str) { return get(str); }
	inline AObject *createIntObject(int64_t i) { return get(i); }
	inline AObject *createFloatObject(double f) { return get(f); }
	inline AObject *createStringObject(AString *str) { return get(str); }
	inline AObject *createString(int64_t i) { return get(AString::from(i)); }
	inline AObject *createString(double f) { return get(AString::from(f)); }
	inline AObject *createString(const char* str) { return get(AString::from(str)); }
	inline AObject *createString(std::string str) { return get(AString::from(str)); }
	void destroy();
};

}

#endif