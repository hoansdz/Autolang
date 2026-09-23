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

AObject *plus(NativeFuncInData) {
	auto map1Obj = args[0];
	auto map2Obj = args[1];
	auto h1 = static_cast<AHashMap *>(map1Obj->data->data);
	auto h2 = static_cast<AHashMap *>(map2Obj->data->data);
	AObject *newObj = constructor(notifier, map1Obj->type, h1->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (h1->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(h1->data);
			auto m2 = static_cast<IntHashMap *>(h2->data);
			auto res = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				res->insert({k, v});
			}
			for (auto &[k, v] : *m2) {
				auto it = res->find(k);
				if (it != res->end()) {
					v->retain();
					notifier.release(it->second);
					it->second = v;
				} else {
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(h1->data);
			auto m2 = static_cast<FloatHashMap *>(h2->data);
			auto res = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				res->insert({k, v});
			}
			for (auto &[k, v] : *m2) {
				auto it = res->find(k);
				if (it != res->end()) {
					v->retain();
					notifier.release(it->second);
					it->second = v;
				} else {
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(h1->data);
			auto m2 = static_cast<StringHashMap *>(h2->data);
			auto res = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				k->retain();
				v->retain();
				res->insert({k, v});
			}
			for (auto &[k, v] : *m2) {
				auto it = res->find(k);
				if (it != res->end()) {
					v->retain();
					notifier.release(it->second);
					it->second = v;
				} else {
					k->retain();
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(h1->data);
			auto m2 = static_cast<ObjectHashMap *>(h2->data);
			auto res = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				k->retain();
				v->retain();
				res->insert({k, v});
			}
			for (auto &[k, v] : *m2) {
				auto it = res->find(k);
				if (it != res->end()) {
					v->retain();
					notifier.release(it->second);
					it->second = v;
				} else {
					k->retain();
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
	}
	return newObj;
}

AObject *minus(NativeFuncInData) {
	auto mapObj = args[0];
	auto key = args[1];
	auto h = static_cast<AHashMap *>(mapObj->data->data);
	AObject *newObj = constructor(notifier, mapObj->type, h->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (h->type) {
		case DefaultClass::intClassId: {
			auto m = static_cast<IntHashMap *>(h->data);
			auto res = static_cast<IntHashMap *>(newMapData->data);
			int64_t targetKey = key->i;
			for (auto &[k, v] : *m) {
				if (k != targetKey) {
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		case DefaultClass::floatClassId: {
			auto m = static_cast<FloatHashMap *>(h->data);
			auto res = static_cast<FloatHashMap *>(newMapData->data);
			double targetKey = (key->type == DefaultClass::intClassId) ? static_cast<double>(key->i) : key->f;
			for (auto &[k, v] : *m) {
				if (k != targetKey) {
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		case DefaultClass::stringClassId: {
			auto m = static_cast<StringHashMap *>(h->data);
			auto res = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m) {
				if (!(k->str->size == key->str->size && memcmp(k->str->data, key->str->data, k->str->size) == 0)) {
					k->retain();
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
		default: {
			auto m = static_cast<ObjectHashMap *>(h->data);
			auto res = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m) {
				if (!DefaultFunction::op_eqeq(k, key)) {
					k->retain();
					v->retain();
					res->insert({k, v});
				}
			}
			notifier.addManagedMemory(res->size() * 32);
			break;
		}
	}
	return newObj;
}

AObject *is_not_empty(NativeFuncInData) {
	auto res = is_empty(notifier, args, argSize);
	return notifier.createBool(res == DefaultClass::falseObject);
}

AObject *contains_value(NativeFuncInData) {
	auto hashMapData = static_cast<AHashMap *>(args[0]->data->data);
	AObject *target = args[1];
	bool found = false;

	auto checkMatch = [&](AObject *v) {
		if (v == target) return true;
		if (!v || !target) return false;
		if (v->type == target->type) {
			switch (v->type) {
				case DefaultClass::intClassId: return v->i == target->i;
				case DefaultClass::floatClassId: return v->f == target->f;
				case DefaultClass::boolClassId: return v->b == target->b;
				case DefaultClass::stringClassId:
					return v->str->size == target->str->size &&
					       memcmp(v->str->data, target->str->data, v->str->size) == 0;
				default:
					return DefaultFunction::op_eqeq(v, target);
			}
		}
		return false;
	};

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto map = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				if (checkMatch(v)) { found = true; break; }
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				if (checkMatch(v)) { found = true; break; }
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto map = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				if (checkMatch(v)) { found = true; break; }
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *map) {
				if (checkMatch(v)) { found = true; break; }
			}
			break;
		}
	}
	return notifier.createBool(found);
}

AObject *get_or_put(NativeFuncInData) {
	auto mapObj = args[0];
	auto key = args[1];
	auto defaultFunc = args[2];
	AObject *getArgs[2] = {mapObj, key};
	auto existing = get(notifier, getArgs, 2);
	if (existing && existing != DefaultClass::nullObject) {
		return existing;
	}
	auto newVal = notifier.callFunctionObject(defaultFunc);
	if (notifier.hasException()) return nullptr;
	AObject *setArgs[3] = {mapObj, key, newVal};
	set(notifier, setArgs, 3);
	return newVal;
}

AObject *get_or_else(NativeFuncInData) {
	auto mapObj = args[0];
	auto key = args[1];
	auto defaultBlock = args[2];
	AObject *gArgs[2] = {mapObj, key};
	auto v = get(notifier, gArgs, 2);
	if (v && v != DefaultClass::nullObject) {
		return v;
	}
	return notifier.callFunctionObject(defaultBlock);
}

AObject *filter_keys(NativeFuncInData) {
	auto mapObj = args[0];
	auto funcObject = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	AObject *newObj = constructor(notifier, mapObj->type, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				auto res = notifier.callFunctionObject(funcObject, keyObj);
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
				auto res = notifier.callFunctionObject(funcObject, keyObj);
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
				auto res = notifier.callFunctionObject(funcObject, k);
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
				auto res = notifier.callFunctionObject(funcObject, k);
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

AObject *filter_values(NativeFuncInData) {
	auto mapObj = args[0];
	auto funcObject = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	AObject *newObj = constructor(notifier, mapObj->type, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto res = notifier.callFunctionObject(funcObject, v);
				if (notifier.hasException()) {
					notifier.release(v);
					return nullptr;
				}
				if (res == notifier.getTrueObject()) {
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				} else {
					notifier.release(v);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			auto m2 = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto res = notifier.callFunctionObject(funcObject, v);
				if (notifier.hasException()) {
					notifier.release(v);
					return nullptr;
				}
				if (res == notifier.getTrueObject()) {
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				} else {
					notifier.release(v);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			auto m2 = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto res = notifier.callFunctionObject(funcObject, v);
				if (notifier.hasException()) {
					notifier.release(v);
					return nullptr;
				}
				if (res == notifier.getTrueObject()) {
					k->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				} else {
					notifier.release(v);
				}
				notifier.release(res);
			}
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			auto m2 = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto res = notifier.callFunctionObject(funcObject, v);
				if (notifier.hasException()) {
					notifier.release(v);
					return nullptr;
				}
				if (res == notifier.getTrueObject()) {
					k->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				} else {
					notifier.release(v);
				}
				notifier.release(res);
			}
			break;
		}
	}
	return newObj;
}

AObject *map_values(NativeFuncInData) {
	auto mapObj = args[0];
	auto transform = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	ClassId returnId = notifier.callFrame->func->returnId;
	AObject *newObj = constructor(notifier, returnId, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);

	bool expectsTwo = false;
	if (transform->type == DefaultClass::functionClassId && transform->function && transform->function->function) {
		if (transform->function->function->argSize >= 2) {
			expectsTwo = true;
		}
	}

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				AObject *newV = nullptr;
				if (expectsTwo) {
					auto keyObj = notifier.createInt(k);
					keyObj->retain();
					newV = notifier.callFunctionObject(transform, keyObj, v);
					notifier.release(keyObj);
				} else {
					newV = notifier.callFunctionObject(transform, v);
				}
				notifier.release(v);
				if (notifier.hasException()) return nullptr;
				m2->insert({k, newV});
				notifier.addManagedMemory(32);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			auto m2 = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				AObject *newV = nullptr;
				if (expectsTwo) {
					auto keyObj = notifier.createFloat(k);
					keyObj->retain();
					newV = notifier.callFunctionObject(transform, keyObj, v);
					notifier.release(keyObj);
				} else {
					newV = notifier.callFunctionObject(transform, v);
				}
				notifier.release(v);
				if (notifier.hasException()) return nullptr;
				m2->insert({k, newV});
				notifier.addManagedMemory(32);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			auto m2 = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto newV = expectsTwo ? notifier.callFunctionObject(transform, k, v)
				                       : notifier.callFunctionObject(transform, v);
				notifier.release(v);
				if (notifier.hasException()) return nullptr;
				k->retain();
				m2->insert({k, newV});
				notifier.addManagedMemory(32);
			}
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			auto m2 = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				v->retain();
				auto newV = expectsTwo ? notifier.callFunctionObject(transform, k, v)
				                       : notifier.callFunctionObject(transform, v);
				notifier.release(v);
				if (notifier.hasException()) return nullptr;
				k->retain();
				m2->insert({k, newV});
				notifier.addManagedMemory(32);
			}
			break;
		}
	}
	return newObj;
}

static inline ClassId getMapKeyType(ANotifier &notifier, ClassId classId) {
	if (classId < notifier.vm->data.classes.size()) {
		auto clazz = notifier.vm->data.classes[classId];
		if (clazz && clazz->genericType.size > 0) {
			return notifier.vm->data.allGenericType[clazz->genericType.offset];
		}
	}
	return DefaultClass::anyClassId;
}

AObject *map_keys(NativeFuncInData) {
	auto mapObj = args[0];
	auto transform = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId keyId = getMapKeyType(notifier, returnId);
	AObject *newObj = constructor(notifier, returnId, keyId);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;

	bool expectsTwo = false;
	if (transform->type == DefaultClass::functionClassId && transform->function && transform->function->function) {
		if (transform->function->function->argSize >= 2) {
			expectsTwo = true;
		}
	}

	auto setIntoNew = [&](AObject *newK, AObject *v) {
		AObject *sArgs[3] = {newObj, newK, v};
		set(notifier, sArgs, 3);
	};

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				auto newK = expectsTwo ? notifier.callFunctionObject(transform, keyObj, v)
				                       : notifier.callFunctionObject(transform, keyObj);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				setIntoNew(newK, v);
				notifier.release(newK);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				auto newK = expectsTwo ? notifier.callFunctionObject(transform, keyObj, v)
				                       : notifier.callFunctionObject(transform, keyObj);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				setIntoNew(newK, v);
				notifier.release(newK);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m1) {
				auto newK = expectsTwo ? notifier.callFunctionObject(transform, k, v)
				                       : notifier.callFunctionObject(transform, k);
				if (notifier.hasException()) return nullptr;
				setIntoNew(newK, v);
				notifier.release(newK);
			}
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m1) {
				auto newK = expectsTwo ? notifier.callFunctionObject(transform, k, v)
				                       : notifier.callFunctionObject(transform, k);
				if (notifier.hasException()) return nullptr;
				setIntoNew(newK, v);
				notifier.release(newK);
			}
			break;
		}
	}
	return newObj;
}

AObject *plus_pair(NativeFuncInData) {
	auto newMap = clone(notifier, args, 1);
	if (!newMap) return nullptr;
	auto pairObj = args[1];
	if (pairObj && (pairObj->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA)) {
		auto clazz = notifier.vm->data.classes[pairObj->type];
		auto itFirst = clazz->memberMap.find("first");
		auto itSecond = clazz->memberMap.find("second");
		if (itFirst != clazz->memberMap.end() && itSecond != clazz->memberMap.end()) {
			AObject *sArgs[3] = {newMap, pairObj->member->data[itFirst->second], pairObj->member->data[itSecond->second]};
			set(notifier, sArgs, 3);
		}
	}
	return newMap;
}

static inline AObject *mapToPairsArray(ANotifier &notifier, AObject *mapObj, ClassId returnId) {
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	ClassId pairClassId = DefaultClass::anyClassId;
	if (returnId < notifier.vm->data.classes.size()) {
		auto rClazz = notifier.vm->data.classes[returnId];
		if (rClazz && rClazz->genericType.size > 0) {
			pairClassId = notifier.vm->data.allGenericType[rClazz->genericType.offset];
		}
	}
	if (pairClassId == DefaultClass::anyClassId) {
		for (ClassId c = 0; c < notifier.vm->data.classes.size(); ++c) {
			auto clazz = notifier.vm->data.classes[c];
			if (clazz) {
				auto name = clazz->getName(notifier.vm->data);
				if (name == "Pair" || name.rfind("Pair<", 0) == 0) {
					pairClassId = c;
					break;
				}
			}
		}
	}
	auto newArr = notifier.createArray(returnId, pairClassId);

	auto addPair = [&](AObject *k, AObject *v) {
		auto pairObj = notifier.createMemberObject(pairClassId, 2);
		k->retain();
		pairObj->member->data[0] = k;
		v->retain();
		pairObj->member->data[1] = v;
		notifier.arrayAdd(newArr, pairObj);
		notifier.release(pairObj);
	};

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				addPair(keyObj, v);
				notifier.release(keyObj);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				addPair(keyObj, v);
				notifier.release(keyObj);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				addPair(k, v);
			}
			break;
		}
		default: {
			auto m = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				addPair(k, v);
			}
			break;
		}
	}
	return newArr;
}

