#ifndef LIBS_MAP_CPP
#define LIBS_MAP_CPP

#include "map.hpp"
#include "backend/vm/ANotifier.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Type.hpp"

namespace Autolang {
class ACompiler;
namespace Libs {
namespace map {

template <typename MapType, bool ReleaseKey>
static void destroyMap(ANotifier &notifier, void *hashMapData) {
	auto hashMapData_ = static_cast<AHashMap *>(hashMapData);
	auto map = static_cast<MapType *>(hashMapData_->data);
	size_t mapSize = map->size();
	for (auto &[key, value] : *map) {
		if constexpr (ReleaseKey)
			notifier.release(key);
		notifier.release(value);
	}
	notifier.addManagedMemory(-static_cast<int64_t>(mapSize * 32));
	delete map;
	delete hashMapData_;
}

AObject *constructor(ANotifier &notifier, ClassId classId,
                            ClassId keyId) {
	switch (keyId) {
		case DefaultClass::intClassId: {
			return notifier.createNativeData(
			    classId, new AHashMap{keyId, new IntHashMap()},
			    destroyMap<IntHashMap, false>);
		}
		case DefaultClass::floatClassId: {
			return notifier.createNativeData(
			    classId, new AHashMap{keyId, new FloatHashMap()},
			    destroyMap<FloatHashMap, false>);
		}
		case DefaultClass::stringClassId: {
			return notifier.createNativeData(
			    classId, new AHashMap{keyId, new StringHashMap()},
			    destroyMap<StringHashMap, true>);
		}
		default: {
			return notifier.createNativeData(
			    classId, new AHashMap{keyId, new ObjectHashMap()},
			    destroyMap<ObjectHashMap, true>);
		}
	}
}

AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	ClassId keyId = args[1]->i;
	auto obj = constructor(notifier, classId, keyId);
	obj->flags |= AObject::Flags::OBJ_IS_MAP;
	return obj;
}

AObject *is_empty(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	bool empty = false;

	switch (hashMapData->type) {
		case DefaultClass::intClassId:
			empty = static_cast<IntHashMap *>(hashMapData->data)->empty();
			break;
		case DefaultClass::floatClassId:
			empty = static_cast<FloatHashMap *>(hashMapData->data)->empty();
			break;
		case DefaultClass::stringClassId:
			empty = static_cast<StringHashMap *>(hashMapData->data)->empty();
			break;
		default:
			empty = static_cast<ObjectHashMap *>(hashMapData->data)->empty();
			break;
	}
	return notifier.createBool(empty);
}

AObject *contains_key(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	AObject *key = args[1];
	bool found = false;

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			if (key->type == DefaultClass::intClassId) {
				auto map = static_cast<IntHashMap *>(hashMapData->data);
				found = map->find(key->i) != map->end();
			}
			break;
		}
		case DefaultClass::floatClassId: {
			if (key->type == DefaultClass::floatClassId) {
				auto map = static_cast<FloatHashMap *>(hashMapData->data);
				found = map->find(key->f) != map->end();
			}
			break;
		}
		case DefaultClass::stringClassId: {
			if (key->type == DefaultClass::stringClassId) {
				auto map = static_cast<StringHashMap *>(hashMapData->data);
				found = map->find(key) != map->end();
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			found = map->find(key) != map->end();
			break;
		}
	}
	return notifier.createBool(found);
}

AObject *for_each(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	auto funcObject = args[1];

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				auto keyObj = notifier.createInt(k);
				auto value = notifier.callFunctionObject(funcObject, keyObj, v);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				auto keyObj = notifier.createFloat(k);
				auto value = notifier.callFunctionObject(funcObject, keyObj, v);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				auto value = notifier.callFunctionObject(funcObject, k, v);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				auto value = notifier.callFunctionObject(funcObject, k, v);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
	}
	return nullptr;
}

AObject *keys(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	auto classId = notifier.callFrame->func->returnId;
	auto newArr = notifier.createArray(classId);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				notifier.arrayAdd(newArr, notifier.createInt(k));
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				notifier.arrayAdd(newArr, notifier.createFloat(k));
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				notifier.arrayAdd(newArr, k);
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				notifier.arrayAdd(newArr, k);
			}
			break;
		}
	}
	return newArr;
}

AObject *values(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	auto classId = notifier.callFrame->func->returnId;
	auto newArr = notifier.createArray(classId);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				notifier.arrayAdd(newArr, v);
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				notifier.arrayAdd(newArr, v);
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				notifier.arrayAdd(newArr, v);
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map)
				notifier.arrayAdd(newArr, v);
			break;
		}
	}
	return newArr;
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
			notifier.addManagedMemory(-32);
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			auto it = map->find(args[1]->f);
			if (it == map->end())
				return nullptr;

			notifier.release(it->second);
			map->erase(it);
			notifier.addManagedMemory(-32);
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
			notifier.addManagedMemory(-32);
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
			notifier.addManagedMemory(-32);
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

			AObject *value = it->second;
			switch (value->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
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

			AObject *value = it->second;
			switch (value->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
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

			AObject *value = it->second;
			switch (value->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end())
				return DefaultClass::nullObject;

			AObject *value = it->second;
			switch (value->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
		}
	}
}

