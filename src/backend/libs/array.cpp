#ifndef LIBS_LIST_CPP
#define LIBS_LIST_CPP

#include "array.hpp"
#include "backend/vm/ANotifier.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include "backend/libs/set.hpp"
#include "backend/libs/map.hpp"

namespace Autolang {
class ACompiler;
namespace Libs {
namespace array {

AObject *add(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;

	if (array->size == 0 && array->maxSize == 0) {
		int64_t delta = static_cast<int64_t>(1 * 8);
		if (notifier.getCurrentManagedMemory() + delta >
		    notifier.getMaxManagedMemory()) {
			notifier.throwMemoryLimitExceeded(
			    notifier.getCurrentManagedMemory() + delta);
			return nullptr;
		}
		array->reallocate(1);
		array->size = 1;
		switch (array->key) {
			case DefaultClass::intClassId: {
				array->intData[0] =
				    (args[1]->type == DefaultClass::floatClassId)
				        ? static_cast<int64_t>(args[1]->f)
				        : args[1]->i;
				break;
			}
			case DefaultClass::floatClassId: {
				array->floatData[0] =
				    (args[1]->type == DefaultClass::intClassId)
				        ? static_cast<double>(args[1]->i)
				        : args[1]->f;
				break;
			}
			default: {
				args[1]->retain();
				array->objData[0] = args[1];
				break;
			}
		}
		notifier.addManagedMemory(delta);
		return nullptr;
	}

	if (array->size == array->maxSize) {
		uint64_t oldMax = array->maxSize;
		constexpr uint64_t maxCapacity = std::numeric_limits<uint32_t>::max();
		uint64_t newMax = (oldMax == 0) ? 1 : oldMax * 2;
		if (newMax > maxCapacity) {
			newMax = maxCapacity;
		}
		if (newMax == oldMax) {
			notifier.throwException("Array.add: maximum capacity exceeded3");
			return nullptr;
		}
		int64_t delta = static_cast<int64_t>((newMax - oldMax) * 8);
		if (notifier.getCurrentManagedMemory() + delta >
		    notifier.getMaxManagedMemory()) {
			notifier.throwMemoryLimitExceeded(
			    notifier.getCurrentManagedMemory() + delta);
			return nullptr;
		}
		array->reallocate(newMax);
		notifier.addManagedMemory(delta);
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			array->intData[array->size++] =
			    (args[1]->type == DefaultClass::floatClassId)
			        ? static_cast<int64_t>(args[1]->f)
			        : args[1]->i;
			break;
		}
		case DefaultClass::floatClassId: {
			array->floatData[array->size++] =
			    (args[1]->type == DefaultClass::intClassId)
			        ? static_cast<double>(args[1]->i)
			        : args[1]->f;
			break;
		}
		default: {
			args[1]->retain();
			array->objData[array->size++] = args[1];
			break;
		}
	}

	return nullptr;
}

AObject *remove(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;

	if (args[1]->type != Autolang::DefaultClass::intClassId) {
		notifier.throwException("Array.remove: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= array->size) {
		notifier.throwException("Array.remove: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (int i = index; i < array->size - 1; ++i) {
				array->intData[i] = array->intData[i + 1];
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (int i = index; i < array->size - 1; ++i) {
				array->floatData[i] = array->floatData[i + 1];
			}
			break;
		}
		default: {
			notifier.release(array->objData[index]);
			for (int i = index; i < array->size - 1; ++i) {
				array->objData[i] = array->objData[i + 1];
			}
			array->objData[array->size - 1] = nullptr;
			break;
		}
	}
	array->size--;

	if (array->maxSize > 1 && array->size <= array->maxSize / 4) {
		size_t oldMax = array->maxSize;
		size_t newMax = oldMax / 2;
		if (newMax < 1)
			newMax = 1;
		array->reallocate(newMax);
		notifier.addManagedMemory(static_cast<int64_t>((newMax - oldMax) * 8));
	}

	return nullptr;
}

AObject *reserve(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;
	int64_t capacity = args[1]->i;
	const int64_t maxCapacity =
	    static_cast<int64_t>(std::numeric_limits<uint32_t>::max());

	if (capacity < 0) {
		notifier.throwException("Array.reserve: capacity must be non-negative");
		return nullptr;
	}

	if (capacity > maxCapacity) {
		notifier.throwException(
		    "Array.reserve: capacity exceeds maximum supported size (" +
		    std::to_string(maxCapacity) + ")");
		return nullptr;
	}

	if (capacity > array->maxSize) {
		uint32_t oldMax = array->maxSize;
		int64_t delta = static_cast<int64_t>((capacity - oldMax) * 8);
		if (notifier.getCurrentManagedMemory() + delta >
		    notifier.getMaxManagedMemory()) {
			notifier.throwMemoryLimitExceeded(
			    notifier.getCurrentManagedMemory() + delta);
			return nullptr;
		}
		array->reallocate(static_cast<uint32_t>(capacity));
		notifier.addManagedMemory(delta);
	}
	return nullptr;
}

AObject *insert(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;
	int64_t index = args[1]->i;
	AObject *value = args[2];

	if (index < 0 || index > array->size) {
		notifier.throwException("Array.insert: index out of range");
		return nullptr;
	}

	if (array->size == array->maxSize) {
		uint64_t oldMax = array->maxSize;
		constexpr uint64_t maxCapacity = std::numeric_limits<uint32_t>::max();
		uint64_t newMax = (oldMax == 0) ? 1 : oldMax * 2;
		if (newMax > maxCapacity) {
			newMax = maxCapacity;
		}
		if (newMax == oldMax) {
			notifier.throwException("Array.insert: maximum capacity exceeded");
			return nullptr;
		}
		int64_t delta = static_cast<int64_t>((newMax - oldMax) * 8);
		if (notifier.getCurrentManagedMemory() + delta >
		    notifier.getMaxManagedMemory()) {
			notifier.throwMemoryLimitExceeded(
			    notifier.getCurrentManagedMemory() + delta);
			return nullptr;
		}
		array->reallocate(newMax);
		notifier.addManagedMemory(delta);
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (int64_t i = array->size; i > index; --i) {
				array->intData[i] = array->intData[i - 1];
			}
			array->intData[index] = value->i;
			break;
		}
		case DefaultClass::floatClassId: {
			for (int64_t i = array->size; i > index; --i) {
				array->floatData[i] = array->floatData[i - 1];
			}
			array->floatData[index] =
			    (value->type == DefaultClass::intClassId)
			        ? static_cast<double>(value->i)
			        : value->f;
			break;
		}
		default: {
			for (int64_t i = array->size; i > index; --i) {
				array->objData[i] = array->objData[i - 1];
			}
			value->retain();
			array->objData[index] = value;
			break;
		}
	}

	array->size++;
	return nullptr;
}

AObject *pop(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;
	if (array->size == 0) {
		return notifier.getNullObject();
	}

	array->size--;
	switch (array->key) {
		case DefaultClass::intClassId: {
			return notifier.createInt(array->intData[array->size]);
		}
		case DefaultClass::floatClassId: {
			return notifier.createFloat(array->floatData[array->size]);
		}
		default: {
			AObject *lastObj = array->objData[array->size];
			array->objData[array->size] = nullptr;
			return lastObj;
		}
	}
}

AObject *for_each(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) {
					return nullptr;
				}
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, item);
				notifier.release(item);
				if (notifier.hasException()) {
					return nullptr;
				}
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				notifier.callFunctionObject(funcObject, array->objData[i]);
				if (notifier.hasException()) {
					return nullptr;
				}
			}
			break;
		}
	}

	return nullptr;
}

AObject *for_each_with_index(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	auto indexObj = notifier.createInt(0);
	indexObj->retain();

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, item, indexObj);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, item, indexObj);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				notifier.callFunctionObject(funcObject, array->objData[i],
				                            indexObj);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
	}

	notifier.release(indexObj);

	return nullptr;
}

AObject *for_each_indexed(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	auto indexObj = notifier.createInt(0);
	indexObj->retain();

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, indexObj, item);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				notifier.callFunctionObject(funcObject, indexObj, item);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				notifier.callFunctionObject(funcObject, indexObj,
				                            array->objData[i]);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				indexObj->i++;
			}
			break;
		}
	}

	notifier.release(indexObj);

	return nullptr;
}

AObject *filter(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	auto newArr = notifier.createArray(arr->type, array->key);
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				auto value = notifier.callFunctionObject(funcObject, item);
				if (notifier.hasException()) {
					notifier.release(item);
					return nullptr;
				}
				if (value == notifier.getTrueObject()) {
					notifier.arrayAdd(newArr, item);
				}
				notifier.release(item);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				auto value = notifier.callFunctionObject(funcObject, item);
				if (notifier.hasException()) {
					notifier.release(item);
					return nullptr;
				}
				if (value == notifier.getTrueObject()) {
					notifier.arrayAdd(newArr, item);
				}
				notifier.release(item);
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				auto obj = array->objData[i];
				auto value = notifier.callFunctionObject(funcObject, obj);
				if (notifier.hasException()) {
					return nullptr;
				}
				if (value == notifier.getTrueObject()) {
					notifier.arrayAdd(newArr, obj);
				}
			}
			break;
		}
	}

	return newArr;
}

AObject *sort(NativeFuncInData) {
	auto arr = args[0];
	auto comparator = args[1];
	auto array = arr->array;

	if (array->size <= 1) {
		return nullptr;
	}

	bool hasErr = false;

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::stable_sort(array->intData, array->intData + array->size,
			                 [&](int64_t a, int64_t b) {
				                 if (hasErr) {
					                 return false;
				                 }
				                 auto objA = notifier.createInt(a);
				                 objA->retain();
				                 auto objB = notifier.createInt(b);
				                 objB->retain();
				                 AObject *result = notifier.callFunctionObject(
				                     comparator, objA, objB);
				                 notifier.release(objA);
				                 notifier.release(objB);
				                 if (notifier.hasException()) {
					                 hasErr = true;
					                 return false;
				                 }
				                 bool val = result->i < 0;
				                 notifier.release(result);
				                 return val;
			                 });
			break;
		}
		case DefaultClass::floatClassId: {
			std::stable_sort(array->floatData, array->floatData + array->size,
			                 [&](double a, double b) {
				                 if (hasErr) {
					                 return false;
				                 }
				                 auto objA = notifier.createFloat(a);
				                 objA->retain();
				                 auto objB = notifier.createFloat(b);
				                 objB->retain();
				                 AObject *result = notifier.callFunctionObject(
				                     comparator, objA, objB);
				                 notifier.release(objA);
				                 notifier.release(objB);
				                 if (notifier.hasException()) {
					                 hasErr = true;
					                 return false;
				                 }
				                 bool val = result->i < 0;
				                 notifier.release(result);
				                 return val;
			                 });
			break;
		}
		default: {
			std::stable_sort(array->objData, array->objData + array->size,
			                 [&](AObject *a, AObject *b) {
				                 if (hasErr) {
					                 return false;
				                 }
				                 AObject *result = notifier.callFunctionObject(
				                     comparator, a, b);
				                 if (notifier.hasException()) {
					                 hasErr = true;
					                 return false;
				                 }
				                 bool val = result->i < 0;
				                 notifier.release(result);
				                 return val;
			                 });
			break;
		}
	}

	return nullptr;
}

