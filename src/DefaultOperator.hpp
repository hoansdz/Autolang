#ifndef DEFAULT_OPERATOR_HPP
#define DEFAULT_OPERATOR_HPP

#include <iostream>
#include <cmath>
#include "Debugger.hpp"
#include "DefaultClass.hpp"

namespace AutoLang {
namespace DefaultFunction {

inline AObject* plus_plus(NativeFuncInData);
inline AObject* minus_minus(NativeFuncInData);
inline AObject* plus(NativeFuncInData);
inline AObject* plus_eq(NativeFuncInData);
inline AObject* minus(NativeFuncInData);
inline AObject* minus_eq(NativeFuncInData);
inline AObject* mul(NativeFuncInData);
inline AObject* mul_eq(NativeFuncInData);
inline AObject* divide(NativeFuncInData);
inline AObject* divide_eq(NativeFuncInData);
inline AObject* mod(NativeFuncInData);
inline AObject* negative(NativeFuncInData);
inline AObject* op_not(NativeFuncInData);
inline AObject* op_and_and(NativeFuncInData);
inline AObject* op_or_or(NativeFuncInData);
inline AObject* op_less_than(NativeFuncInData);
inline AObject* op_greater_than(NativeFuncInData);
inline AObject* op_less_than_eq(NativeFuncInData);
inline AObject* op_greater_than_eq(NativeFuncInData);
inline AObject* op_eqeq(NativeFuncInData);
inline AObject* op_not_eq(NativeFuncInData);
inline AObject* op_eq_pointer(NativeFuncInData);
inline AObject* op_not_eq_pointer(NativeFuncInData);

#define create_operator_func_plus(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->i) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->i) op (obj2->f));\
				default:\
					if (obj2->type == AutoLang::DefaultClass::stringClassId)\
						return manager.create(AString::plus((obj1->i), (static_cast<AString*>(obj2->ref))));\
					else\
						break;\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->f) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->f) op (obj2->f));\
				default:\
					if (obj2->type == AutoLang::DefaultClass::stringClassId)\
						return manager.create(AString::plus((obj1->f), (static_cast<AString*>(obj2->ref))));\
					else\
						break;\
			}\
		}\
		default: {\
			auto obj2 = stackAllocator[1];\
			if (obj1->type == AutoLang::DefaultClass::stringClassId) {\
				switch (obj2->type) {\
					case AutoLang::DefaultClass::INTCLASSID:\
						return manager.create((*static_cast<AString*>(obj1->ref)) op (obj2->i));\
					case AutoLang::DefaultClass::FLOATCLASSID:\
						return manager.create((*static_cast<AString*>(obj1->ref)) op (obj2->f));\
					default:\
						if (obj2->type == AutoLang::DefaultClass::stringClassId)\
							return manager.create((*static_cast<AString*>(obj1->ref)) op (static_cast<AString*>(obj2->ref)));\
						else\
							break;\
				}\
			} else break;\
		}\
	}\
	return nullptr;\
}

#define create_operator_number(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->i) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->i) op (obj2->f));\
				default:\
					break;\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->f) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->f) op (obj2->f));\
				default:\
					break;\
			}\
		}\
		default: \
			break;\
	}\
	return nullptr;\
}

AObject* mod(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::INTCLASSID: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create((obj1->i) % (obj2->i));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->i), (obj2->f))));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::FLOATCLASSID: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->f), (obj2->i))));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->f), (obj2->f))));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	throw std::runtime_error("Cannot mod 2 variable");
}

create_operator_func_plus(plus, +);
create_operator_number(minus, -);
create_operator_number(mul, *);
create_operator_number(divide, /);

#define create_operator_func_plus_equal(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	auto obj2 = stackAllocator[1];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					obj1->i op obj2->i;\
					return nullptr;\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					obj1->i op obj2->f;\
					return nullptr;\
				default:\
					obj1->i op obj2->b;\
					return nullptr;\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					obj1->f op obj2->i;\
					return nullptr;\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					obj1->f op obj2->f;\
					return nullptr;\
				default:\
					obj1->f op obj2->b;\
					return nullptr;\
			}\
		}\
		default: {\
			return nullptr;\
		}\
	}\
	return nullptr;\
}

#define create_operator_func_number_equal(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	auto obj2 = stackAllocator[1];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					obj1->i op obj2->i;\
					return nullptr;\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					obj1->i op obj2->f;\
					return nullptr;\
				default:\
					obj1->i op obj2->b;\
					return nullptr;\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					obj1->f op obj2->i;\
					return nullptr;\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					obj1->f op obj2->f;\
					return nullptr;\
				default:\
					obj1->f op obj2->b;\
					return nullptr;\
			}\
		}\
	}\
	return nullptr;\
}

create_operator_func_plus_equal(plus_eq, +=);
create_operator_func_number_equal(minus_eq, -=);
create_operator_func_number_equal(mul_eq, *=);
create_operator_func_number_equal(divide_eq, /=);

AObject* plus_plus(NativeFuncInData) {
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			++obj->i;
			break;
		case AutoLang::DefaultClass::FLOATCLASSID:
			++obj->f;
			break;
		default:
			throw std::runtime_error("Cannot plus plus ");
	}
	return obj;
}

AObject* minus_minus(NativeFuncInData) {
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			--obj->i;
			break;
		case AutoLang::DefaultClass::FLOATCLASSID:
			--obj->f;
			break;
		default:
			throw std::runtime_error("Cannot plus plus ");
	}
	return obj;
}

AObject* negative(NativeFuncInData) {
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(-obj->i);
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(-obj->f);
		default:
			if (obj->type == AutoLang::DefaultClass::boolClassId)
				return manager.create(-static_cast<long long>(obj->b));
			throw std::runtime_error("Cannot negative");
	}
}

AObject* op_not(NativeFuncInData) {
	auto obj = stackAllocator[0];
	if (obj->type == AutoLang::DefaultClass::boolClassId)
		return manager.create(!obj->b);
	if (obj->type == AutoLang::DefaultClass::nullClassId)
		return manager.create(true);
	throw std::runtime_error("Cannot use not oparator");
}

AObject* op_and_and(NativeFuncInData) {
	return ObjectManager::create(stackAllocator[0]->b && stackAllocator[1]->b);
}

AObject* op_or_or(NativeFuncInData) {
	return ObjectManager::create(stackAllocator[0]->b || stackAllocator[1]->b);
}

AObject* op_eq_pointer(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1 == obj2);
}

AObject* op_not_eq_pointer(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1 != obj2);
}

create_operator_number(op_less_than, <);
create_operator_number(op_greater_than, >);
create_operator_number(op_less_than_eq, <=);
create_operator_number(op_greater_than_eq, >=);
create_operator_number(op_eqeq, ==);
create_operator_number(op_not_eq, !=);

}
}

#endif