AObject *get_or_default(NativeFuncInData) {
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
				auto defaultObject = args[2];
				defaultObject->retain();
				(*map)[args[1]->i] = defaultObject;
				return defaultObject;
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
			if (it == map->end()) {
				auto defaultObject = args[2];
				defaultObject->retain();
				(*map)[args[1]->f] = defaultObject;
				return defaultObject;
			}

			return it->second;
		}

		case DefaultClass::stringClassId: {
			if (args[1]->type != DefaultClass::stringClassId) {
				notifier.throwException("Map.get: key must be String");
				return nullptr;
			}

			auto map = static_cast<StringHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end()) {
				auto defaultObject = args[2];
				defaultObject->retain();
				(*map)[args[1]] = defaultObject;
				return defaultObject;
			}

			return it->second;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			auto it = map->find(args[1]);
			if (it == map->end()) {
				auto defaultObject = args[2];
				defaultObject->retain();
				(*map)[args[1]] = defaultObject;
				return defaultObject;
			}

			return it->second;
		}
	}
}

AObject *set(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);

	AObject *key = args[1];
	AObject *value = args[2];

	switch (hashMapData->type) {

		case DefaultClass::intClassId: {
			if (key->type != DefaultClass::intClassId) {
				notifier.throwException("Map.set: key must be Int");
				return nullptr;
			}

			auto map = static_cast<IntHashMap *>(hashMapData->data);

			auto it = map->find(key->i);
			if (it != map->end()) {
				value->retain();
				notifier.release(it->second);
				it->second = value;
			} else {
				value->retain();
				(*map)[key->i] = value;
				notifier.addManagedMemory(32);
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
				value->retain();
				notifier.release(it->second);
				it->second = value;
			} else {
				value->retain();
				(*map)[key->f] = value;
				notifier.addManagedMemory(32);
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
				value->retain();
				notifier.release(it->second);
				it->second = value;
			} else {
				value->retain();
				key->retain();
				(*map)[key] = value;
				notifier.addManagedMemory(32);
			}
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);

			auto it = map->find(key);
			if (it != map->end()) {
				value->retain();
				notifier.release(it->second);
				it->second = value;
			} else {
				value->retain();
				key->retain();
				(*map)[key] = value;
				notifier.addManagedMemory(32);
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
			size_t mapSize = map->size();
			for (auto &p : *map)
				notifier.release(p.second);
			map->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(mapSize * 32));
			break;
		}

		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			size_t mapSize = map->size();
			for (auto &p : *map)
				notifier.release(p.second);
			map->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(mapSize * 32));
			break;
		}

		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			size_t mapSize = map->size();
			for (auto &p : *map) {
				notifier.release(p.first);
				notifier.release(p.second);
			}
			map->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(mapSize * 32));
			break;
		}

		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			size_t mapSize = map->size();
			for (auto &p : *map) {
				notifier.release(p.first);
				notifier.release(p.second);
			}
			map->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(mapSize * 32));
		}
	}

	return nullptr;
}

std::string to_string(ANotifier &notifier, AObject *obj) {
	auto hashMapData = static_cast<AHashMap *>(obj->data->data);
	std::string str = "{";
	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			if (map->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (auto &[key, value] : *map) {
				if (isFirst) {
					str += std::to_string(key) + ": " +
					       DefaultFunction::to_string(notifier, value);
					isFirst = false;
					continue;
				}
				str += ", " + std::to_string(key) + ": " +
				       DefaultFunction::to_string(notifier, value);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			if (map->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (auto &[key, value] : *map) {
				if (isFirst) {
					str += std::to_string(key) + ": " +
					       DefaultFunction::to_string(notifier, value);
					isFirst = false;
					continue;
				}
				str += ", " + std::to_string(key) + ": " +
				       DefaultFunction::to_string(notifier, value);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			if (map->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (auto &[key, value] : *map) {
				if (isFirst) {
					str += std::string(key->str->data) + ": " +
					       DefaultFunction::to_string(notifier, value);
					isFirst = false;
					continue;
				}
				str += ", " + std::string(key->str->data) + ": " +
				       DefaultFunction::to_string(notifier, value);
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			if (map->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (auto &[key, value] : *map) {
				if (isFirst) {
					str += DefaultFunction::to_string(notifier, key) + ": " +
					       DefaultFunction::to_string(notifier, value);
					isFirst = false;
					continue;
				}
				str += ", " + DefaultFunction::to_string(notifier, key) + ": " +
				       DefaultFunction::to_string(notifier, value);
			}
			break;
		}
	}
	str += '}';
	return str;
}

AObject *to_string(NativeFuncInData) {
	return notifier.createString(to_string(notifier, args[0]));
}

AObject *clone(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	AObject *newObj = constructor(notifier, args[0]->type, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				m2->insert({k, v});
			}
			notifier.addManagedMemory(m1->size() * 32);
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			auto m2 = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				m2->insert({k, v});
			}
			notifier.addManagedMemory(m1->size() * 32);
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			auto m2 = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				k->retain();
				v->retain();
				m2->insert({k, v});
			}
			notifier.addManagedMemory(m1->size() * 32);
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			auto m2 = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				k->retain();
				v->retain();
				m2->insert({k, v});
			}
			notifier.addManagedMemory(m1->size() * 32);
			break;
		}
	}
	return newObj;
}

AObject *filter(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	auto funcObject = args[1];

	AObject *newObj = constructor(notifier, args[0]->type, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				auto res = notifier.callFunctionObject(funcObject, keyObj, v);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			auto m2 = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				auto res = notifier.callFunctionObject(funcObject, keyObj, v);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			auto m2 = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto res = notifier.callFunctionObject(funcObject, k, v);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					k->retain();
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			auto m2 = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto res = notifier.callFunctionObject(funcObject, k, v);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					k->retain();
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
	}
	return newObj;
}

} // namespace map
} // namespace Libs
} // namespace Autolang

#endif
