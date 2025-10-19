#ifndef LIB_MATH_CPP
#define LIB_MATH_CPP

#include "./Math.hpp"

namespace AutoLang {
namespace Libs {
namespace Math {
	
void init(CompiledProgram& compile) {
	auto math = &compile.classes[
		compile.registerClass("Math")
	];
	compile.registerFunction(
		math,
		true,
		"round()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&round
	);
	compile.registerFunction( 
		math,
		true,
		"floor()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&floor
	);
	compile.registerFunction( 
		math,
		true,
		"ceil()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&ceil
	);
	compile.registerFunction( 
		math,
		true,
		"trunc()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&trunc
	);
	compile.registerFunction( 
		math,
		true,
		"pow()",
		{ AutoLang::DefaultClass::INTCLASSID , AutoLang::DefaultClass::INTCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&pow
	);
	compile.registerFunction( 
		math,
		true,
		"pow()",
		{ AutoLang::DefaultClass::FLOATCLASSID , AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&pow
	);
	compile.registerFunction( 
		math,
		true,
		"abs()",
		{ AutoLang::DefaultClass::INTCLASSID }, 
		AutoLang::DefaultClass::INTCLASSID,
		&abs
	);
	compile.registerFunction( 
		math,
		true,
		"abs()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&abs
	);
	compile.registerFunction( 
		math,
		true,
		"sin()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&sin
	);
	compile.registerFunction( 
		math,
		true,
		"cos()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&cos
	);
	compile.registerFunction( 
		math,
		true,
		"tan()",
		{ AutoLang::DefaultClass::FLOATCLASSID }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&tan
	);
}

AObject* abs(NativeFuncInData) {
	if (size != 1) return nullptr;
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(static_cast<long long>(std::abs(obj->i)));
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(std::abs(obj->f));
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

AObject* pow(NativeFuncInData) {
	if (size != 2) return nullptr;
	//base
	auto obj1 = stackAllocator[0];
	//input
	auto obj2 = stackAllocator[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(integer_pow(obj1->i, obj2->i));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(std::pow(obj1->i, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		case AutoLang::DefaultClass::FLOATCLASSID:
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(std::pow(obj1->f, obj2->i));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(std::pow(obj1->f, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

#define in_num_out_int(name, func) AObject* name(NativeFuncInData) {\
	if (size != 1) return nullptr;\
	auto obj = stackAllocator[0];\
	switch (obj->type) {\
		case AutoLang::DefaultClass::INTCLASSID:\
			return manager.create(static_cast<long long>(func(obj->i)));\
		case AutoLang::DefaultClass::FLOATCLASSID:\
			return manager.create(static_cast<long long>(func(obj->f)));\
		default:\
			throw std::runtime_error("Cannot run with this type");\
	}\
}

#define in_num_out_float(name, func) AObject* name(NativeFuncInData) {\
	if (size != 1) return nullptr;\
	auto obj = stackAllocator[0];\
	switch (obj->type) {\
		case AutoLang::DefaultClass::INTCLASSID:\
			return manager.create(static_cast<double>(func(obj->i)));\
		case AutoLang::DefaultClass::FLOATCLASSID:\
			return manager.create(static_cast<double>(func(obj->f)));\
		default:\
			throw std::runtime_error("Cannot run with this type");\
	}\
}

in_num_out_int(round, std::round);
in_num_out_int(floor, std::floor);
in_num_out_int(ceil, std::ceil);
in_num_out_float(sin, std::sin);
in_num_out_float(cos, std::cos);
in_num_out_float(tan, std::tan);

AObject* trunc(NativeFuncInData) {
	if (size != 1) return nullptr;
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(obj->i);
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(static_cast<long long>(std::trunc(obj->f)));
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

AObject* fmod(NativeFuncInData) {
	if (size != 2) return nullptr;
	//base
	auto obj1 = stackAllocator[0];
	//input
	auto obj2 = stackAllocator[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			switch (obj1->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(static_cast<long long>(std::fmod(obj1->i, obj2->i)));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(std::fmod(obj1->i, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		case AutoLang::DefaultClass::FLOATCLASSID:
			switch (obj1->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(std::fmod(obj1->f, obj2->i));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(std::fmod(obj1->f, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

long long integer_pow(long long base, long long exp) {
	if (exp < 0)
		throw std::runtime_error("Exponent must be non-negative");

	long long result = 1;
	while (exp > 0) {
		if (exp & 1)
			result *= base;
		base *= base;
		exp >>= 1;
	}
	return result;
}

}
}
}
#endif