#ifndef LIBS_SET_CPP
#define LIBS_SET_CPP

#include "set.hpp"
#include "backend/vm/ANotifier.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Type.hpp"

namespace Autolang {
namespace Libs {
namespace set {

template <typename SetType, bool ReleaseKey>
static void destroySet(ANotifier &notifier, void *unorderedSetData) {
	auto unorderedSetData_ = static_cast<AUnorderedSet *>(unorderedSetData);
	auto set = static_cast<SetType *>(unorderedSetData_->data);
	size_t setSize = set->size();

	if constexpr (ReleaseKey) {
		for (auto &key : *set) {
			notifier.release(key);
		}
	}
	notifier.addManagedMemory(-static_cast<int64_t>(setSize * 32));

	delete set;
	delete unorderedSetData_;
}

AObject *constructor(ANotifier &notifier, ClassId classId,
                            ClassId keyId) {
	switch (keyId) {
		case DefaultClass::intClassId: {
			return notifier.createNativeData(
			    classId, new AUnorderedSet{keyId, new IntHashSet()},
			    destroySet<IntHashSet, false>);
		}
		case DefaultClass::floatClassId: {
			return notifier.createNativeData(
			    classId, new AUnorderedSet{keyId, new FloatHashSet()},
			    destroySet<FloatHashSet, false>);
		}
		case DefaultClass::stringClassId: {
			return notifier.createNativeData(
			    classId, new AUnorderedSet{keyId, new StringHashSet()},
			    destroySet<StringHashSet, true>);
		}
		default: {
			return notifier.createNativeData(
			    classId, new AUnorderedSet{keyId, new ObjectHashSet()},
			    destroySet<ObjectHashSet, true>);
		}
	}
}

AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	ClassId keyId = args[1]->i;
	auto obj = constructor(notifier, classId, keyId);
	obj->flags |= AObject::Flags::OBJ_IS_SET;
	return obj;
}

AObject *add(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	AObject *element = args[1];

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			if (element->type != DefaultClass::intClassId) {
				notifier.throwException("Set.add: element must be Int");
				return nullptr;
			}
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->insert(element->i).second) {
				notifier.addManagedMemory(32);
			}
			break;
		}

		case DefaultClass::floatClassId: {
			if (element->type != DefaultClass::floatClassId) {
				notifier.throwException("Set.add: element must be Float");
				return nullptr;
			}
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			if (set->insert(element->f).second) {
				notifier.addManagedMemory(32);
			}
			break;
		}

		case DefaultClass::stringClassId: {
			if (element->type != DefaultClass::stringClassId) {
				notifier.throwException("Set.add: element must be String");
				return nullptr;
			}
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it == set->end()) {
				element->retain();
				set->insert(element);
				notifier.addManagedMemory(32);
			}
			break;
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it == set->end()) {
				element->retain();
				set->insert(element);
				notifier.addManagedMemory(32);
			}
		}
	}

	return nullptr;
}

AObject *contains(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	AObject *element = args[1];

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			if (element->type != DefaultClass::intClassId) {
				notifier.throwException("Set.contains: element must be Int");
				return nullptr;
			}
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			bool found = set->find(element->i) != set->end();
			return notifier.createBool(found);
		}

		case DefaultClass::floatClassId: {
			if (element->type != DefaultClass::floatClassId) {
				notifier.throwException("Set.contains: element must be Float");
				return nullptr;
			}
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			bool found = set->find(element->f) != set->end();
			return notifier.createBool(found);
		}

		case DefaultClass::stringClassId: {
			if (element->type != DefaultClass::stringClassId) {
				notifier.throwException("Set.contains: element must be String");
				return nullptr;
			}
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			bool found = set->find(element) != set->end();
			return notifier.createBool(found);
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			bool found = set->find(element) != set->end();
			return notifier.createBool(found);
		}
	}
}