AObject *sort_default(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;

	if (array->size <= 1) {
		return nullptr;
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::stable_sort(array->intData, array->intData + array->size);
			break;
		}
		case DefaultClass::floatClassId: {
			std::stable_sort(array->floatData, array->floatData + array->size);
			break;
		}
		case DefaultClass::stringClassId: {
			std::stable_sort(array->objData, array->objData + array->size,
			                 [](AObject *a, AObject *b) {
				                 if (!a || !b) return a != nullptr;
				                 auto sa = std::string_view(a->str->data, a->str->size);
				                 auto sb = std::string_view(b->str->data, b->str->size);
				                 return sa < sb;
			                 });
			break;
		}
		default: {
			if (array->objData && array->size > 1 && array->objData[0]) {
				if (array->objData[0]->type == DefaultClass::intClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 return a->i < b->i;
					                 });
				} else if (array->objData[0]->type == DefaultClass::floatClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 return a->f < b->f;
					                 });
				} else if (array->objData[0]->type == DefaultClass::stringClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 auto sa = std::string_view(a->str->data, a->str->size);
						                 auto sb = std::string_view(b->str->data, b->str->size);
						                 return sa < sb;
					                 });
				}
			}
			break;
		}
	}

	return nullptr;
}

AObject *slice(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t len = array->size;

	int64_t from = args[1]->i;
	int64_t to = args[2]->i;

	if (from < 0)
		from += len;
	if (to < 0)
		to += len;

	if (from < 0)
		from = 0;
	if (to > len)
		to = len;

	if (from >= to || from >= len) {
		return notifier.createArray(arr->type, array->key);
	}

	auto newArr = notifier.createArray(arr->type, array->key, to - from);

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::copy(array->intData + from, array->intData + to,
			          newArr->array->intData);
			newArr->array->size = to - from;
			break;
		}
		case DefaultClass::floatClassId: {
			std::copy(array->floatData + from, array->floatData + to,
			          newArr->array->floatData);
			newArr->array->size = to - from;
			break;
		}
		default: {
			for (int64_t i = from; i < to; ++i) {
				notifier.arrayAdd(newArr, array->objData[i]);
			}
			break;
		}
	}

	return newArr;
}

AObject *reversed(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t len = array->size;

	if (len == 0) {
		return notifier.createArray(arr->type, array->key);
	}

	auto newArr = notifier.createArray(arr->type, array->key, len);

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::reverse_copy(array->intData, array->intData + len,
			                  newArr->array->intData);
			newArr->array->size = len;
			break;
		}
		case DefaultClass::floatClassId: {
			std::reverse_copy(array->floatData, array->floatData + len,
			                  newArr->array->floatData);
			newArr->array->size = len;
			break;
		}
		default: {
			for (int64_t i = len - 1; i >= 0; --i) {
				notifier.arrayAdd(newArr, array->objData[i]);
			}
			break;
		}
	}

	return newArr;
}

AObject *contains(NativeFuncInData) {
	auto arr = args[0];
	auto obj = args[1];
	auto array = arr->array;

	if (array->size == 0) {
		return notifier.createBool(false);
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			if (obj->type == DefaultClass::intClassId) {
				int64_t target = obj->i;
				for (size_t i = 0; i < array->size; ++i) {
					if (array->intData[i] == target) {
						return notifier.createBool(true);
					}
				}
			}
			return notifier.createBool(false);
		}
		case DefaultClass::floatClassId: {
			double target = (obj->type == DefaultClass::intClassId)
			                    ? static_cast<double>(obj->i)
			                    : obj->f;
			for (size_t i = 0; i < array->size; ++i) {
				if (array->floatData[i] == target) {
					return notifier.createBool(true);
				}
			}
			return notifier.createBool(false);
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				if (DefaultFunction::op_eqeq(array->objData[i], obj)) {
					return notifier.createBool(true);
				}
			}
			return notifier.createBool(false);
		}
	}
}

AObject *size(NativeFuncInData) {
	auto obj = args[0];
	return notifier.createInt(static_cast<int64_t>(obj->array->size));
}

AObject *is_empty(NativeFuncInData) {
	auto obj = args[0];
	return notifier.createBool(obj->array->size == 0);
}

AObject *index_of(NativeFuncInData) {
	auto arr = args[0];
	auto target = args[1];
	auto array = arr->array;

	uint32_t memberSize = array->size;
	if (memberSize == 0) {
		return notifier.createInt(-1);
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			if (target->type == DefaultClass::intClassId) {
				int64_t val = target->i;
				for (uint32_t i = 0; i < memberSize; ++i) {
					if (array->intData[i] == val) {
						return notifier.createInt(i);
					}
				}
			}
			return notifier.createInt(-1);
		}
		case DefaultClass::floatClassId: {
			double val = (target->type == DefaultClass::intClassId)
			                 ? static_cast<double>(target->i)
			                 : target->f;
			for (uint32_t i = 0; i < memberSize; ++i) {
				if (array->floatData[i] == val) {
					return notifier.createInt(i);
				}
			}
			return notifier.createInt(-1);
		}
		default: {
			for (uint32_t i = 0; i < memberSize; ++i) {
				AObject *currentItem = array->objData[i];
				if (DefaultFunction::op_eqeq(currentItem, target)) {
					return notifier.createInt(i);
				}
			}
			return notifier.createInt(-1);
		}
	}
}

AObject *get(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;

	if (args[1]->type != Autolang::DefaultClass::intClassId) {
		notifier.throwException("Array.get: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= array->size) {
		notifier.throwException("Array.get: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	switch (array->key) {
		case DefaultClass::intClassId:
			return notifier.createInt(array->intData[index]);
		case DefaultClass::floatClassId:
			return notifier.createFloat(array->floatData[index]);
		default: {
			AObject *value = array->objData[index];
			switch (value->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
		}
	}
}

AObject *set(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;

	if (args[1]->type != Autolang::DefaultClass::intClassId) {
		notifier.throwException("Array.set: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= array->size) {
		notifier.throwException("Array.set: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			array->intData[index] = args[2]->i;
			break;
		}
		case DefaultClass::floatClassId: {
			array->floatData[index] =
			    (args[2]->type == DefaultClass::intClassId)
			        ? static_cast<double>(args[2]->i)
			        : args[2]->f;
			break;
		}
		default: {
			notifier.release(array->objData[index]);
			args[2]->retain();
			array->objData[index] = args[2];
			break;
		}
	}
	return nullptr;
}

AObject *clear(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;

	switch (array->key) {
		case DefaultClass::intClassId:
		case DefaultClass::floatClassId:
			break;
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				notifier.release(array->objData[i]);
				array->objData[i] = nullptr;
			}
			break;
		}
	}
	array->size = 0;

	if (array->maxSize > 8) {
		array->reallocate(8);
	}

	return nullptr;
}

std::string to_string(ANotifier &notifier, AObject *obj) {
	auto array = obj->array;
	if (array->size == 0) {
		return "[]";
	}
	std::string str = "[";
	switch (array->key) {
		case DefaultClass::intClassId: {
			str += std::to_string(array->intData[0]);
			for (size_t i = 1; i < array->size; ++i) {
				str += ", " + std::to_string(array->intData[i]);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			auto formatDouble = [](double val) {
				std::string s = std::to_string(val);
				s.erase(s.find_last_not_of('0') + 1, std::string::npos);
				if (s.back() == '.')
					s += '0';
				return s;
			};
			str += formatDouble(array->floatData[0]);
			for (size_t i = 1; i < array->size; ++i) {
				str += ", " + formatDouble(array->floatData[i]);
			}
			break;
		}
		default: {
			str += DefaultFunction::to_string(notifier, array->objData[0]);
			for (size_t i = 1; i < array->size; ++i) {
				str += ", " + DefaultFunction::to_string(notifier,
				                                         array->objData[i]);
			}
			break;
		}
	}
	str += ']';
	return str;
}

AObject *to_string(NativeFuncInData) {
	return notifier.createString(to_string(notifier, args[0]));
}

static inline AObject *getItem(ANotifier &notifier, AArray *array, size_t index) {
	switch (array->key) {
		case DefaultClass::intClassId:
			return notifier.createInt(array->intData[index]);
		case DefaultClass::floatClassId:
			return notifier.createFloat(array->floatData[index]);
		default: {
			AObject *value = array->objData[index];
			if (!value) return notifier.getNullObject();
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

// args[0] = Array
// args[1] = separator String (default ", ")
// args[2] = prefix String (default "")
// args[3] = postfix String (default "")
// args[4] = limit Int (default -1)
// args[5] = truncated String (default "...")
// args[6] = transform ((T) -> Any?) (default null)
AObject *join_to_string(NativeFuncInData) {
	auto array = args[0]->array;
	std::string separator = ", ";
	std::string prefix = "";
	std::string postfix = "";
	int64_t limit = -1;
	std::string truncated = "...";
	AObject *transform = nullptr;

	if (argSize >= 2 && args[1]->type == DefaultClass::stringClassId) {
		separator = std::string(args[1]->str->data, args[1]->str->size);
	}
	if (argSize >= 3 && args[2]->type == DefaultClass::stringClassId) {
		prefix = std::string(args[2]->str->data, args[2]->str->size);
	}
	if (argSize >= 4 && args[3]->type == DefaultClass::stringClassId) {
		postfix = std::string(args[3]->str->data, args[3]->str->size);
	}
	if (argSize >= 5 && args[4]->type == DefaultClass::intClassId) {
		limit = args[4]->i;
	}
	if (argSize >= 6 && args[5]->type == DefaultClass::stringClassId) {
		truncated = std::string(args[5]->str->data, args[5]->str->size);
	}
	if (argSize >= 7 && args[6] && args[6]->type != DefaultClass::nullClassId) {
		transform = args[6];
	}

	std::string result = prefix;
	size_t sz = array->size;
	size_t count = 0;
	bool wasTruncated = false;

	for (size_t i = 0; i < sz; ++i) {
		if (limit >= 0 && static_cast<int64_t>(count) >= limit) {
			wasTruncated = true;
			break;
		}
		if (count > 0) result += separator;

		auto item = getItem(notifier, array, i);
		if (transform) {
			item->retain();
			auto transformed = notifier.callFunctionObject(transform, item);
			notifier.release(item);
			if (notifier.hasException()) return nullptr;
			result += DefaultFunction::to_string(notifier, transformed);
			notifier.release(transformed);
		} else {
			result += DefaultFunction::to_string(notifier, item);
			notifier.release(item);
		}
		++count;
	}
	if (wasTruncated) {
		if (count > 0) result += separator;
		result += truncated;
	}
	result += postfix;
	return notifier.createString(result);
}

AObject *clone(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	size_t len = array->size;
	auto newArr = notifier.createArray(arr->type, array->key, len);
	switch (array->key) {
		case DefaultClass::intClassId:
			std::copy(array->intData, array->intData + len, newArr->array->intData);
			newArr->array->size = len;
			break;
		case DefaultClass::floatClassId:
			std::copy(array->floatData, array->floatData + len, newArr->array->floatData);
			newArr->array->size = len;
			break;
		default:
			newArr->array->size = 0;
			for (size_t i = 0; i < len; ++i) {
				notifier.arrayAdd(newArr, array->objData[i]);
			}
			break;
	}
	return newArr;
}

AObject *sorted(NativeFuncInData) {
	auto newArr = clone(notifier, args, argSize);
	if (newArr->array->size <= 1) {
		return newArr;
	}
	auto array = newArr->array;
	switch (array->key) {
		case DefaultClass::intClassId:
			std::stable_sort(array->intData, array->intData + array->size);
			break;
		case DefaultClass::floatClassId:
			std::stable_sort(array->floatData, array->floatData + array->size);
			break;
		case DefaultClass::stringClassId:
			std::stable_sort(array->objData, array->objData + array->size,
			                 [](AObject *a, AObject *b) {
				                 if (!a || !b) return a != nullptr;
				                 auto sa = std::string_view(a->str->data, a->str->size);
				                 auto sb = std::string_view(b->str->data, b->str->size);
				                 return sa < sb;
			                 });
			break;
		default: {
			if (array->objData && array->size > 1 && array->objData[0]) {
				if (array->objData[0]->type == DefaultClass::intClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 return a->i < b->i;
					                 });
				} else if (array->objData[0]->type == DefaultClass::floatClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 return a->f < b->f;
					                 });
				} else if (array->objData[0]->type == DefaultClass::stringClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return a != nullptr;
						                 auto sa = std::string_view(a->str->data, a->str->size);
						                 auto sb = std::string_view(b->str->data, b->str->size);
						                 return sa < sb;
					                 });
				}
			}
			break;
		}
	}
	return newArr;
}

