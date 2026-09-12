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
		default:
			break;
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

// args[0] = Array, args[1] = separator String
AObject *join_to_string(NativeFuncInData) {
	auto array = args[0]->array;
	const std::string separator =
	    (argSize >= 2 && args[1]->type == DefaultClass::stringClassId)
	        ? std::string(args[1]->str->data)
	        : ", ";
	std::string result;
	size_t sz = array->size;
	switch (array->key) {
		case DefaultClass::intClassId: {
			for (size_t i = 0; i < sz; ++i) {
				if (i > 0) result += separator;
				result += std::to_string(array->intData[i]);
			}
			break;
		}
		case DefaultClass::floatClassId: {
			for (size_t i = 0; i < sz; ++i) {
				if (i > 0) result += separator;
				result += std::to_string(array->floatData[i]);
			}
			break;
		}
		default: {
			// Bool, String, Object, Function all use objData
			for (size_t i = 0; i < sz; ++i) {
				if (i > 0) result += separator;
				result +=
				    DefaultFunction::to_string(notifier, array->objData[i]);
			}
			break;
		}
	}
	return notifier.createString(result);
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
		default:
			break;
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
				auto res = notifier.callFunctionObject(funcObject, item, indexObj);
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
				auto res = notifier.callFunctionObject(funcObject, item, indexObj);
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
				auto res = notifier.callFunctionObject(funcObject, array->objData[i], indexObj);
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

} // namespace array
} // namespace Libs
} // namespace Autolang

#endif