AObject *remove(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	AObject *element = args[1];

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			auto it = set->find(element->i);
			if (it != set->end()) {
				set->erase(it);
				notifier.addManagedMemory(-32);
			}
			break;
		}

		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			auto it = set->find(element->f);
			if (it != set->end()) {
				set->erase(it);
				notifier.addManagedMemory(-32);
			}
			break;
		}

		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it != set->end()) {
				notifier.release(*it);
				set->erase(it);
				notifier.addManagedMemory(-32);
			}
			break;
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it != set->end()) {
				notifier.release(*it);
				set->erase(it);
				notifier.addManagedMemory(-32);
			}
		}
	}

	return nullptr;
}

AObject *size(NativeFuncInData) {
	AUnorderedSet *unorderedSetData =
	    static_cast<AUnorderedSet *>(args[0]->data->data);
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId:
			return notifier.createInt(
			    static_cast<IntHashSet *>(unorderedSetData->data)->size());
		case DefaultClass::floatClassId:
			return notifier.createInt(
			    static_cast<FloatHashSet *>(unorderedSetData->data)->size());
		case DefaultClass::stringClassId:
			return notifier.createInt(
			    static_cast<StringHashSet *>(unorderedSetData->data)->size());
		default:
			return notifier.createInt(
			    static_cast<ObjectHashSet *>(unorderedSetData->data)->size());
	}
}

AObject *is_empty(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	bool empty = false;

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId:
			empty = static_cast<IntHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::floatClassId:
			empty =
			    static_cast<FloatHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::stringClassId:
			empty =
			    static_cast<StringHashSet *>(unorderedSetData->data)->empty();
			break;
		default:
			empty =
			    static_cast<ObjectHashSet *>(unorderedSetData->data)->empty();
			break;
	}
	return notifier.createBool(empty);
}

AObject *for_each(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto funcObject = args[1];

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t value : *set) {
				auto obj = notifier.createInt(value);
				auto v = notifier.callFunctionObject(funcObject, obj);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double value : *set) {
				auto obj = notifier.createFloat(value);
				auto v = notifier.callFunctionObject(funcObject, obj);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			for (AObject *value : *set) {
				auto v = notifier.callFunctionObject(funcObject, value);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (AObject *value : *set) {
				auto v = notifier.callFunctionObject(funcObject, value);
				if (notifier.hasException())
					return nullptr;
			}
			break;
		}
	}
	return nullptr;
}

AObject *to_array(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto newArr = notifier.createArray(notifier.callFrame->func->returnId);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t value : *set) {
				notifier.arrayAdd(newArr, notifier.createInt(value));
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double value : *set) {
				notifier.arrayAdd(newArr, notifier.createFloat(value));
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			for (AObject *value : *set) {
				notifier.arrayAdd(newArr, value);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (AObject *value : *set) {
				notifier.arrayAdd(newArr, value);
			}
			break;
		}
	}
	return newArr;
}

AObject *set_union(NativeFuncInData) {
	auto set1Data = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto set2Data = static_cast<AUnorderedSet *>(args[1]->data->data);

	AObject *newObj = constructor(notifier, args[0]->type, set1Data->type);
	auto newSetData = static_cast<AUnorderedSet *>(newObj->data->data);

	switch (set1Data->type) {
		case DefaultClass::intClassId: {
			auto s1 = static_cast<IntHashSet *>(set1Data->data);
			auto s2 = static_cast<IntHashSet *>(set2Data->data);
			auto s3 = static_cast<IntHashSet *>(newSetData->data);
			s3->insert(s1->begin(), s1->end());
			s3->insert(s2->begin(), s2->end());
			break;
		}
		case DefaultClass::floatClassId: {
			auto s1 = static_cast<FloatHashSet *>(set1Data->data);
			auto s2 = static_cast<FloatHashSet *>(set2Data->data);
			auto s3 = static_cast<FloatHashSet *>(newSetData->data);
			s3->insert(s1->begin(), s1->end());
			s3->insert(s2->begin(), s2->end());
			break;
		}
		case DefaultClass::stringClassId: {
			auto s1 = static_cast<StringHashSet *>(set1Data->data);
			auto s2 = static_cast<StringHashSet *>(set2Data->data);
			auto s3 = static_cast<StringHashSet *>(newSetData->data);
			for (auto item : *s1) {
				item->retain();
				s3->insert(item);
			}
			for (auto item : *s2) {
				if (s3->insert(item).second) {
					item->retain();
				}
			}
			break;
		}
		default: {
			auto s1 = static_cast<ObjectHashSet *>(set1Data->data);
			auto s2 = static_cast<ObjectHashSet *>(set2Data->data);
			auto s3 = static_cast<ObjectHashSet *>(newSetData->data);
			for (auto item : *s1) {
				item->retain();
				s3->insert(item);
			}
			for (auto item : *s2) {
				if (s3->insert(item).second)
					item->retain();
			}
			break;
		}
	}
	return newObj;
}

