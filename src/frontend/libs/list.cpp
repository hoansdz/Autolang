#ifndef LIBS_LIST_CPP
#define LIBS_LIST_CPP

#include "list.hpp"
#include "frontend/ACompiler.hpp"
#include <string>

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace list {

AObject *add(NativeFuncInData) {
	auto obj = args[0];
	args[1]->retain();

	if (obj->member->size == 0) {
		obj->member->reallocate(1);
		obj->member->data[0] = args[1];
		obj->member->size = 1;
		obj->member->maxSize = 1;
		return nullptr;
	}

	if (obj->member->size == obj->member->maxSize) {
		size_t newMax =
		    (obj->member->maxSize == 0) ? 1 : obj->member->maxSize * 2;
		obj->member->reallocate(newMax);
		obj->member->maxSize = static_cast<int64_t>(newMax);
	}

	obj->member->data[obj->member->size++] = args[1];
	return nullptr;
}

AObject *remove(NativeFuncInData) {
	auto obj = args[0];

	if (args[1]->type != AutoLang::DefaultClass::intClassId) {
		notifier.throwException("List.remove: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= obj->member->size) {
		notifier.throwException("List.remove: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	notifier.release(obj->member->data[index]);

	for (int i = index; i < obj->member->size - 1; ++i) {
		obj->member->data[i] = obj->member->data[i + 1];
	}

	obj->member->data[obj->member->size - 1] = nullptr;
	obj->member->size--;

	// optional: shrink capacity if too sparse (e.g., 1/4)
	if (obj->member->maxSize > 1 &&
	    obj->member->size <= obj->member->maxSize / 4) {
		size_t newMax = obj->member->maxSize / 2;
		if (newMax < 1)
			newMax = 1;
		obj->member->reallocate(newMax);
		obj->member->maxSize = static_cast<int64_t>(newMax);
	}

	return nullptr;
}

AObject *size(NativeFuncInData) {
	auto obj = args[0];
	return notifier.createInt(static_cast<int64_t>(obj->member->size));
}

AObject *get(NativeFuncInData) {
	auto obj = args[0];

	if (args[1]->type != AutoLang::DefaultClass::intClassId) {
		notifier.throwException("List.get: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= obj->member->size) {
		notifier.throwException("List.get: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	// Trả chính AObject* đã lưu
	return obj->member->data[index];
}

AObject *set(NativeFuncInData) {
	auto obj = args[0];

	if (args[1]->type != AutoLang::DefaultClass::intClassId) {
		notifier.throwException("List.set: index must be Int");
		return nullptr;
	}
	int index = static_cast<int64_t>(args[1]->i);

	if (index < 0 || index >= obj->member->size) {
		notifier.throwException("List.set: index out of range: " +
		                        std::to_string(index));
		return nullptr;
	}

	notifier.release(obj->member->data[index]);
	args[2]->retain();
	obj->member->data[index] = args[2];
	return nullptr;
}

AObject *clear(NativeFuncInData) {
	auto obj = args[0];

	for (int i = 0; i < obj->member->size; ++i) {
		notifier.release(obj->member->data[i]);
		obj->member->data[i] = nullptr;
	}
	obj->member->size = 0;

	if (obj->member->maxSize > 8) {
		obj->member->reallocate(8);
		obj->member->maxSize = 8;
	}

	return nullptr;
}

void init(ACompiler &compiler) {
	compiler.registerFromSource("std/list", R"###(
@no_extends
class List<T>() {
	@native("add")
	func add(value: T)
	@native("remove")
	func remove(index: Int)
	@native("size")
	func size()
	@native("get")
	func get(index: Int): T
	@native("get")
	func [](index: Int): T
	@native("set")
	func set(index: Int, value: T)
	@native("clear")
	func clear()
}
	)###",
	                            true,
	                            ANativeMap({
	                                {"add", &add},
	                                {"remove", &remove},
	                                {"size", &size},
	                                {"get", &get},
	                                {"set", &set},
	                                {"clear", &clear},
	                            }));
}

} // namespace list
} // namespace Libs
} // namespace AutoLang

#endif