AObject *first(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size == 0) {
		notifier.throwException("Array is empty");
		return nullptr;
	}
	return getItem(notifier, array, 0);
}

AObject *first_or_null(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size == 0) {
		return notifier.getNullObject();
	}
	return getItem(notifier, array, 0);
}

AObject *last(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size == 0) {
		notifier.throwException("Array is empty");
		return nullptr;
	}
	return getItem(notifier, array, array->size - 1);
}

AObject *last_or_null(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size == 0) {
		return notifier.getNullObject();
	}
	return getItem(notifier, array, array->size - 1);
}

AObject *take(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t n = args[1]->i;
	if (n <= 0) {
		return notifier.createArray(arr->type, array->key);
	}
	int64_t len = static_cast<int64_t>(array->size);
	if (n > len) n = len;
	auto newArr = notifier.createArray(arr->type, array->key, n);
	switch (array->key) {
		case DefaultClass::intClassId:
			std::copy(array->intData, array->intData + n, newArr->array->intData);
			newArr->array->size = n;
			break;
		case DefaultClass::floatClassId:
			std::copy(array->floatData, array->floatData + n, newArr->array->floatData);
			newArr->array->size = n;
			break;
		default:
			newArr->array->size = 0;
			for (int64_t i = 0; i < n; ++i) {
				notifier.arrayAdd(newArr, array->objData[i]);
			}
			break;
	}
	return newArr;
}

AObject *drop(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t n = args[1]->i;
	int64_t len = static_cast<int64_t>(array->size);
	if (n <= 0) n = 0;
	if (n >= len) {
		return notifier.createArray(arr->type, array->key);
	}
	int64_t rem = len - n;
	auto newArr = notifier.createArray(arr->type, array->key, rem);
	switch (array->key) {
		case DefaultClass::intClassId:
			std::copy(array->intData + n, array->intData + len, newArr->array->intData);
			newArr->array->size = rem;
			break;
		case DefaultClass::floatClassId:
			std::copy(array->floatData + n, array->floatData + len, newArr->array->floatData);
			newArr->array->size = rem;
			break;
		default:
			newArr->array->size = 0;
			for (int64_t i = n; i < len; ++i) {
				notifier.arrayAdd(newArr, array->objData[i]);
			}
			break;
	}
	return newArr;
}

AObject *any(NativeFuncInData) {
	return notifier.createBool(args[0]->array->size > 0);
}

AObject *none(NativeFuncInData) {
	return notifier.createBool(args[0]->array->size == 0);
}

AObject *any_fn(NativeFuncInData) {
	auto array = args[0]->array;
	auto func = args[1];
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(func, array->objData[i]);
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
	auto array = args[0]->array;
	auto func = args[1];
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(func, array->objData[i]);
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
	auto array = args[0]->array;
	auto func = args[1];
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(func, array->objData[i]);
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

AObject *count_fn(NativeFuncInData) {
	auto array = args[0]->array;
	auto func = args[1];
	int64_t count = 0;
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) ++count;
				notifier.release(res);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				auto res = notifier.callFunctionObject(func, item);
				notifier.release(item);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) ++count;
				notifier.release(res);
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(func, array->objData[i]);
				if (notifier.hasException()) return nullptr;
				if (res == notifier.getTrueObject()) ++count;
				notifier.release(res);
			}
			break;
		}
	}
	return notifier.createInt(count);
}

static inline ClassId getArrayGenericKey(ANotifier &notifier, ClassId classId) {
	if (classId < notifier.vm->data.classes.size()) {
		auto clazz = notifier.vm->data.classes[classId];
		if (clazz && clazz->genericType.size > 0) {
			return notifier.vm->data.allGenericType[clazz->genericType.offset];
		}
	}
	return DefaultClass::anyClassId;
}

AObject *map(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size > 0) {
		newArr->array->reallocate(array->size);
	}

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
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
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(funcObject, array->objData[i]);
				if (notifier.hasException()) return nullptr;
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
			}
			break;
		}
	}
	return newArr;
}

AObject *map_indexed(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size > 0) {
		newArr->array->reallocate(array->size);
	}

	auto indexObj = notifier.createInt(0);
	indexObj->retain();

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createInt(array->intData[i]);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, indexObj, item);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
				indexObj->i++;
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < array->size; ++i) {
				auto item = notifier.createFloat(array->floatData[i]);
				item->retain();
				auto res = notifier.callFunctionObject(funcObject, indexObj, item);
				notifier.release(item);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
				indexObj->i++;
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				auto res = notifier.callFunctionObject(funcObject, indexObj, array->objData[i]);
				if (notifier.hasException()) {
					notifier.release(indexObj);
					return nullptr;
				}
				notifier.arrayAdd(newArr, res);
				notifier.release(res);
				indexObj->i++;
			}
			break;
		}
	}
	notifier.release(indexObj);
	return newArr;
}

AObject *reduce(NativeFuncInData) {
	auto arr = args[0];
	auto op = args[1];
	auto array = arr->array;
	if (argSize >= 3) {
		auto initial = args[2];
		AObject *acc = initial;
		acc->retain();

		for (size_t i = 0; i < array->size; ++i) {
			AObject *item = getItem(notifier, array, i);
			item->retain();
			AObject *nextAcc = notifier.callFunctionObject(op, acc, item);
			notifier.release(item);
			notifier.release(acc);
			if (notifier.hasException()) {
				return nullptr;
			}
			acc = nextAcc;
		}
		if (acc && !(acc->flags & AObject::Flags::OBJ_IS_CONST)) {
			--acc->refCount;
		}
		return acc;
	}

	if (array->size == 0) {
		notifier.throwException("Unsupported operation on empty array");
		return nullptr;
	}
	AObject *acc = getItem(notifier, array, 0);
	acc->retain();

	for (size_t i = 1; i < array->size; ++i) {
		AObject *item = getItem(notifier, array, i);
		item->retain();
		AObject *nextAcc = notifier.callFunctionObject(op, acc, item);
		notifier.release(item);
		notifier.release(acc);
		if (notifier.hasException()) {
			return nullptr;
		}
		acc = nextAcc;
	}
	if (acc && !(acc->flags & AObject::Flags::OBJ_IS_CONST)) {
		--acc->refCount;
	}
	return acc;
}

AObject *fold(NativeFuncInData) {
	auto arr = args[0];
	auto initial = args[1];
	auto op = args[2];
	auto array = arr->array;
	AObject *acc = initial;
	acc->retain();

	for (size_t i = 0; i < array->size; ++i) {
		AObject *item = getItem(notifier, array, i);
		item->retain();
		AObject *nextAcc = notifier.callFunctionObject(op, acc, item);
		notifier.release(item);
		notifier.release(acc);
		if (notifier.hasException()) {
			return nullptr;
		}
		acc = nextAcc;
	}
	if (acc && !(acc->flags & AObject::Flags::OBJ_IS_CONST)) {
		--acc->refCount;
	}
	return acc;
}

AObject *sum(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t intSum = 0;
	double floatSum = 0.0;
	bool hasFloat = false;

	for (size_t i = 0; i < array->size; ++i) {
		AObject *item = getItem(notifier, array, i);
		if (!item) continue;
		if (item->type == DefaultClass::floatClassId) {
			hasFloat = true;
			floatSum += item->f;
		} else if (item->type == DefaultClass::intClassId) {
			intSum += item->i;
			floatSum += static_cast<double>(item->i);
		}
	}
	if (hasFloat) {
		return notifier.createFloat(floatSum);
	}
	return notifier.createInt(intSum);
}

static AObject *returnArrayItem(ANotifier &notifier, AArray *array, size_t index) {
	switch (array->key) {
		case DefaultClass::intClassId:
			return notifier.createInt(array->intData[index]);
		case DefaultClass::floatClassId:
			return notifier.createFloat(array->floatData[index]);
		default: {
			AObject *value = array->objData[index];
			if (!value) return notifier.createNull();
			switch (value->type) {
				case Autolang::DefaultClass::intClassId:
					return notifier.createInt(value->i);
				case Autolang::DefaultClass::floatClassId:
					return notifier.createFloat(value->f);
				default:
					return value;
			}
		}
	}
}

AObject *average(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (array->size == 0) return notifier.createFloat(0.0);
	double sum = 0.0;
	if (array->key == DefaultClass::intClassId) {
		for (size_t i = 0; i < array->size; ++i) {
			sum += static_cast<double>(array->intData[i]);
		}
	} else if (array->key == DefaultClass::floatClassId) {
		for (size_t i = 0; i < array->size; ++i) {
			sum += array->floatData[i];
		}
	} else {
		for (size_t i = 0; i < array->size; ++i) {
			AObject *item = getItem(notifier, array, i);
			if (!item) continue;
			if (item->type == DefaultClass::floatClassId) sum += item->f;
			else if (item->type == DefaultClass::intClassId) sum += static_cast<double>(item->i);
		}
	}
	return notifier.createFloat(sum / static_cast<double>(array->size));
}