AObject *intersect(NativeFuncInData) {
	auto set1Data = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto set2Data = static_cast<AUnorderedSet *>(args[1]->data->data);

	AObject *newObj = constructor(notifier, args[0]->type, set1Data->type);
	auto newSetData = static_cast<AUnorderedSet *>(newObj->data->data);

	switch (set1Data->type) {
		case DefaultClass::intClassId: {
			auto s1 = static_cast<IntHashSet *>(set1Data->data);
			auto s2 = static_cast<IntHashSet *>(set2Data->data);
			auto s3 = static_cast<IntHashSet *>(newSetData->data);
			for (auto item : *s1) {
				if (s2->find(item) != s2->end())
					s3->insert(item);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto s1 = static_cast<FloatHashSet *>(set1Data->data);
			auto s2 = static_cast<FloatHashSet *>(set2Data->data);
			auto s3 = static_cast<FloatHashSet *>(newSetData->data);
			for (auto item : *s1) {
				if (s2->find(item) != s2->end())
					s3->insert(item);
			}
			break;
		}
		case DefaultClass::stringClassId:
		default: {
			auto s1 = static_cast<StringHashSet *>(set1Data->data);
			auto s2 = static_cast<StringHashSet *>(set2Data->data);
			auto s3 = static_cast<StringHashSet *>(newSetData->data);
			for (auto item : *s1) {
				if (s2->find(item) != s2->end()) {
					item->retain();
					s3->insert(item);
				}
			}
			break;
		}
	}
	return newObj;
}

AObject *difference(NativeFuncInData) {
	auto set1Data = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto set2Data = static_cast<AUnorderedSet *>(args[1]->data->data);

	AObject *newObj = constructor(notifier, args[0]->type, set1Data->type);
	auto newSetData = static_cast<AUnorderedSet *>(newObj->data->data);

	switch (set1Data->type) {
		case DefaultClass::intClassId: {
			auto s1 = static_cast<IntHashSet *>(set1Data->data);
			auto s2 = static_cast<IntHashSet *>(set2Data->data);
			auto s3 = static_cast<IntHashSet *>(newSetData->data);
			for (auto item : *s1) {
				if (s2->find(item) == s2->end())
					s3->insert(item);
			}
			break;
		}
		case DefaultClass::stringClassId:
		default: {
			auto s1 = static_cast<StringHashSet *>(set1Data->data);
			auto s2 = static_cast<StringHashSet *>(set2Data->data);
			auto s3 = static_cast<StringHashSet *>(newSetData->data);
			for (auto item : *s1) {
				if (s2->find(item) == s2->end()) {
					item->retain();
					s3->insert(item);
				}
			}
			break;
		}
	}
	return newObj;
}

AObject *clear(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			size_t setSize = set->size();
			set->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(setSize * 32));
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			size_t setSize = set->size();
			set->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(setSize * 32));
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			size_t setSize = set->size();
			for (auto &key : *set) {
				notifier.release(key);
			}
			set->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(setSize * 32));
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			size_t setSize = set->size();
			for (auto &key : *set) {
				notifier.release(key);
			}
			set->clear();
			notifier.addManagedMemory(-static_cast<int64_t>(setSize * 32));
		}
	}

	return nullptr;
}

