#ifndef LIBS_SET_CPP
#define LIBS_SET_CPP

#include "set.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/Type.hpp"


namespace AutoLang {
namespace Libs {
namespace set {

template <typename SetType, bool ReleaseKey>
static void destroySet(ANotifier &notifier, void *unorderedSetData) {
	auto unorderedSetData_ = static_cast<AUnorderedSet *>(unorderedSetData);
	auto set = static_cast<SetType *>(unorderedSetData_->data);

	if constexpr (ReleaseKey) {
		for (auto &key : *set) {
			notifier.release(key);
		}
	}

	delete set;
	delete unorderedSetData_;
}

AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	ClassId keyId = args[1]->i;
	return constructor(notifier, classId, keyId);
}

AObject *constructor(ANotifier &notifier, ClassId classId, ClassId keyId) {
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

AObject *insert(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	AObject *element = args[1];

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			if (element->type != DefaultClass::intClassId) {
				notifier.throwException("Set.add: element must be Int");
				return nullptr;
			}
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			set->insert(element->i);
			break;
		}

		case DefaultClass::floatClassId: {
			if (element->type != DefaultClass::floatClassId) {
				notifier.throwException("Set.add: element must be Float");
				return nullptr;
			}
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			set->insert(element->f);
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
			}
			break;
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it == set->end()) {
				element->retain();
				set->insert(element);
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
			if (it != set->end())
				set->erase(it);
			break;
		}

		case DefaultClass::floatClassId: {
			auto set = static_cast<FloatHashSet *>(unorderedSetData->data);
			auto it = set->find(element->f);
			if (it != set->end())
				set->erase(it);
			break;
		}

		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it != set->end()) {
				notifier.release(*it);
				set->erase(it);
			}
			break;
		}

		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			auto it = set->find(element);
			if (it != set->end()) {
				notifier.release(*it);
				set->erase(it);
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

AObject *clear(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);

	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			static_cast<IntHashSet *>(unorderedSetData->data)->clear();
			break;
		}
		case DefaultClass::floatClassId: {
			static_cast<FloatHashSet *>(unorderedSetData->data)->clear();
			break;
		}
		case DefaultClass::stringClassId: {
			auto set = static_cast<StringHashSet *>(unorderedSetData->data);
			for (auto &key : *set) {
				notifier.release(key);
			}
			set->clear();
			break;
		}
		default: {
			auto set = static_cast<ObjectHashSet *>(unorderedSetData->data);
			for (auto &key : *set) {
				notifier.release(key);
			}
			set->clear();
		}
	}

	return nullptr;
}

AObject *to_string(NativeFuncInData) {
	auto unorderedSetData = static_cast<AUnorderedSet *>(args[0]->data->data);
	std::string str = "{";
	switch (unorderedSetData->type) {
		case DefaultClass::intClassId: {
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return notifier.createString("{}");
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
			auto set = static_cast<IntHashSet *>(unorderedSetData->data);
			if (set->empty()) {
				return notifier.createString("{}");
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
				return notifier.createString("{}");
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
				return notifier.createString("{}");
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
	return notifier.createString(str);
}

} // namespace set
} // namespace Libs
} // namespace AutoLang

#endif