AObject *max_or_null(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (array->size == 0) return notifier.createNull();

	size_t bestIdx = 0;
	if (array->key == DefaultClass::intClassId) {
		int64_t maxVal = array->intData[0];
		for (size_t i = 1; i < array->size; ++i) {
			if (array->intData[i] > maxVal) {
				maxVal = array->intData[i];
				bestIdx = i;
			}
		}
		return notifier.createInt(maxVal);
	} else if (array->key == DefaultClass::floatClassId) {
		double maxVal = array->floatData[0];
		for (size_t i = 1; i < array->size; ++i) {
			if (array->floatData[i] > maxVal) {
				maxVal = array->floatData[i];
				bestIdx = i;
			}
		}
		return notifier.createFloat(maxVal);
	} else {
		for (size_t i = 1; i < array->size; ++i) {
			AObject *best = getItem(notifier, array, bestIdx);
			AObject *cur = getItem(notifier, array, i);
			if (cur && best) {
				if (cur->type == DefaultClass::intClassId && best->type == DefaultClass::intClassId) {
					if (cur->i > best->i) bestIdx = i;
				} else if (cur->type == DefaultClass::floatClassId && best->type == DefaultClass::floatClassId) {
					if (cur->f > best->f) bestIdx = i;
				} else if (cur->type == DefaultClass::stringClassId && best->type == DefaultClass::stringClassId) {
					if (std::string_view(cur->str->data, cur->str->size) > std::string_view(best->str->data, best->str->size)) bestIdx = i;
				}
			}
		}
		return returnArrayItem(notifier, array, bestIdx);
	}
}

AObject *min_or_null(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (array->size == 0) return notifier.createNull();

	size_t bestIdx = 0;
	if (array->key == DefaultClass::intClassId) {
		int64_t minVal = array->intData[0];
		for (size_t i = 1; i < array->size; ++i) {
			if (array->intData[i] < minVal) {
				minVal = array->intData[i];
				bestIdx = i;
			}
		}
		return notifier.createInt(minVal);
	} else if (array->key == DefaultClass::floatClassId) {
		double minVal = array->floatData[0];
		for (size_t i = 1; i < array->size; ++i) {
			if (array->floatData[i] < minVal) {
				minVal = array->floatData[i];
				bestIdx = i;
			}
		}
		return notifier.createFloat(minVal);
	} else {
		for (size_t i = 1; i < array->size; ++i) {
			AObject *best = getItem(notifier, array, bestIdx);
			AObject *cur = getItem(notifier, array, i);
			if (cur && best) {
				if (cur->type == DefaultClass::intClassId && best->type == DefaultClass::intClassId) {
					if (cur->i < best->i) bestIdx = i;
				} else if (cur->type == DefaultClass::floatClassId && best->type == DefaultClass::floatClassId) {
					if (cur->f < best->f) bestIdx = i;
				} else if (cur->type == DefaultClass::stringClassId && best->type == DefaultClass::stringClassId) {
					if (std::string_view(cur->str->data, cur->str->size) < std::string_view(best->str->data, best->str->size)) bestIdx = i;
				}
			}
		}
		return returnArrayItem(notifier, array, bestIdx);
	}
}

AObject *plus(NativeFuncInData) {
	auto arr1 = args[0];
	auto arg2 = args[1];
	auto a1 = arr1->array;

	if (arg2->flags & AObject::Flags::OBJ_IS_ARRAY) {
		auto a2 = arg2->array;
		size_t len1 = a1->size;
		size_t len2 = a2->size;
		size_t totalLen = len1 + len2;
		auto newArr = notifier.createArray(arr1->type, a1->key, totalLen);

		switch (a1->key) {
			case DefaultClass::intClassId:
				std::copy(a1->intData, a1->intData + len1, newArr->array->intData);
				std::copy(a2->intData, a2->intData + len2, newArr->array->intData + len1);
				newArr->array->size = totalLen;
				break;
			case DefaultClass::floatClassId:
				std::copy(a1->floatData, a1->floatData + len1, newArr->array->floatData);
				std::copy(a2->floatData, a2->floatData + len2, newArr->array->floatData + len1);
				newArr->array->size = totalLen;
				break;
			default:
				for (size_t i = 0; i < len1; ++i) {
					notifier.arrayAdd(newArr, a1->objData[i]);
				}
				for (size_t i = 0; i < len2; ++i) {
					notifier.arrayAdd(newArr, a2->objData[i]);
				}
				break;
		}
		return newArr;
	} else {
		size_t len1 = a1->size;
		auto newArr = notifier.createArray(arr1->type, a1->key, len1 + 1);
		switch (a1->key) {
			case DefaultClass::intClassId:
				std::copy(a1->intData, a1->intData + len1, newArr->array->intData);
				newArr->array->intData[len1] = arg2->i;
				newArr->array->size = len1 + 1;
				break;
			case DefaultClass::floatClassId:
				std::copy(a1->floatData, a1->floatData + len1, newArr->array->floatData);
				newArr->array->floatData[len1] = (arg2->type == DefaultClass::intClassId) ? static_cast<double>(arg2->i) : arg2->f;
				newArr->array->size = len1 + 1;
				break;
			default:
				for (size_t i = 0; i < len1; ++i) {
					notifier.arrayAdd(newArr, a1->objData[i]);
				}
				notifier.arrayAdd(newArr, arg2);
				break;
		}
		return newArr;
	}
}

AObject *minus(NativeFuncInData) {
	auto arr1 = args[0];
	auto arg2 = args[1];
	auto a1 = arr1->array;
	auto newArr = notifier.createArray(arr1->type, a1->key);

	if (arg2->flags & AObject::Flags::OBJ_IS_ARRAY) {
		auto a2 = arg2->array;
		switch (a1->key) {
			case DefaultClass::intClassId: {
				std::unordered_set<int64_t> s2(a2->intData, a2->intData + a2->size);
				for (size_t i = 0; i < a1->size; ++i) {
					if (s2.find(a1->intData[i]) == s2.end()) {
						notifier.arrayAdd(newArr, notifier.createInt(a1->intData[i]));
					}
				}
				break;
			}
			case DefaultClass::floatClassId: {
				std::unordered_set<double> s2(a2->floatData, a2->floatData + a2->size);
				for (size_t i = 0; i < a1->size; ++i) {
					if (s2.find(a1->floatData[i]) == s2.end()) {
						notifier.arrayAdd(newArr, notifier.createFloat(a1->floatData[i]));
					}
				}
				break;
			}
			default: {
				for (size_t i = 0; i < a1->size; ++i) {
					bool found = false;
					for (size_t j = 0; j < a2->size; ++j) {
						if (DefaultFunction::op_eqeq(a1->objData[i], a2->objData[j])) {
							found = true;
							break;
						}
					}
					if (!found) {
						notifier.arrayAdd(newArr, a1->objData[i]);
					}
				}
				break;
			}
		}
	} else {
		bool removed = false;
		switch (a1->key) {
			case DefaultClass::intClassId: {
				int64_t target = arg2->i;
				for (size_t i = 0; i < a1->size; ++i) {
					if (!removed && a1->intData[i] == target) {
						removed = true;
						continue;
					}
					notifier.arrayAdd(newArr, notifier.createInt(a1->intData[i]));
				}
				break;
			}
			case DefaultClass::floatClassId: {
				double target = (arg2->type == DefaultClass::intClassId) ? static_cast<double>(arg2->i) : arg2->f;
				for (size_t i = 0; i < a1->size; ++i) {
					if (!removed && a1->floatData[i] == target) {
						removed = true;
						continue;
					}
					notifier.arrayAdd(newArr, notifier.createFloat(a1->floatData[i]));
				}
				break;
			}
			default: {
				for (size_t i = 0; i < a1->size; ++i) {
					if (!removed && DefaultFunction::op_eqeq(a1->objData[i], arg2)) {
						removed = true;
						continue;
					}
					notifier.arrayAdd(newArr, a1->objData[i]);
				}
				break;
			}
		}
	}
	return newArr;
}

AObject *find(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		bool match = (res == notifier.getTrueObject());
		notifier.release(res);
		if (match) {
			return item;
		}
		notifier.release(item);
	}
	return notifier.getNullObject();
}

AObject *find_last(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;

	for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
		auto item = getItem(notifier, array, static_cast<size_t>(i));
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		bool match = (res == notifier.getTrueObject());
		notifier.release(res);
		if (match) {
			return item;
		}
		notifier.release(item);
	}
	return notifier.getNullObject();
}

AObject *filter_not(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	auto newArr = notifier.createArray(arr->type, array->key);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		if (res != notifier.getTrueObject()) {
			notifier.arrayAdd(newArr, item);
		}
		notifier.release(res);
		notifier.release(item);
	}
	return newArr;
}

AObject *filter_not_null(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	auto newArr = notifier.createArray(arr->type, array->key);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		if (item && item != notifier.getNullObject()) {
			notifier.arrayAdd(newArr, item);
		}
	}
	return newArr;
}

AObject *distinct(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	auto newArr = notifier.createArray(arr->type, array->key);

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::unordered_set<int64_t> seen;
			for (size_t i = 0; i < array->size; ++i) {
				if (seen.insert(array->intData[i]).second) {
					notifier.arrayAdd(newArr, notifier.createInt(array->intData[i]));
				}
			}
			break;
		}
		case DefaultClass::floatClassId: {
			std::unordered_set<double> seen;
			for (size_t i = 0; i < array->size; ++i) {
				if (seen.insert(array->floatData[i]).second) {
					notifier.arrayAdd(newArr, notifier.createFloat(array->floatData[i]));
				}
			}
			break;
		}
		default: {
			std::vector<AObject *> seen;
			for (size_t i = 0; i < array->size; ++i) {
				auto obj = array->objData[i];
				bool found = false;
				for (auto s : seen) {
					if (DefaultFunction::op_eqeq(s, obj)) {
						found = true;
						break;
					}
				}
				if (!found) {
					seen.push_back(obj);
					notifier.arrayAdd(newArr, obj);
				}
			}
			break;
		}
	}
	return newArr;
}

AObject *take_last(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t n = args[1]->i;
	int64_t sz = static_cast<int64_t>(array->size);
	if (n <= 0) {
		return notifier.createArray(arr->type, array->key);
	}
	if (n >= sz) {
		return clone(notifier, args, 1);
	}
	int64_t from = sz - n;
	AObject *fromObj = notifier.createInt(from);
	AObject *toObj = notifier.createInt(sz);
	AObject *sliceArgs[3] = {arr, fromObj, toObj};
	return slice(notifier, sliceArgs, 3);
}

AObject *drop_last(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t n = args[1]->i;
	int64_t sz = static_cast<int64_t>(array->size);
	if (n <= 0) {
		return clone(notifier, args, 1);
	}
	if (n >= sz) {
		return notifier.createArray(arr->type, array->key);
	}
	int64_t to = sz - n;
	AObject *fromObj = notifier.createInt(0);
	AObject *toObj = notifier.createInt(to);
	AObject *sliceArgs[3] = {arr, fromObj, toObj};
	return slice(notifier, sliceArgs, 3);
}

AObject *take_while(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	auto newArr = notifier.createArray(arr->type, array->key);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		bool match = (res == notifier.getTrueObject());
		notifier.release(res);
		if (!match) {
			notifier.release(item);
			break;
		}
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	return newArr;
}