std::string to_string(ANotifier &notifier, AObject *obj) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(obj->data->data);
	std::string str = "{";
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (int64_t value : *set) {
				if (isFirst) {
					str += std::to_string(value);
					isFirst = false;
					continue;
				}
				str += ", " + std::to_string(value);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (double value : *set) {
				if (isFirst) {
					str += std::to_string(value);
					isFirst = false;
					continue;
				}
				str += ", " + std::to_string(value);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (AObject *value : *set) {
				if (isFirst) {
					str += value->str->data;
					isFirst = false;
					continue;
				}
				str += ", " + std::string(value->str->data);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return "{}";
			}
			bool isFirst = true;
			for (AObject *value : *set) {
				if (isFirst) {
					str += DefaultFunction::to_string(notifier, value);
					isFirst = false;
					continue;
				}
				str += ", " + DefaultFunction::to_string(notifier, value);
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
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	AObject *newObj = constructor(notifier, args[0]->type, unorderedSetData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_SET;
	auto newSetData = static_cast<AUnorderedSet *>(newObj->data->data);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto s1 = static_cast<IntHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<IntHashSet *>(newSetData->data);
			s2->insert(s1->begin(), s1->end());
			notifier.addManagedMemory(s1->size() * 32);
			break;
		}
		case DefaultClass::floatClassId: {
			auto s1 = static_cast<FloatHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<FloatHashSet *>(newSetData->data);
			s2->insert(s1->begin(), s1->end());
			notifier.addManagedMemory(s1->size() * 32);
			break;
		}
		case DefaultClass::stringClassId: {
			auto s1 = static_cast<StringHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<StringHashSet *>(newSetData->data);
			for (auto item : *s1) {
				item->retain();
				s2->insert(item);
			}
			notifier.addManagedMemory(s1->size() * 32);
			break;
		}
		default: {
			auto s1 = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<ObjectHashSet *>(newSetData->data);
			for (auto item : *s1) {
				item->retain();
				s2->insert(item);
			}
			notifier.addManagedMemory(s1->size() * 32);
			break;
		}
	}
	return newObj;
}

AObject *filter(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto funcObject = args[1];

	AObject *newObj = constructor(notifier, args[0]->type, unorderedSetData->type);
	newObj->flags |= AObject::Flags::OBJ_IS_SET;
	auto newSetData = static_cast<AUnorderedSet *>(newObj->data->data);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto s1 = static_cast<IntHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<IntHashSet *>(newSetData->data);
			for (int64_t val : *s1) {
				auto item = notifier.createInt(val);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					s2->insert(val);
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto s1 = static_cast<FloatHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<FloatHashSet *>(newSetData->data);
			for (double val : *s1) {
				auto item = notifier.createFloat(val);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					s2->insert(val);
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::stringClassId: {
			auto s1 = static_cast<StringHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<StringHashSet *>(newSetData->data);
			for (auto item : *s1) {
				auto res = notifier.callFunctionObject(funcObject, item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					item->retain();
					s2->insert(item);
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
		default: {
			auto s1 = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto s2 = static_cast<ObjectHashSet *>(newSetData->data);
			for (auto item : *s1) {
				auto res = notifier.callFunctionObject(funcObject, item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) {
					item->retain();
					s2->insert(item);
					notifier.addManagedMemory(32);
				}
				notifier.release(res);
			}
			break;
		}
	}
	return newObj;
}

static inline ClassId getSetGenericKey(ANotifier &notifier, ClassId classId) {
	if (classId < notifier.vm->data.classes.size()) {
		auto clazz = notifier.vm->data.classes[classId];
		if (clazz && clazz->genericType.size > 0) {
			return notifier.vm->data.allGenericType[clazz->genericType.offset];
		}
	}
	return DefaultClass::anyClassId;
}

AObject *map(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto funcObject = args[1];
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getSetGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t val : *set) {
				auto item = notifier.createInt(val);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double val : *set) {
				auto item = notifier.createFloat(val);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (auto item : *set) {
				auto res = notifier.callFunctionObject(funcObject, item);
				if (notifier.hasException()) return nullptr;
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
			}
			break;
		}
	}
	return newArr;
}

AObject *first(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				notifier.throwException("Set is empty");
				return nullptr;
			}
			return notifier.createInt(*set->begin());
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				notifier.throwException("Set is empty");
				return nullptr;
			}
			return notifier.createFloat(*set->begin());
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				notifier.throwException("Set is empty");
				return nullptr;
			}
			AObject *val = *set->begin();
			if (!val) return notifier.getNullObject();
			switch (val->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(val->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(val->f);
				default:
					return val;
			}
		}
	}
}

