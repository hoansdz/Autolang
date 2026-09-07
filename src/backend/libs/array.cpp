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

} // namespace array
} // namespace Libs
} // namespace Autolang

#endif
