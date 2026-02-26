#ifndef LIBS_MAP_CPP
#define LIBS_MAP_CPP

#include "map.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace map {

struct ObjStringHashable {
	inline size_t operator()(const AObject *s) const {
		size_t h = 0;
		for (size_t i = 0; i < s->str->size; ++i) {
			h = h * 31 + (unsigned char)s->str->data[i];
		}
		return h;
	}
};

struct ObjStringEqualable {
	inline bool operator()(const AObject *a, const AObject *b) const {
		return a->str->size == b->str->size &&
		       memcmp(a->str->data, b->str->data, a->str->size) == 0;
	}
};

struct AHashMap {
	ClassId type;
	void *data;
};

template <typename MapType, bool ReleaseKey>
static void destroyMap(ANotifier &notifier, void *hashMapData) {
	auto hashMapData_ = static_cast<AHashMap *>(hashMapData);
	auto map = static_cast<MapType *>(hashMapData_->data);
	for (auto &pair : *map) {
		if constexpr (ReleaseKey)
			notifier.release(pair.first);
		notifier.release(pair.second);
	}
	delete map;
	delete hashMapData_;
}

using IntHashMap = HashMap<int64_t, AObject *>;
using FloatHashMap = HashMap<double, AObject *>;
using StringHashMap =
    HashMap<AObject *, AObject *, ObjStringHashable, ObjStringEqualable>;
using ObjectHashMap = HashMap<AObject *, AObject *>;

AObject *constructor(NativeFuncInData) {
	ClassId keyId = args[0]->i;
	switch (keyId) {
		case DefaultClass::intClassId: {
			return notifier.createNativeData(
			    keyId, new AHashMap{keyId, new IntHashMap()},
			    destroyMap<IntHashMap, false>);
		}
		case DefaultClass::floatClassId: {
			return notifier.createNativeData(
			    keyId, new AHashMap{keyId, new FloatHashMap()},
			    destroyMap<FloatHashMap, false>);
		}
		case DefaultClass::stringClassId: {
			return notifier.createNativeData(
			    keyId, new AHashMap{keyId, new StringHashMap()},
			    destroyMap<StringHashMap, true>);
		}
		default: {
			return notifier.createNativeData(
			    keyId, new AHashMap{keyId, new ObjectHashMap()},
			    destroyMap<ObjectHashMap, true>);
		}
	}
}

AObject *remove(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			auto it = map->find(args[1]->i);
			if (it == map->end())
				return nullptr;

			notifier.release(it->second);
			map->erase(it);
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			auto it = map->find(args[1]->f);
			if (it == map->end())
				return nullptr;

			notifier.release(it->second);
			map->erase(it);
			break;
		}

		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end())
				return nullptr;

			notifier.release(it->first);
			notifier.release(it->second);
			map->erase(it);
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end())
				return nullptr;

			notifier.release(it->first);
			notifier.release(it->second);
			map->erase(it);
		}
	}

	return nullptr;
}

AObject *size(NativeFuncInData) {
	AHashMap *hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			return notifier.createInt(
			    static_cast<IntHashMap *>(hashMapData->data)->size());
		}
		case DefaultClass::floatClassId: {
			return notifier.createInt(
			    static_cast<FloatHashMap *>(hashMapData->data)->size());
		}
		case DefaultClass::stringClassId: {
			return notifier.createInt(
			    static_cast<StringHashMap *>(hashMapData->data)->size());
		}
		default: {
			return notifier.createInt(
			    static_cast<ObjectHashMap *>(hashMapData->data)->size());
		}
	}
}

AObject *get(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			if (args[1]->type != DefaultClass::intClassId) {
				notifier.throwException("Map.get: key must be Int");
				return nullptr;
			}

			auto map = static_cast<IntHashMap *>(hashMapData->data);
			auto it = map->find(args[1]->i);
			if (it == map->end()) {
				return DefaultClass::nullObject;
			}

			return it->second;
		}

		case DefaultClass::floatClassId: {
			if (args[1]->type != DefaultClass::floatClassId) {
				notifier.throwException("Map.get: key must be Float");
				return nullptr;
			}

			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			auto it = map->find(args[1]->f);
			if (it == map->end())
				return DefaultClass::nullObject;

			return it->second;
		}

		case DefaultClass::stringClassId: {
			if (args[1]->type != DefaultClass::stringClassId) {
				notifier.throwException("Map.get: key must be String");
				return nullptr;
			}

			auto map = static_cast<StringHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end())
				return DefaultClass::nullObject;

			return it->second;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end())
				return DefaultClass::nullObject;

			return it->second;
		}
	}
}

AObject *set(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);

	AObject *key = args[1];
	AObject *value = args[2];

	value->retain();

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			if (key->type != DefaultClass::intClassId) {
				notifier.throwException("Map.set: key must be Int");
				return nullptr;
			}

			auto map = static_cast<IntHashMap *>(hashMapData->data);

			auto it = map->find(key->i);
			if (it != map->end()) {
				notifier.release(it->second);
				it->second = value;
			} else {
				(*map)[key->i] = value;
			}
			break;
		}

		case DefaultClass::floatClassId: {
			if (key->type != DefaultClass::floatClassId) {
				notifier.throwException("Map.set: key must be Float");
				return nullptr;
			}

			auto map = static_cast<FloatHashMap *>(hashMapData->data);

			auto it = map->find(key->f);
			if (it != map->end()) {
				notifier.release(it->second);
				it->second = value;
			} else {
				(*map)[key->f] = value;
			}
			break;
		}

		case DefaultClass::stringClassId: {
			if (key->type != DefaultClass::stringClassId) {
				notifier.throwException("Map.set: key must be String");
				return nullptr;
			}

			auto map = static_cast<StringHashMap *>(hashMapData->data);

			auto it = map->find(key);
			if (it != map->end()) {
				notifier.release(it->second);
				it->second = value;
			} else {
				key->retain(); // vì ReleaseKey = true
				(*map)[key] = value;
			}
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);

			auto it = map->find(key);
			if (it != map->end()) {
				notifier.release(it->second);
				it->second = value;
			} else {
				key->retain(); // vì ReleaseKey = true
				(*map)[key] = value;
			}
		}
	}

	return nullptr;
}

AObject *clear(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &p : *map)
				notifier.release(p.second);
			map->clear();
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &p : *map)
				notifier.release(p.second);
			map->clear();
			break;
		}

		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &p : *map) {
				notifier.release(p.first);
				notifier.release(p.second);
			}
			map->clear();
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &p : *map) {
				notifier.release(p.first);
				notifier.release(p.second);
			}
			map->clear();
		}
	}

	return nullptr;
}

} // namespace map
} // namespace Libs
} // namespace AutoLang

#endif