AObject *first_or_null(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->empty()) return notifier.getNullObject();
			return notifier.createInt(*set->begin());
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			if (set->empty()) return notifier.getNullObject();
			return notifier.createFloat(*set->begin());
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			if (set->empty()) return notifier.getNullObject();
			AObject *val = *set->begin();
			if (!val) return notifier.getNullObject();
			switch (val->type) {
				case DefaultClass::intClassId:
					return notifier.createInt(val->i);
				case DefaultClass::floatClassId:
					return notifier.createFloat(val->f);
				default:
					return val;
			}
		}
	}
}

AObject *any(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	bool notEmpty = false;
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId:
			notEmpty = !static_cast<IntHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::floatClassId:
			notEmpty = !static_cast<FloatHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::stringClassId:
			notEmpty = !static_cast<StringHashSet *>(unorderedSetData->data)->empty();
			break;
		default:
			notEmpty = !static_cast<ObjectHashSet *>(unorderedSetData->data)->empty();
			break;
	}
	return notifier.createBool(notEmpty);
}

AObject *none(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	bool isEmpty = true;
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId:
			isEmpty = static_cast<IntHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::floatClassId:
			isEmpty = static_cast<FloatHashSet *>(unorderedSetData->data)->empty();
			break;
		case DefaultClass::stringClassId:
			isEmpty = static_cast<StringHashSet *>(unorderedSetData->data)->empty();
			break;
		default:
			isEmpty = static_cast<ObjectHashSet *>(unorderedSetData->data)->empty();
			break;
	}
	return notifier.createBool(isEmpty);
}

AObject *any_fn(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto func = args[1];
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t val : *set) {
				auto item = notifier.createInt(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(true);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double val : *set) {
				auto item = notifier.createFloat(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(true);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (auto item : *set) {
				auto res = notifier.callFunctionObject(func, item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(true);
			}
			break;
		}
	}
	return notifier.createBool(false);
}

AObject *all_fn(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto func = args[1];
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t val : *set) {
				auto item = notifier.createInt(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (!ok) return notifier.createBool(false);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double val : *set) {
				auto item = notifier.createFloat(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (!ok) return notifier.createBool(false);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (auto item : *set) {
				auto res = notifier.callFunctionObject(func, item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (!ok) return notifier.createBool(false);
			}
			break;
		}
	}
	return notifier.createBool(true);
}

AObject *none_fn(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	auto func = args[1];
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			for (int64_t val : *set) {
				auto item = notifier.createInt(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(false);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			for (double val : *set) {
				auto item = notifier.createFloat(val);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(false);
			}
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (auto item : *set) {
				auto res = notifier.callFunctionObject(func, item);
				if (notifier.hasException()) return nullptr;
				bool ok = (res == notifier.getTrueObject());
				notifier.release(res);
				if (ok) return notifier.createBool(false);
			}
			break;
		}
	}
	return notifier.createBool(true);
}

} // namespace set
} // namespace Libs
} // namespace Autolang

#endif
