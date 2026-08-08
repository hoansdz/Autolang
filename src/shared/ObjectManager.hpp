#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include "backend/optimize/Stack.hpp"
#include "shared/AreaAllocator.hpp"
#include <iostream>

namespace Autolang {

class ObjectManager {
  private:
	friend AVM;
	ANotifier *notifier;

  public:
	static constexpr uint32_t size = 8;
	AreaAllocator<128> areaAllocator;
	Stack<AObject *, size> intObjects;
	Stack<AObject *, size> floatObjects;
	inline void add(AObject *obj) {
		switch (obj->type) {
			case Autolang::DefaultClass::intClassId: {
				// std::cerr<<"released " << obj << "\n";
				if (intObjects.index == size) {
					areaAllocator.release(obj);
					return;
				}
				intObjects.objects[intObjects.index++] = obj;
				return;
			}
			case Autolang::DefaultClass::floatClassId: {
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
	inline AObject *getEmptyObject() { return areaAllocator.getObject(); }
	inline AObject *getBytes(uint32_t size) {
		auto obj = areaAllocator.getObject();
		obj->type = DefaultClass::bytesClassId;
		obj->bytes = new ABytes(size, size, new uint8_t[size]);
		areaAllocator.addManagedMemory(sizeof(ABytes) + size);
		return obj;
	}
#ifdef __EMSCRIPTEN__
	inline AObject *getJsObject(ClassId classId, emscripten::val *jsObject) {
		auto obj = areaAllocator.getObject();
		obj->type = classId;
		obj->jsObject = jsObject;
		obj->flags = AObject::Flags::OBJ_IS_JS_OBJECT;
		return obj;
	}
#elif __PYBIND11__
	inline AObject *getPyObject(ClassId classId, pybind11::object *pyObject) {
		auto obj = areaAllocator.getObject();
		obj->type = classId;
		obj->pyObject = pyObject;
		obj->flags = AObject::Flags::OBJ_IS_PY_OBJECT;
		return obj;
	}
#endif
	inline AObject *get(int64_t i) {
		if (intObjects.index == 0) {
			AObject *obj = areaAllocator.getObject();
			obj->type = Autolang::DefaultClass::intClassId;
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
			obj->type = Autolang::DefaultClass::floatClassId;
			obj->f = f;
			return obj;
		}
		auto obj = floatObjects.objects[--floatObjects.index];
		obj->f = f;
		return obj;
	}
	inline AObject *get(AString *str) {
		auto obj = areaAllocator.getObject();
		obj->type = Autolang::DefaultClass::stringClassId;
		obj->str = str;
		if (str) {
			areaAllocator.addManagedMemory(sizeof(AString) + str->size);
		}
		return obj;
	}
	inline AObject *get(FunctionObject *function) {
		auto obj = areaAllocator.getObject();
		obj->type = Autolang::DefaultClass::functionClassId;
		obj->function = function;
		return obj;
	}
	inline AObject *get(ClassId classId, ANativeData *nativeData) {
		auto obj = areaAllocator.getObject();
		obj->type = classId;
		obj->data = nativeData;
		obj->flags = AObject::Flags::OBJ_IS_NATIVE_DATA;
		return obj;
	}
	inline void freeObjectData(AObject *obj) { // As free but it push again
		if (obj->flags == AObject::Flags::OBJ_IS_FREE) {
			return;
		}
		switch (obj->type) {
			case Autolang::DefaultClass::intClassId:
			case Autolang::DefaultClass::floatClassId: {
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
			case Autolang::DefaultClass::nullClassId:
			case Autolang::DefaultClass::boolClassId: {
				int *a = nullptr;
				*a = 5;
				assert(false && "Critical Bug: free bool/null object");
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
			case Autolang::DefaultClass::stringClassId: {
				if (obj->str) {
					areaAllocator.addManagedMemory(-static_cast<int64_t>(sizeof(AString) + obj->str->size));
				}
				delete obj->str;
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
#ifndef NO_INCLUDE_LIBS_JSON
			case Autolang::DefaultClass::jsonClassId: {
				if (obj->json) {
					areaAllocator.addManagedMemory(-128);
					delete obj->json;
				}
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
#endif
#ifdef __EMSCRIPTEN__
			case Autolang::DefaultClass::jsObjectClassId: {
				delete obj->jsObject;
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
#elif __PYBIND11__
			case Autolang::DefaultClass::pyObjectClassId: {
				delete obj->pyObject;
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
#endif
			case Autolang::DefaultClass::bytesClassId: {
				if (obj->bytes) {
					areaAllocator.addManagedMemory(-static_cast<int64_t>(sizeof(ABytes) + obj->bytes->capacity));
				}
				delete[] obj->bytes->data;
				delete obj->bytes;
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
			case Autolang::DefaultClass::functionClassId: {
				for (int i = obj->function->size; i-- > 0;) {
					release(obj->function->args[i]);
				}
				delete obj->function;
				obj->flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
			default:
				break;
		}
#ifdef __EMSCRIPTEN__
		if (obj->flags & AObject::Flags::OBJ_IS_JS_OBJECT) {
			delete obj->jsObject;
			obj->flags = AObject::Flags::OBJ_IS_FREE;
			return;
		}
#endif
		if (obj->flags & AObject::Flags::OBJ_IS_NATIVE_DATA) {
			if (obj->data->destructor) {
				obj->data->destructor(*notifier, obj->data->data);
			}
			delete obj->data;
			obj->flags = AObject::Flags::OBJ_IS_FREE;
			return;
		}
		for (uint32_t i = 0; i < obj->member->size; ++i) {
			auto *mem = (*obj->member)[i];
			if (!mem)
				continue;
			release(mem);
		}
		areaAllocator.addManagedMemory(-static_cast<int64_t>(sizeof(NormalArray<AObject *>) + obj->member->maxSize * sizeof(AObject *)));
		delete obj->member;
		obj->flags = AObject::Flags::OBJ_IS_FREE;
	}

  public:
	ObjectManager() {}
	inline void release(AObject *obj) {
		if (obj->flags & AObject::Flags::OBJ_IS_CONST)
			return;
		if (obj->refCount > 1) {
			--obj->refCount;
			return;
		} else {
			obj->refCount = 0;
		}
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
		areaAllocator.destroy(*notifier);
		intObjects.refresh();
		floatObjects.refresh();
	}
	static inline AObject *create(bool b) {
		return b ? Autolang::DefaultClass::trueObject
		         : Autolang::DefaultClass::falseObject;
	}
	static inline AObject *createBoolObject(bool b) { return create(b); }
	inline AObject *get(uint32_t type, size_t memberCount) {
		AObject *obj = areaAllocator.getObject();
		obj->type = type;
		obj->member = new NormalArray<AObject *>(memberCount);
		obj->flags = AObject::Flags::OBJ_HAS_MEMBER_DATA;
		areaAllocator.addManagedMemory(sizeof(NormalArray<AObject *>) + memberCount * sizeof(AObject *));
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
	inline AObject *createString(const char *str) {
		return get(AString::from(str));
	}
	inline AObject *createString(std::string str) {
		return get(AString::from(str));
	}
	inline void setMaxManagedMemory(size_t limit) {
		areaAllocator.maxManagedMemory = limit;
	}
	inline size_t getMaxManagedMemory() const {
		return areaAllocator.maxManagedMemory;
	}
	inline size_t getCurrentManagedMemory() const {
		return areaAllocator.currentManagedMemory;
	}
	inline void addManagedMemory(int64_t delta) {
		areaAllocator.addManagedMemory(delta);
	}
	void destroy();
};

} // namespace Autolang

#endif