AObject *drop_while(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	size_t startIndex = array->size;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		bool match = (res == notifier.getTrueObject());
		notifier.release(res);
		notifier.release(item);
		if (!match) {
			startIndex = i;
			break;
		}
	}

	if (startIndex >= array->size) {
		return notifier.createArray(arr->type, array->key);
	}
	AObject *fromObj = notifier.createInt(static_cast<int64_t>(startIndex));
	AObject *toObj = notifier.createInt(static_cast<int64_t>(array->size));
	AObject *sliceArgs[3] = {arr, fromObj, toObj};
	return slice(notifier, sliceArgs, 3);
}

AObject *chunked(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t chunkSize = args[1]->i;
	auto result = notifier.createArray(DefaultClass::arrayClassId, DefaultClass::arrayClassId);
	if (chunkSize <= 0) {
		return result;
	}
	int64_t sz = static_cast<int64_t>(array->size);
	int64_t i = 0;
	while (i < sz) {
		int64_t end = i + chunkSize;
		if (end > sz) end = sz;
		AObject *fromObj = notifier.createInt(i);
		AObject *toObj = notifier.createInt(end);
		AObject *sliceArgs[3] = {arr, fromObj, toObj};
		auto chunk = slice(notifier, sliceArgs, 3);
		notifier.arrayAdd(result, chunk);
		i = end;
	}
	return result;
}

AObject *to_set(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	AObject *setObj = set::constructor(notifier, DefaultClass::setClassId, array->key);
	setObj->flags |= AObject::Flags::OBJ_IS_SET;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		AObject *addArgs[2] = {setObj, item};
		set::add(notifier, addArgs, 2);
	}
	return setObj;
}

AObject *sum_of(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	int64_t totalInt = 0;
	double totalFloat = 0.0;
	bool hasFloat = false;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) {
			return nullptr;
		}
		if (res) {
			if (res->type == DefaultClass::floatClassId) {
				hasFloat = true;
				totalFloat += res->f;
			} else if (res->type == DefaultClass::intClassId) {
				totalInt += res->i;
				totalFloat += static_cast<double>(res->i);
			}
			notifier.release(res);
		}
	}
	if (hasFloat) {
		return notifier.createFloat(totalFloat);
	}
	return notifier.createInt(totalInt);
}

AObject *is_not_empty(NativeFuncInData) {
	return notifier.createBool(args[0]->array->size != 0);
}

AObject *get_or_null(NativeFuncInData) {
	auto array = args[0]->array;
	int64_t index = args[1]->i;
	if (index < 0 || static_cast<size_t>(index) >= array->size) {
		return notifier.getNullObject();
	}
	return getItem(notifier, array, static_cast<size_t>(index));
}

AObject *get_or_else(NativeFuncInData) {
	auto array = args[0]->array;
	int64_t index = args[1]->i;
	auto defaultValueFn = args[2];
	if (index >= 0 && static_cast<size_t>(index) < array->size) {
		return getItem(notifier, array, static_cast<size_t>(index));
	}
	auto indexObj = notifier.createInt(index);
	indexObj->retain();
	auto res = notifier.callFunctionObject(defaultValueFn, indexObj);
	notifier.release(indexObj);
	return res;
}

AObject *sorted_descending(NativeFuncInData) {
	auto newArr = clone(notifier, args, argSize);
	if (newArr->array->size <= 1) {
		return newArr;
	}
	auto array = newArr->array;
	switch (array->key) {
		case DefaultClass::intClassId:
			std::stable_sort(array->intData, array->intData + array->size, std::greater<int64_t>());
			break;
		case DefaultClass::floatClassId:
			std::stable_sort(array->floatData, array->floatData + array->size, std::greater<double>());
			break;
		case DefaultClass::stringClassId:
			std::stable_sort(array->objData, array->objData + array->size,
			                 [](AObject *a, AObject *b) {
				                 if (!a || !b) return b != nullptr;
				                 auto sa = std::string_view(a->str->data, a->str->size);
				                 auto sb = std::string_view(b->str->data, b->str->size);
				                 return sa > sb;
			                 });
			break;
		default: {
			if (array->objData && array->size > 1 && array->objData[0]) {
				if (array->objData[0]->type == DefaultClass::intClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return b != nullptr;
						                 return a->i > b->i;
					                 });
				} else if (array->objData[0]->type == DefaultClass::floatClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return b != nullptr;
						                 return a->f > b->f;
					                 });
				} else if (array->objData[0]->type == DefaultClass::stringClassId) {
					std::stable_sort(array->objData, array->objData + array->size,
					                 [](AObject *a, AObject *b) {
						                 if (!a || !b) return b != nullptr;
						                 auto sa = std::string_view(a->str->data, a->str->size);
						                 auto sb = std::string_view(b->str->data, b->str->size);
						                 return sa > sb;
					                 });
				}
			}
			break;
		}
	}
	return newArr;
}

AObject *index_of_first(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		bool matched = (res == notifier.getTrueObject());
		notifier.release(res);
		if (matched) return notifier.createInt(static_cast<int64_t>(i));
	}
	return notifier.createInt(-1);
}

AObject *index_of_last(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	if (array->size == 0) return notifier.createInt(-1);
	for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
		auto item = getItem(notifier, array, static_cast<size_t>(i));
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		bool matched = (res == notifier.getTrueObject());
		notifier.release(res);
		if (matched) return notifier.createInt(i);
	}
	return notifier.createInt(-1);
}

AObject *single(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size != 1) {
		notifier.throwException("Array does not have exactly one element.");
		return nullptr;
	}
	return getItem(notifier, array, 0);
}

AObject *single_or_null(NativeFuncInData) {
	auto array = args[0]->array;
	if (array->size != 1) {
		return notifier.getNullObject();
	}
	return getItem(notifier, array, 0);
}

inline int compareAnyObjects(ANotifier &notifier, AObject *a, AObject *b) {
	if (a == b) return 0;
	if (!a || a == DefaultClass::nullObject) return -1;
	if (!b || b == DefaultClass::nullObject) return 1;

	if (a->type == DefaultClass::intClassId && b->type == DefaultClass::intClassId) {
		return (a->i < b->i) ? -1 : ((a->i > b->i) ? 1 : 0);
	}
	if ((a->type == DefaultClass::intClassId || a->type == DefaultClass::floatClassId) &&
	    (b->type == DefaultClass::intClassId || b->type == DefaultClass::floatClassId)) {
		double fa = (a->type == DefaultClass::intClassId) ? static_cast<double>(a->i) : a->f;
		double fb = (b->type == DefaultClass::intClassId) ? static_cast<double>(b->i) : b->f;
		return (fa < fb) ? -1 : ((fa > fb) ? 1 : 0);
	}
	if (a->type == DefaultClass::stringClassId && b->type == DefaultClass::stringClassId) {
		std::string_view sa(a->str->data, a->str->size);
		std::string_view sb(b->str->data, b->str->size);
		return (sa < sb) ? -1 : ((sa > sb) ? 1 : 0);
	}
	if (a->type == DefaultClass::boolClassId && b->type == DefaultClass::boolClassId) {
		return (a->b < b->b) ? -1 : ((a->b > b->b) ? 1 : 0);
	}
	std::string sa = DefaultFunction::to_string(notifier, a);
	std::string sb = DefaultFunction::to_string(notifier, b);
	return (sa < sb) ? -1 : ((sa > sb) ? 1 : 0);
}

static std::unordered_map<AArray *, std::vector<double>> lastSortScores;

AObject *sorted_by(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;
	newArr->array->reallocate(array->size);

	std::vector<std::pair<AObject *, size_t>> scored(array->size);
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (!res) res = DefaultClass::nullObject;
		res->retain();
		scored[i] = {res, i};
	}
	std::stable_sort(scored.begin(), scored.end(), [&](const auto &a, const auto &b) {
		return compareAnyObjects(notifier, a.first, b.first) < 0;
	});
	if (lastSortScores.size() > 256) lastSortScores.clear();
	std::vector<double> finalScores(array->size);
	for (size_t i = 0; i < scored.size(); ++i) {
		if (scored[i].first->type == DefaultClass::intClassId) finalScores[i] = static_cast<double>(scored[i].first->i);
		else if (scored[i].first->type == DefaultClass::floatClassId) finalScores[i] = scored[i].first->f;
		else finalScores[i] = static_cast<double>(i);
		notifier.release(scored[i].first);
		auto item = getItem(notifier, array, scored[i].second);
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	lastSortScores[newArr->array] = std::move(finalScores);
	return newArr;
}

AObject *sorted_by_descending(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;
	newArr->array->reallocate(array->size);

	std::vector<std::pair<AObject *, size_t>> scored(array->size);
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (!res) res = DefaultClass::nullObject;
		res->retain();
		scored[i] = {res, i};
	}
	std::stable_sort(scored.begin(), scored.end(), [&](const auto &a, const auto &b) {
		return compareAnyObjects(notifier, a.first, b.first) > 0;
	});
	if (lastSortScores.size() > 256) lastSortScores.clear();
	std::vector<double> finalScores(array->size);
	for (size_t i = 0; i < scored.size(); ++i) {
		if (scored[i].first->type == DefaultClass::intClassId) finalScores[i] = static_cast<double>(scored[i].first->i);
		else if (scored[i].first->type == DefaultClass::floatClassId) finalScores[i] = scored[i].first->f;
		else finalScores[i] = static_cast<double>(i);
		notifier.release(scored[i].first);
		auto item = getItem(notifier, array, scored[i].second);
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	lastSortScores[newArr->array] = std::move(finalScores);
	return newArr;
}

AObject *then_by(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;
	newArr->array->reallocate(array->size);

	auto it = lastSortScores.find(array);
	std::vector<double> prevScores;
	if (it != lastSortScores.end()) {
		prevScores = it->second;
	} else {
		prevScores.assign(array->size, 0.0);
	}

	struct SortItem {
		double prevScore;
		double newScore;
		size_t originalIndex;
	};
	std::vector<SortItem> items(array->size);
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		double score = 0.0;
		if (res) {
			if (res->type == DefaultClass::intClassId) score = static_cast<double>(res->i);
			else if (res->type == DefaultClass::floatClassId) score = res->f;
			notifier.release(res);
		}
		items[i] = {prevScores[i], score, i};
	}

	std::stable_sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
		if (a.prevScore != b.prevScore) {
			return false;
		}
		return a.newScore < b.newScore;
	});

	if (lastSortScores.size() > 256) lastSortScores.clear();
	std::vector<double> newScores(array->size);
	for (size_t i = 0; i < items.size(); ++i) {
		newScores[i] = items[i].newScore;
		auto item = getItem(notifier, array, items[i].originalIndex);
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	lastSortScores[newArr->array] = std::move(newScores);
	return newArr;
}