AObject *entries(NativeFuncInData) {
	return mapToPairsArray(notifier, args[0], notifier.callFrame->func->returnId);
}

AObject *to_list(NativeFuncInData) {
	return mapToPairsArray(notifier, args[0], notifier.callFrame->func->returnId);
}

AObject *put_all(NativeFuncInData) {
	auto thisMap = args[0];
	auto fromMap = args[1];
	auto hFrom = static_cast<AHashMap *>(fromMap->data->data);
	switch (hFrom->type) {
		case DefaultClass::intClassId: {
			auto m = static_cast<IntHashMap *>(hFrom->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				AObject *sArgs[3] = {thisMap, keyObj, v};
				set(notifier, sArgs, 3);
				notifier.release(keyObj);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m = static_cast<FloatHashMap *>(hFrom->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				AObject *sArgs[3] = {thisMap, keyObj, v};
				set(notifier, sArgs, 3);
				notifier.release(keyObj);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m = static_cast<StringHashMap *>(hFrom->data);
			for (auto &[k, v] : *m) {
				AObject *sArgs[3] = {thisMap, k, v};
				set(notifier, sArgs, 3);
			}
			break;
		}
		default: {
			auto m = static_cast<ObjectHashMap *>(hFrom->data);
			for (auto &[k, v] : *m) {
				AObject *sArgs[3] = {thisMap, k, v};
				set(notifier, sArgs, 3);
			}
			break;
		}
	}
	return notifier.getNullObject();
}

AObject *remove_pair(NativeFuncInData) {
	auto mapObj = args[0];
	auto key = args[1];
	auto val = args[2];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			if (key->type == DefaultClass::intClassId) {
				auto map = static_cast<IntHashMap *>(hashMapData->data);
				auto it = map->find(key->i);
				if (it != map->end() && DefaultFunction::op_eqeq(it->second, val)) {
					notifier.release(it->second);
					map->erase(it);
					notifier.addManagedMemory(-32);
					return notifier.createBool(true);
				}
			}
			break;
		}
		case DefaultClass::floatClassId: {
			double fKey = (key->type == DefaultClass::intClassId) ? key->i : key->f;
			auto map = static_cast<FloatHashMap *>(hashMapData->data);
			auto it = map->find(fKey);
			if (it != map->end() && DefaultFunction::op_eqeq(it->second, val)) {
				notifier.release(it->second);
				map->erase(it);
				notifier.addManagedMemory(-32);
				return notifier.createBool(true);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			if (key->type == DefaultClass::stringClassId) {
				auto map = static_cast<StringHashMap *>(hashMapData->data);
				auto it = map->find(key);
				if (it != map->end() && DefaultFunction::op_eqeq(it->second, val)) {
					notifier.release(it->first);
					notifier.release(it->second);
					map->erase(it);
					notifier.addManagedMemory(-32);
					return notifier.createBool(true);
				}
			}
			break;
		}
		default: {
			auto map = static_cast<ObjectHashMap *>(hashMapData->data);
			auto it = map->find(key);
			if (it != map->end() && DefaultFunction::op_eqeq(it->second, val)) {
				notifier.release(it->first);
				notifier.release(it->second);
				map->erase(it);
				notifier.addManagedMemory(-32);
				return notifier.createBool(true);
			}
			break;
		}
	}
	return notifier.createBool(false);
}

static inline bool testMapEntryPredicate(ANotifier &notifier, AObject *func, AObject *k, AObject *v, bool expectsTwo) {
	AObject *res = nullptr;
	if (expectsTwo) {
		res = notifier.callFunctionObject(func, k, v);
	} else {
		res = notifier.callFunctionObject(func, k);
	}
	if (notifier.hasException()) return false;
	bool match = (res == notifier.getTrueObject());
	notifier.release(res);
	return match;
}

AObject *any_fn(NativeFuncInData) {
	if (argSize <= 1) {
		return is_not_empty(notifier, args, argSize);
	}
	auto mapObj = args[0];
	auto func = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	bool expectsTwo = (func->type == DefaultClass::functionClassId && func->function && func->function->function && func->function->function->argSize >= 2);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, func, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (match) return notifier.createBool(true);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, func, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (match) return notifier.createBool(true);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				if (testMapEntryPredicate(notifier, func, k, v, expectsTwo)) return notifier.createBool(true);
				if (notifier.hasException()) return nullptr;
			}
			break;
		}
		default: {
			auto m = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				if (testMapEntryPredicate(notifier, func, k, v, expectsTwo)) return notifier.createBool(true);
				if (notifier.hasException()) return nullptr;
			}
			break;
		}
	}
	return notifier.createBool(false);
}

