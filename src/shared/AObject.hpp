	#ifndef AOBJECT_HPP
	#define AOBJECT_HPP

	#include "AString.hpp"
	#include "shared/DefaultClass.hpp"
	#include "shared/FunctionObject.hpp"
	#include "shared/NormalArray.hpp"
	#include "shared/Type.hpp"
	#ifndef NO_INCLUDE_LIBS_JSON
	#include <third_party/nlohmann/json.hpp>
	#endif
	#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#include <emscripten/bind.h>
	#endif
	#include <iostream>

	namespace AutoLang {

	class AVM;

	using DestructorParameters = void (*)(ANotifier &notifier, void *data);

	struct ANativeData {
		void *data;
		DestructorParameters destructor;
	};

	struct ABytes {
		int32_t capacity;
		int32_t size;
		uint8_t *data;
		ABytes(int32_t capacity, int32_t size, uint8_t *data)
			: capacity(capacity), size(size), data(data) {}
	};

	struct AObject {
		enum Flags : uint32_t {
			OBJ_IS_FREE = 1u << 0,
			OBJ_IS_CONST = 1u << 1,
			OBJ_IS_NATIVE_DATA = 1u << 2,
			OBJ_IS_NO_DATA = 1u << 3,
			OBJ_IS_ARRAY = 1u << 4,
			OBJ_IS_SET = 1u << 5,
			OBJ_IS_MAP = 1u << 6,
			OBJ_HAS_MEMBER_DATA = 1u << 7,
			OBJ_IS_JS_OBJECT = 1u << 8
		};
		ClassId type;
		uint32_t refCount;
		uint32_t flags = 0;
		union {
			int64_t i;
			double f;
			uint8_t b;
			FunctionObject *function;
			NormalArray<AObject *> *member;
			AString *str;
			ANativeData *data;
			ABytes *bytes;
	#ifndef NO_INCLUDE_LIBS_JSON
			nlohmann::json *json;
	#endif
	#ifdef __EMSCRIPTEN__
			emscripten::val *jsObject;
	#endif
		};
		AObject() : type(0), refCount(0) {}
		AObject(ClassId type) : type(type), refCount(0) {}
		AObject(ClassId type, uint32_t memberCount)
			: type(type), refCount(0),
			member(new NormalArray<AObject *>(memberCount)) {}
		AObject(int64_t i)
			: type(AutoLang::DefaultClass::intClassId), refCount(0), i(i) {}
		AObject(double f)
			: type(AutoLang::DefaultClass::floatClassId), refCount(0), f(f) {}
		AObject(AString *str)
			: type(AutoLang::DefaultClass::stringClassId), refCount(0), str(str) {}
		AObject(FunctionObject *function)
			: type(AutoLang::DefaultClass::functionClassId), refCount(0),
			function(function) {}
		AObject(ClassId type, ANativeData *data)
			: type(type), refCount(0), data(data) {}
		inline void retain() {
			if (flags & AObject::Flags::OBJ_IS_CONST)
				return;
			++refCount;
		};
		template <bool checkRefCount = false>
		inline void free(ANotifier &notifier) {
			if (flags == AObject::Flags::OBJ_IS_FREE) {
				return;
			}
			switch (type) {
				case AutoLang::DefaultClass::intClassId:
				case AutoLang::DefaultClass::floatClassId: {
					flags = AObject::Flags::OBJ_IS_FREE;
					return;
				}
				case AutoLang::DefaultClass::boolClassId:
				case AutoLang::DefaultClass::nullClassId: {
					return;
				}
				case AutoLang::DefaultClass::stringClassId: {
					// if constexpr (checkRefCount) {
					// 	if (refCount > 0)
					// 		--refCount;
					// 	if (refCount != 0)
					// 		return;
					// }
					delete str;
					flags = AObject::Flags::OBJ_IS_FREE;
					return;
				}
	#ifndef NO_INCLUDE_LIBS_JSON
				case AutoLang::DefaultClass::jsonClassId: {
					delete json;
					flags = AObject::Flags::OBJ_IS_FREE;
					return;
				}
	#endif
	#ifdef __EMSCRIPTEN__
				case AutoLang::DefaultClass::jsObjectClassId: {
					delete jsObject;
					flags = AObject::Flags::OBJ_IS_FREE;
					return;
				}
	#endif
				case AutoLang::DefaultClass::bytesClassId: {
					delete[] bytes->data;
					delete bytes;
					flags = AObject::Flags::OBJ_IS_FREE;
					return;
				}
				case AutoLang::DefaultClass::functionClassId: {
					for (int i = function->size; i-- > 0;) {
						--function->args[i]->refCount;
					}
					delete function;
					flags = AObject::Flags::OBJ_IS_FREE;
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
			#ifdef __EMSCRIPTEN__
			if (flags & Flags::OBJ_IS_JS_OBJECT) {
				delete jsObject;
				return;
			}
			#endif
			if (flags & Flags::OBJ_IS_NO_DATA) {
				flags = AObject::Flags::OBJ_IS_FREE;
				return;
			}
			if (flags & Flags::OBJ_IS_NATIVE_DATA) {
				if (data->destructor) {
					data->destructor(notifier, data->data);
				}
				delete data;
				flags = AObject::Flags::OBJ_IS_FREE;
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
			flags = AObject::Flags::OBJ_IS_FREE;
		}
	};

	struct AObjectHashable {
		inline size_t operator()(const AObject *obj) const {
			switch (obj->type) {
				case AutoLang::DefaultClass::intClassId: {
					return obj->i;
				}
				case AutoLang::DefaultClass::floatClassId: {
					uint32_t bits;
					std::memcpy(&bits, &obj->f, sizeof(bits));

					size_t h = bits;

					h ^= h >> 16;
					h *= 0x7feb352d;
					h ^= h >> 15;
					h *= 0x846ca68b;
					h ^= h >> 16;

					return h;
				}
				case AutoLang::DefaultClass::stringClassId: {
					size_t h = 1469598103934665603ULL;
					for (size_t i = 0; i < obj->str->size; ++i) {
						h ^= (unsigned char)obj->str->data[i];
						h *= 1099511628211ULL;
					}
					return h;
				}
				case AutoLang::DefaultClass::functionClassId: {
					return size_t(obj->function);
				}
				default: {
					return size_t(obj);
				}
			}
		}
	};

	struct AObjectEqualable {
		inline bool operator()(const AObject *a, const AObject *b) const {
			if (a->type != b->type) {
				return false;
			}
			switch (a->type) {
				case AutoLang::DefaultClass::intClassId: {
					return a->i == b->i;
				}
				case AutoLang::DefaultClass::floatClassId: {
					return a->f == b->f;
				}
				case AutoLang::DefaultClass::stringClassId: {
					return a->str == b->str;
				}
				default: {
					return true;
				}
			}
		}
	};

	} // namespace AutoLang

	#endif