AObject *then_by_descending(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;
	newArr->array->reallocate(array->size);

	auto it = lastSortScores.find(array);
	std::vector<double> prevScores;
	if (it != lastSortScores.end()) {
		prevScores = it->second;
	} else {
		prevScores.assign(array->size, 0.0);
	}

	struct SortItem {
		double prevScore;
		double newScore;
		size_t originalIndex;
	};
	std::vector<SortItem> items(array->size);
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		double score = 0.0;
		if (res) {
			if (res->type == DefaultClass::intClassId) score = static_cast<double>(res->i);
			else if (res->type == DefaultClass::floatClassId) score = res->f;
			notifier.release(res);
		}
		items[i] = {prevScores[i], score, i};
	}

	std::stable_sort(items.begin(), items.end(), [](const auto &a, const auto &b) {
		if (a.prevScore != b.prevScore) {
			return false;
		}
		return a.newScore > b.newScore;
	});

	if (lastSortScores.size() > 256) lastSortScores.clear();
	std::vector<double> newScores(array->size);
	for (size_t i = 0; i < items.size(); ++i) {
		newScores[i] = items[i].newScore;
		auto item = getItem(notifier, array, items[i].originalIndex);
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	lastSortScores[newArr->array] = std::move(newScores);
	return newArr;
}

AObject *sorted_with(NativeFuncInData) {
	auto arr = args[0];
	auto compObj = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;
	newArr->array->reallocate(array->size);

	AObject *selectorsArr = nullptr;
	AObject *directionsArr = nullptr;
	if (compObj && (compObj->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && compObj->member && compObj->member->data) {
		selectorsArr = compObj->member->data[0];
		if (compObj->member->size > 1) {
			directionsArr = compObj->member->data[1];
		}
	}

	std::vector<size_t> indices(array->size);
	for (size_t i = 0; i < array->size; ++i) indices[i] = i;

	bool hasErr = false;
	if (selectorsArr && selectorsArr->array) {
		auto selArray = selectorsArr->array;
		auto dirArray = (directionsArr && directionsArr->array) ? directionsArr->array : nullptr;
		size_t numStages = selArray->size;

		std::stable_sort(indices.begin(), indices.end(), [&](size_t ia, size_t ib) {
			if (hasErr) return false;
			auto itemA = getItem(notifier, array, ia);
			auto itemB = getItem(notifier, array, ib);
			for (size_t s = 0; s < numStages; ++s) {
				auto selector = selArray->objData[s];
				int64_t dir = 1;
				if (dirArray && s < dirArray->size) {
					dir = dirArray->intData[s];
				}

				itemA->retain();
				auto resA = notifier.callFunctionObject(selector, itemA);
				if (notifier.hasException()) {
					notifier.release(itemA);
					hasErr = true;
					return false;
				}
				double scoreA = 0.0;
				if (resA) {
					if (resA->type == DefaultClass::intClassId) scoreA = static_cast<double>(resA->i);
					else if (resA->type == DefaultClass::floatClassId) scoreA = resA->f;
					notifier.release(resA);
				}

				itemB->retain();
				auto resB = notifier.callFunctionObject(selector, itemB);
				if (notifier.hasException()) {
					notifier.release(itemA);
					notifier.release(itemB);
					hasErr = true;
					return false;
				}
				double scoreB = 0.0;
				if (resB) {
					if (resB->type == DefaultClass::intClassId) scoreB = static_cast<double>(resB->i);
					else if (resB->type == DefaultClass::floatClassId) scoreB = resB->f;
					notifier.release(resB);
				}

				if (scoreA != scoreB) {
					notifier.release(itemA);
					notifier.release(itemB);
					if (dir < 0) return scoreA > scoreB;
					return scoreA < scoreB;
				}
			}
			notifier.release(itemA);
			notifier.release(itemB);
			return false;
		});
	} else {
		AObject *funcObject = compObj;
		std::stable_sort(indices.begin(), indices.end(), [&](size_t ia, size_t ib) {
			if (hasErr) return false;
			auto itemA = getItem(notifier, array, ia);
			auto itemB = getItem(notifier, array, ib);
			itemA->retain();
			itemB->retain();
			auto res = notifier.callFunctionObject(funcObject, itemA, itemB);
			notifier.release(itemA);
			notifier.release(itemB);
			if (notifier.hasException()) {
				hasErr = true;
				return false;
			}
			bool less = false;
			if (res) {
				if (res->type == DefaultClass::intClassId) less = res->i < 0;
				else if (res->type == DefaultClass::floatClassId) less = res->f < 0;
				notifier.release(res);
			}
			return less;
		});
	}

	if (hasErr) return nullptr;

	for (size_t i = 0; i < indices.size(); ++i) {
		auto item = getItem(notifier, array, indices[i]);
		notifier.arrayAdd(newArr, item);
		notifier.release(item);
	}
	return newArr;
}

AObject *sort_with(NativeFuncInData) {
	auto arr = args[0];
	auto compObj = args[1];
	auto array = arr->array;
	if (array->size <= 1) return arr;

	AObject *selectorsArr = nullptr;
	AObject *directionsArr = nullptr;
	if (compObj && (compObj->flags & AObject::Flags::OBJ_HAS_MEMBER_DATA) && compObj->member && compObj->member->data) {
		selectorsArr = compObj->member->data[0];
		if (compObj->member->size > 1) {
			directionsArr = compObj->member->data[1];
		}
	}

	std::vector<size_t> indices(array->size);
	for (size_t i = 0; i < array->size; ++i) indices[i] = i;

	bool hasErr = false;
	if (selectorsArr && selectorsArr->array) {
		auto selArray = selectorsArr->array;
		auto dirArray = (directionsArr && directionsArr->array) ? directionsArr->array : nullptr;
		size_t numStages = selArray->size;

		std::stable_sort(indices.begin(), indices.end(), [&](size_t ia, size_t ib) {
			if (hasErr) return false;
			auto itemA = getItem(notifier, array, ia);
			auto itemB = getItem(notifier, array, ib);
			for (size_t s = 0; s < numStages; ++s) {
				auto selector = selArray->objData[s];
				int64_t dir = 1;
				if (dirArray && s < dirArray->size) {
					dir = dirArray->intData[s];
				}

				itemA->retain();
				auto resA = notifier.callFunctionObject(selector, itemA);
				if (notifier.hasException()) {
					notifier.release(itemA);
					hasErr = true;
					return false;
				}
				double scoreA = 0.0;
				if (resA) {
					if (resA->type == DefaultClass::intClassId) scoreA = static_cast<double>(resA->i);
					else if (resA->type == DefaultClass::floatClassId) scoreA = resA->f;
					notifier.release(resA);
				}

				itemB->retain();
				auto resB = notifier.callFunctionObject(selector, itemB);
				if (notifier.hasException()) {
					notifier.release(itemA);
					notifier.release(itemB);
					hasErr = true;
					return false;
				}
				double scoreB = 0.0;
				if (resB) {
					if (resB->type == DefaultClass::intClassId) scoreB = static_cast<double>(resB->i);
					else if (resB->type == DefaultClass::floatClassId) scoreB = resB->f;
					notifier.release(resB);
				}

				if (scoreA != scoreB) {
					notifier.release(itemA);
					notifier.release(itemB);
					if (dir < 0) return scoreA > scoreB;
					return scoreA < scoreB;
				}
			}
			notifier.release(itemA);
			notifier.release(itemB);
			return false;
		});
	} else {
		AObject *funcObject = compObj;
		std::stable_sort(indices.begin(), indices.end(), [&](size_t ia, size_t ib) {
			if (hasErr) return false;
			auto itemA = getItem(notifier, array, ia);
			auto itemB = getItem(notifier, array, ib);
			itemA->retain();
			itemB->retain();
			auto res = notifier.callFunctionObject(funcObject, itemA, itemB);
			notifier.release(itemA);
			notifier.release(itemB);
			if (notifier.hasException()) {
				hasErr = true;
				return false;
			}
			bool less = false;
			if (res) {
				if (res->type == DefaultClass::intClassId) less = res->i < 0;
				else if (res->type == DefaultClass::floatClassId) less = res->f < 0;
				notifier.release(res);
			}
			return less;
		});
	}

	if (hasErr) return nullptr;

	switch (array->key) {
		case DefaultClass::intClassId: {
			std::vector<int64_t> tmp(array->size);
			for (size_t i = 0; i < array->size; ++i) tmp[i] = array->intData[indices[i]];
			for (size_t i = 0; i < array->size; ++i) array->intData[i] = tmp[i];
			break;
		}
		case DefaultClass::floatClassId: {
			std::vector<double> tmp(array->size);
			for (size_t i = 0; i < array->size; ++i) tmp[i] = array->floatData[indices[i]];
			for (size_t i = 0; i < array->size; ++i) array->floatData[i] = tmp[i];
			break;
		}
		default: {
			std::vector<AObject *> tmp(array->size);
			for (size_t i = 0; i < array->size; ++i) tmp[i] = array->objData[indices[i]];
			for (size_t i = 0; i < array->size; ++i) array->objData[i] = tmp[i];
			break;
		}
	}
	return arr;
}

AObject *min_by_or_null(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	if (array->size == 0) return notifier.getNullObject();

	size_t bestIdx = 0;
	auto firstItem = getItem(notifier, array, 0);
	firstItem->retain();
	auto bestRes = notifier.callFunctionObject(funcObject, firstItem);
	notifier.release(firstItem);
	if (notifier.hasException()) return nullptr;
	if (!bestRes) bestRes = DefaultClass::nullObject;
	bestRes->retain();

	for (size_t i = 1; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) {
			notifier.release(bestRes);
			return nullptr;
		}
		if (!res) res = DefaultClass::nullObject;
		if (compareAnyObjects(notifier, res, bestRes) < 0) {
			notifier.release(bestRes);
			bestRes = res;
			bestRes->retain();
			bestIdx = i;
		} else {
			notifier.release(res);
		}
	}
	notifier.release(bestRes);
	return getItem(notifier, array, bestIdx);
}

AObject *max_by_or_null(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	if (array->size == 0) return notifier.getNullObject();

	size_t bestIdx = 0;
	auto firstItem = getItem(notifier, array, 0);
	firstItem->retain();
	auto bestRes = notifier.callFunctionObject(funcObject, firstItem);
	notifier.release(firstItem);
	if (notifier.hasException()) return nullptr;
	if (!bestRes) bestRes = DefaultClass::nullObject;
	bestRes->retain();

	for (size_t i = 1; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) {
			notifier.release(bestRes);
			return nullptr;
		}
		if (!res) res = DefaultClass::nullObject;
		if (compareAnyObjects(notifier, res, bestRes) > 0) {
			notifier.release(bestRes);
			bestRes = res;
			bestRes->retain();
			bestIdx = i;
		} else {
			notifier.release(res);
		}
	}
	notifier.release(bestRes);
	return getItem(notifier, array, bestIdx);
}