AObject *all_fn(NativeFuncInData) {
	auto mapObj = args[0];
	auto func = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	bool expectsTwo = (func->type == DefaultClass::functionClassId && func->function && func->function->function && func->function->function->argSize >= 2);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m = static_cast<IntHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, func, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (!match) return notifier.createBool(false);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m = static_cast<FloatHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, func, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (!match) return notifier.createBool(false);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m = static_cast<StringHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				if (!testMapEntryPredicate(notifier, func, k, v, expectsTwo)) return notifier.createBool(false);
				if (notifier.hasException()) return nullptr;
			}
			break;
		}
		default: {
			auto m = static_cast<ObjectHashMap *>(hashMapData->data);
			for (auto &[k, v] : *m) {
				if (!testMapEntryPredicate(notifier, func, k, v, expectsTwo)) return notifier.createBool(false);
				if (notifier.hasException()) return nullptr;
			}
			break;
		}
	}
	return notifier.createBool(true);
}

AObject *none_fn(NativeFuncInData) {
	if (argSize <= 1) {
		return is_empty(notifier, args, argSize);
	}
	auto res = any_fn(notifier, args, argSize);
	if (!res) return nullptr;
	return notifier.createBool(res != notifier.getTrueObject());
}

AObject *filter_not(NativeFuncInData) {
	auto mapObj = args[0];
	auto funcObject = args[1];
	auto hashMapData = static_cast<AHashMap *>(mapObj->data->data);
	ClassId returnId = notifier.callFrame->func->returnId;
	AObject *newObj = constructor(notifier, returnId, hashMapData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_MAP;
	auto newMapData = static_cast<AHashMap *>(newObj->data->data);
	bool expectsTwo = (funcObject->type == DefaultClass::functionClassId && funcObject->function && funcObject->function->function && funcObject->function->function->argSize >= 2);

	switch (hashMapData->type) {
		case DefaultClass::intClassId: {
			auto m1 = static_cast<IntHashMap *>(hashMapData->data);
			auto m2 = static_cast<IntHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createInt(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, funcObject, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (!match) {
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto m1 = static_cast<FloatHashMap *>(hashMapData->data);
			auto m2 = static_cast<FloatHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				auto keyObj = notifier.createFloat(k);
				keyObj->retain();
				bool match = testMapEntryPredicate(notifier, funcObject, keyObj, v, expectsTwo);
				notifier.release(keyObj);
				if (notifier.hasException()) return nullptr;
				if (!match) {
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto m1 = static_cast<StringHashMap *>(hashMapData->data);
			auto m2 = static_cast<StringHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				bool match = testMapEntryPredicate(notifier, funcObject, k, v, expectsTwo);
				if (notifier.hasException()) return nullptr;
				if (!match) {
					k->retain();
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
			}
			break;
		}
		default: {
			auto m1 = static_cast<ObjectHashMap *>(hashMapData->data);
			auto m2 = static_cast<ObjectHashMap *>(newMapData->data);
			for (auto &[k, v] : *m1) {
				bool match = testMapEntryPredicate(notifier, funcObject, k, v, expectsTwo);
				if (notifier.hasException()) return nullptr;
				if (!match) {
					k->retain();
					v->retain();
					m2->insert({k, v});
					notifier.addManagedMemory(32);
				}
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