AObject *distinct_by(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	if (array->size == 0) return newArr;

	std::unordered_set<std::string> seenStrings;
	std::unordered_set<int64_t> seenInts;
	std::unordered_set<double> seenFloats;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto k = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		bool alreadySeen = false;
		if (k) {
			if (k->type == DefaultClass::intClassId) {
				alreadySeen = !seenInts.insert(k->i).second;
			} else if (k->type == DefaultClass::floatClassId) {
				alreadySeen = !seenFloats.insert(k->f).second;
			} else if (k->type == DefaultClass::stringClassId) {
				alreadySeen = !seenStrings.insert(std::string(k->str->data, k->str->size)).second;
			} else {
				alreadySeen = !seenStrings.insert(DefaultFunction::to_string(notifier, k)).second;
			}
			notifier.release(k);
		}
		if (!alreadySeen) {
			notifier.arrayAdd(newArr, item);
		}
		notifier.release(item);
	}
	return newArr;
}

AObject *shuffled(NativeFuncInData) {
	auto cloneArr = clone(notifier, args, argSize);
	if (!cloneArr) return nullptr;
	auto array = cloneArr->array;
	if (array->size <= 1) return cloneArr;
	for (size_t i = array->size - 1; i > 0; --i) {
		size_t j = static_cast<size_t>(std::rand()) % (i + 1);
		if (i == j) continue;
		switch (array->key) {
			case DefaultClass::intClassId:
				std::swap(array->intData[i], array->intData[j]);
				break;
			case DefaultClass::floatClassId:
				std::swap(array->floatData[i], array->floatData[j]);
				break;
			default:
				std::swap(array->objData[i], array->objData[j]);
				break;
		}
	}
	return cloneArr;
}

AObject *flatten(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		if (item && (item->flags & AObject::Flags::OBJ_IS_ARRAY)) {
			auto innerArray = item->array;
			for (size_t j = 0; j < innerArray->size; ++j) {
				auto innerItem = getItem(notifier, innerArray, j);
				notifier.arrayAdd(newArr, innerItem);
				notifier.release(innerItem);
			}
		} else {
			notifier.arrayAdd(newArr, item);
		}
		notifier.release(item);
	}
	return newArr;
}

AObject *group_by(NativeFuncInData) {
	auto arr = args[0];
	auto keySelector = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId mapKey = getArrayGenericKey(notifier, returnId);
	auto mapObj = map::constructor(notifier, returnId, mapKey);
	mapObj->flags |= AObject::Flags::OBJ_IS_MAP;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto key = notifier.callFunctionObject(keySelector, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		AObject *getArgs[2] = {mapObj, key};
		auto listObj = map::get(notifier, getArgs, 2);
		if (!listObj || listObj == DefaultClass::nullObject) {
			listObj = notifier.createArray(arr->type, array->key);
			AObject *setArgs[3] = {mapObj, key, listObj};
			map::set(notifier, setArgs, 3);
		}
		notifier.arrayAdd(listObj, item);
		notifier.release(key);
		notifier.release(item);
	}
	return mapObj;
}

AObject *associate_by(NativeFuncInData) {
	auto arr = args[0];
	auto keySelector = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId mapKey = getArrayGenericKey(notifier, returnId);
	auto mapObj = map::constructor(notifier, returnId, mapKey);
	mapObj->flags |= AObject::Flags::OBJ_IS_MAP;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto key = notifier.callFunctionObject(keySelector, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		AObject *setArgs[3] = {mapObj, key, item};
		map::set(notifier, setArgs, 3);
		notifier.release(key);
		notifier.release(item);
	}
	return mapObj;
}

AObject *remove_element(NativeFuncInData) {
	auto obj = args[0];
	auto target = args[1];
	auto array = obj->array;
	if (array->size == 0) return notifier.createBool(false);

	int64_t foundIdx = -1;
	switch (array->key) {
		case DefaultClass::intClassId: {
			if (target->type == DefaultClass::intClassId) {
				int64_t val = target->i;
				for (size_t i = 0; i < array->size; ++i) {
					if (array->intData[i] == val) {
						foundIdx = static_cast<int64_t>(i);
						break;
					}
				}
			}
			break;
		}
		case DefaultClass::floatClassId: {
			double val = (target->type == DefaultClass::intClassId)
			                 ? static_cast<double>(target->i)
			                 : target->f;
			for (size_t i = 0; i < array->size; ++i) {
				if (array->floatData[i] == val) {
					foundIdx = static_cast<int64_t>(i);
					break;
				}
			}
			break;
		}
		default: {
			for (size_t i = 0; i < array->size; ++i) {
				if (DefaultFunction::op_eqeq(array->objData[i], target)) {
					foundIdx = static_cast<int64_t>(i);
					break;
				}
			}
			break;
		}
	}

	if (foundIdx < 0) return notifier.createBool(false);

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = foundIdx; i < array->size - 1; ++i) {
				array->intData[i] = array->intData[i + 1];
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = foundIdx; i < array->size - 1; ++i) {
				array->floatData[i] = array->floatData[i + 1];
			}
			break;
		}
		default: {
			notifier.release(array->objData[foundIdx]);
			for (size_t i = foundIdx; i < array->size - 1; ++i) {
				array->objData[i] = array->objData[i + 1];
			}
			array->objData[array->size - 1] = nullptr;
			break;
		}
	}
	array->size--;
	return notifier.createBool(true);
}

AObject *remove_at(NativeFuncInData) {
	auto obj = args[0];
	auto array = obj->array;
	if (args[1]->type != DefaultClass::intClassId) {
		notifier.throwException("Array.removeAt: index must be Int");
		return nullptr;
	}
	int64_t index = args[1]->i;
	if (index < 0 || static_cast<size_t>(index) >= array->size) {
		notifier.throwException("Array.removeAt: index out of range: " + std::to_string(index));
		return nullptr;
	}
	AObject *removedItem = getItem(notifier, array, static_cast<size_t>(index));

	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = index; i < array->size - 1; ++i) {
				array->intData[i] = array->intData[i + 1];
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = index; i < array->size - 1; ++i) {
				array->floatData[i] = array->floatData[i + 1];
			}
			break;
		}
		default: {
			for (size_t i = index; i < array->size - 1; ++i) {
				array->objData[i] = array->objData[i + 1];
			}
			array->objData[array->size - 1] = nullptr;
			break;
		}
	}
	array->size--;
	return removedItem;
}

AObject *add_all(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (argSize == 2) {
		auto otherArr = args[1];
		if (!otherArr || !(otherArr->flags & AObject::Flags::OBJ_IS_ARRAY)) {
			notifier.throwException("Array.addAll: expected Array argument");
			return nullptr;
		}
		auto otherArray = otherArr->array;
		if (otherArray->size == 0) return notifier.createBool(false);
		for (size_t i = 0; i < otherArray->size; ++i) {
			auto item = getItem(notifier, otherArray, i);
			notifier.arrayAdd(arr, item);
			notifier.release(item);
		}
		return notifier.createBool(true);
	} else if (argSize >= 3) {
		int64_t index = args[1]->i;
		if (index < 0 || static_cast<size_t>(index) > array->size) {
			notifier.throwException("Array.addAll: index out of range");
			return nullptr;
		}
		auto otherArr = args[2];
		if (!otherArr || !(otherArr->flags & AObject::Flags::OBJ_IS_ARRAY)) {
			notifier.throwException("Array.addAll: expected Array argument");
			return nullptr;
		}
		auto otherArray = otherArr->array;
		if (otherArray->size == 0) return notifier.createBool(false);
		for (size_t i = 0; i < otherArray->size; ++i) {
			auto item = getItem(notifier, otherArray, i);
			AObject *insArgs[3] = {arr, notifier.createInt(index + static_cast<int64_t>(i)), item};
			insert(notifier, insArgs, 3);
			notifier.release(item);
		}
		return notifier.createBool(true);
	}
	return notifier.createBool(false);
}

AObject *remove_all(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (array->size == 0) return notifier.createBool(false);

	if (args[1]->flags & AObject::Flags::OBJ_IS_ARRAY) {
		bool modified = false;
		for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
			auto item = getItem(notifier, array, static_cast<size_t>(i));
			AObject *cArgs[2] = {args[1], item};
			auto containsRes = contains(notifier, cArgs, 2);
			notifier.release(item);
			if (containsRes == notifier.getTrueObject()) {
				AObject *rmArgs[2] = {arr, notifier.createInt(i)};
				remove(notifier, rmArgs, 2);
				modified = true;
			}
		}
		return notifier.createBool(modified);
	} else {
		auto funcObject = args[1];
		bool modified = false;
		for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
			auto item = getItem(notifier, array, static_cast<size_t>(i));
			item->retain();
			auto res = notifier.callFunctionObject(funcObject, item);
			notifier.release(item);
			if (notifier.hasException()) return nullptr;
			if (res == notifier.getTrueObject()) {
				AObject *rmArgs[2] = {arr, notifier.createInt(i)};
				remove(notifier, rmArgs, 2);
				modified = true;
			}
			notifier.release(res);
		}
		return notifier.createBool(modified);
	}
}

AObject *retain_all(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	if (array->size == 0) return notifier.createBool(false);
	auto otherArr = args[1];
	bool modified = false;
	for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
		auto item = getItem(notifier, array, static_cast<size_t>(i));
		AObject *cArgs[2] = {otherArr, item};
		auto containsRes = contains(notifier, cArgs, 2);
		notifier.release(item);
		if (containsRes != notifier.getTrueObject()) {
			AObject *rmArgs[2] = {arr, notifier.createInt(i)};
			remove(notifier, rmArgs, 2);
			modified = true;
		}
	}
	return notifier.createBool(modified);
}

AObject *contains_all(NativeFuncInData) {
	auto arr = args[0];
	auto otherArr = args[1];
	if (!otherArr || !(otherArr->flags & AObject::Flags::OBJ_IS_ARRAY)) return notifier.createBool(false);
	auto otherArray = otherArr->array;
	for (size_t i = 0; i < otherArray->size; ++i) {
		auto item = getItem(notifier, otherArray, i);
		AObject *cArgs[2] = {arr, item};
		auto containsRes = contains(notifier, cArgs, 2);
		notifier.release(item);
		if (containsRes != notifier.getTrueObject()) {
			return notifier.createBool(false);
		}
	}
	return notifier.createBool(true);
}

AObject *flat_map(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (res && (res->flags & AObject::Flags::OBJ_IS_ARRAY)) {
			auto innerArray = res->array;
			for (size_t j = 0; j < innerArray->size; ++j) {
				auto innerItem = getItem(notifier, innerArray, j);
				notifier.arrayAdd(newArr, innerItem);
				notifier.release(innerItem);
			}
		}
		notifier.release(res);
	}
	return newArr;
}

AObject *zip(NativeFuncInData) {
	auto arr = args[0];
	auto otherArr = args[1];
	auto array1 = arr->array;
	auto array2 = otherArr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId pairClassId = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, pairClassId);

	size_t minLen = std::min(array1->size, array2->size);
	for (size_t i = 0; i < minLen; ++i) {
		auto item1 = getItem(notifier, array1, i);
		auto item2 = getItem(notifier, array2, i);
		auto pairObj = notifier.createMemberObject(pairClassId, 2);
		item1->retain();
		pairObj->member->data[0] = item1;
		item2->retain();
		pairObj->member->data[1] = item2;
		notifier.arrayAdd(newArr, pairObj);
		notifier.release(item1);
		notifier.release(item2);
		notifier.release(pairObj);
	}
	return newArr;
}

static inline ClassId findPairClassId(ANotifier &notifier) {
	for (ClassId c = 0; c < notifier.vm->data.classes.size(); ++c) {
		auto clazz = notifier.vm->data.classes[c];
		if (clazz) {
			auto name = clazz->getName(notifier.vm->data);
			if (name == "Pair" || name.rfind("Pair<", 0) == 0) {
				return c;
			}
		}
	}
	return DefaultClass::anyClassId;
}

AObject *unzip(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	auto returnClazz = notifier.vm->data.classes[returnId];
	ClassId firstArrId = (returnClazz && returnClazz->genericType.size > 0)
	    ? notifier.vm->data.allGenericType[returnClazz->genericType.offset] : arr->type;
	ClassId secondArrId = (returnClazz && returnClazz->genericType.size > 1)
	    ? notifier.vm->data.allGenericType[returnClazz->genericType.offset + 1] : arr->type;

	auto firstArr = notifier.createArray(firstArrId, DefaultClass::anyClassId);
	auto secondArr = notifier.createArray(secondArrId, DefaultClass::anyClassId);

	for (size_t i = 0; i < array->size; ++i) {
		auto pairObj = getItem(notifier, array, i);
		if (pairObj && pairObj->member && pairObj->member->data) {
			notifier.arrayAdd(firstArr, pairObj->member->data[0]);
			notifier.arrayAdd(secondArr, pairObj->member->data[1]);
		}
		notifier.release(pairObj);
	}

	ClassId pairId = returnId;
	if (pairId == DefaultClass::anyClassId) {
		pairId = findPairClassId(notifier);
	}
	auto pairResult = notifier.createMemberObject(pairId, 2);
	firstArr->retain();
	pairResult->member->data[0] = firstArr;
	secondArr->retain();
	pairResult->member->data[1] = secondArr;
	return pairResult;
}

AObject *windowed(NativeFuncInData) {
	auto arr = args[0];
	auto array = arr->array;
	int64_t size = args[1]->i;
	int64_t step = (argSize >= 3 && args[2]->type == DefaultClass::intClassId) ? args[2]->i : 1;
	bool partialWindows = (argSize >= 4 && args[3]->type == DefaultClass::boolClassId) ? args[3]->b : false;

	if (size <= 0) {
		notifier.throwException("Array.windowed: size must be greater than 0");
		return nullptr;
	}
	if (step <= 0) {
		notifier.throwException("Array.windowed: step must be greater than 0");
		return nullptr;
	}

	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemArrId = (returnId < notifier.vm->data.classes.size() && notifier.vm->data.classes[returnId] && notifier.vm->data.classes[returnId]->genericType.size > 0)
	    ? notifier.vm->data.allGenericType[notifier.vm->data.classes[returnId]->genericType.offset] : arr->type;
	auto resultArr = notifier.createArray(returnId, elemArrId);

	for (size_t i = 0; i < array->size; i += step) {
		size_t winLen = std::min<size_t>(size, array->size - i);
		if (winLen < static_cast<size_t>(size) && !partialWindows) {
			break;
		}
		auto winArr = notifier.createArray(elemArrId, array->key);
		for (size_t j = 0; j < winLen; ++j) {
			auto item = getItem(notifier, array, i + j);
			notifier.arrayAdd(winArr, item);
			notifier.release(item);
		}
		notifier.arrayAdd(resultArr, winArr);
		notifier.release(winArr);
	}
	return resultArr;
}

AObject *partition(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	auto returnClazz = notifier.vm->data.classes[returnId];
	ClassId firstArrId = (returnClazz && returnClazz->genericType.size > 0)
	    ? notifier.vm->data.allGenericType[returnClazz->genericType.offset] : arr->type;
	ClassId secondArrId = (returnClazz && returnClazz->genericType.size > 1)
	    ? notifier.vm->data.allGenericType[returnClazz->genericType.offset + 1] : arr->type;

	auto firstArr = notifier.createArray(firstArrId, array->key);
	auto secondArr = notifier.createArray(secondArrId, array->key);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		if (res == notifier.getTrueObject()) {
			notifier.arrayAdd(firstArr, item);
		} else {
			notifier.arrayAdd(secondArr, item);
		}
		notifier.release(res);
		notifier.release(item);
	}

	ClassId pairId = returnId;
	if (pairId == DefaultClass::anyClassId) {
		pairId = findPairClassId(notifier);
	}
	auto pairObj = notifier.createMemberObject(pairId, 2);
	firstArr->retain();
	pairObj->member->data[0] = firstArr;
	secondArr->retain();
	pairObj->member->data[1] = secondArr;
	return pairObj;
}

AObject *associate(NativeFuncInData) {
	auto arr = args[0];
	auto transform = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId mapKey = getArrayGenericKey(notifier, returnId);
	auto mapObj = map::constructor(notifier, returnId, mapKey);
	mapObj->flags |= AObject::Flags::OBJ_IS_MAP;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto pairObj = notifier.callFunctionObject(transform, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (pairObj && pairObj->member && pairObj->member->data) {
			AObject *setArgs[3] = {mapObj, pairObj->member->data[0], pairObj->member->data[1]};
			map::set(notifier, setArgs, 3);
		}
		notifier.release(pairObj);
	}
	return mapObj;
}

AObject *associate_with(NativeFuncInData) {
	auto arr = args[0];
	auto valueSelector = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId mapKey = array->key;
	auto mapObj = map::constructor(notifier, returnId, mapKey);
	mapObj->flags |= AObject::Flags::OBJ_IS_MAP;

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto val = notifier.callFunctionObject(valueSelector, item);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		AObject *setArgs[3] = {mapObj, item, val};
		map::set(notifier, setArgs, 3);
		notifier.release(val);
		notifier.release(item);
	}
	return mapObj;
}

AObject *map_not_null(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (res && res != DefaultClass::nullObject) {
			notifier.arrayAdd(newArr, res);
		}
		notifier.release(res);
	}
	return newArr;
}

AObject *map_indexed_not_null(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	ClassId returnId = notifier.callFrame->func->returnId;
	ClassId elemKey = getArrayGenericKey(notifier, returnId);
	auto newArr = notifier.createArray(returnId, elemKey);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto idxObj = notifier.createInt(i);
		idxObj->retain();
		auto res = notifier.callFunctionObject(funcObject, idxObj, item);
		notifier.release(item);
		notifier.release(idxObj);
		if (notifier.hasException()) return nullptr;
		if (res && res != DefaultClass::nullObject) {
			notifier.arrayAdd(newArr, res);
		}
		notifier.release(res);
	}
	return newArr;
}

AObject *filter_indexed(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	auto newArr = notifier.createArray(arr->type, array->key);

	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto idxObj = notifier.createInt(i);
		idxObj->retain();
		auto res = notifier.callFunctionObject(funcObject, idxObj, item);
		notifier.release(idxObj);
		if (notifier.hasException()) {
			notifier.release(item);
			return nullptr;
		}
		if (res == notifier.getTrueObject()) {
			notifier.arrayAdd(newArr, item);
		}
		notifier.release(res);
		notifier.release(item);
	}
	return newArr;
}

AObject *first_fn(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		bool matched = (res == notifier.getTrueObject());
		notifier.release(res);
		if (matched) return getItem(notifier, array, i);
	}
	notifier.throwException("Collection contains no element matching the predicate.");
	return nullptr;
}

AObject *last_fn(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
		auto item = getItem(notifier, array, static_cast<size_t>(i));
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		bool matched = (res == notifier.getTrueObject());
		notifier.release(res);
		if (matched) return getItem(notifier, array, static_cast<size_t>(i));
	}
	notifier.throwException("Collection contains no element matching the predicate.");
	return nullptr;
}

AObject *single_fn(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	size_t matchIdx = 0;
	size_t matchCount = 0;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (res == notifier.getTrueObject()) {
			matchIdx = i;
			matchCount++;
			if (matchCount > 1) {
				notifier.release(res);
				notifier.throwException("Collection contains more than one matching element.");
				return nullptr;
			}
		}
		notifier.release(res);
	}
	if (matchCount == 0) {
		notifier.throwException("Collection contains no element matching the predicate.");
		return nullptr;
	}
	return getItem(notifier, array, matchIdx);
}

AObject *single_or_null_fn(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	size_t matchIdx = 0;
	size_t matchCount = 0;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto res = notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
		if (res == notifier.getTrueObject()) {
			matchIdx = i;
			matchCount++;
			if (matchCount > 1) {
				notifier.release(res);
				return notifier.getNullObject();
			}
		}
		notifier.release(res);
	}
	if (matchCount == 1) return getItem(notifier, array, matchIdx);
	return notifier.getNullObject();
}

AObject *last_index_of(NativeFuncInData) {
	auto arr = args[0];
	auto target = args[1];
	auto array = arr->array;
	if (array->size == 0) return notifier.createInt(-1);

	for (int64_t i = static_cast<int64_t>(array->size) - 1; i >= 0; --i) {
		switch (array->key) {
			case DefaultClass::intClassId: {
				if (target->type == DefaultClass::intClassId && array->intData[i] == target->i) {
					return notifier.createInt(i);
				}
				break;
			}
			case DefaultClass::floatClassId: {
				double val = (target->type == DefaultClass::intClassId) ? target->i : target->f;
				if (array->floatData[i] == val) return notifier.createInt(i);
				break;
			}
			default: {
				if (DefaultFunction::op_eqeq(array->objData[i], target)) {
					return notifier.createInt(i);
				}
				break;
			}
		}
	}
	return notifier.createInt(-1);
}

AObject *last_index(NativeFuncInData) {
	auto arr = args[0];
	return notifier.createInt(static_cast<int64_t>(arr->array->size) - 1);
}

AObject *indices(NativeFuncInData) {
	auto arr = args[0];
	int64_t sz = static_cast<int64_t>(arr->array->size);
	ClassId returnId = (notifier.callFrame && notifier.callFrame->func) ? notifier.callFrame->func->returnId : DefaultClass::arrayClassId;
	AObject *newArr = notifier.createArray(returnId, DefaultClass::intClassId, static_cast<uint32_t>(sz));
	newArr->array->size = sz;
	for (int64_t i = 0; i < sz; ++i) {
		newArr->array->intData[i] = i;
	}
	return newArr;
}

AObject *on_each(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		notifier.callFunctionObject(funcObject, item);
		notifier.release(item);
		if (notifier.hasException()) return nullptr;
	}
	return arr;
}

AObject *on_each_indexed(NativeFuncInData) {
	auto arr = args[0];
	auto funcObject = args[1];
	auto array = arr->array;
	for (size_t i = 0; i < array->size; ++i) {
		auto item = getItem(notifier, array, i);
		item->retain();
		auto idxObj = notifier.createInt(i);
		idxObj->retain();
		notifier.callFunctionObject(funcObject, idxObj, item);
		notifier.release(item);
		notifier.release(idxObj);
		if (notifier.hasException()) return nullptr;
	}
	return arr;
}

} // namespace array
} // namespace Libs
} // namespace Autolang

#endif
