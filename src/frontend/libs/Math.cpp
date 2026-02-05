#ifndef LIB_MATH_CPP
#define LIB_MATH_CPP

#include "shared/Type.hpp"
#include "./Math.hpp"
#include "frontend/ACompiler.hpp"

namespace AutoLang {
namespace Libs {
namespace Math {

void init(AutoLang::ACompiler& compiler) {
	auto nativeMap = ANativeMap();
	nativeMap.reserve(9);

	nativeMap.emplace("round", &round);
	nativeMap.emplace("floor", &floor);
	nativeMap.emplace("ceil", &ceil);
	nativeMap.emplace("trunc", &trunc);
	nativeMap.emplace("pow", &pow);
	nativeMap.emplace("abs", &abs);
	nativeMap.emplace("sin", &sin);
	nativeMap.emplace("cos", &cos);
	nativeMap.emplace("tan", &tan);

	compiler.registerFromSource("std/math", false, R"###(

		
		class Math {
			@native("round")
			static func round(value: Float): Int
			@native("floor")
			static func floor(value: Float): Int
			@native("ceil")
			static func ceil(value: Float): Int
			@native("trunc")
			static func trunc(value: Float): Int
			@native("pow")
			static func pow(base: Int, exp: Int): Int
			@native("pow")
			static func pow(base: Float, exp: Float): Float
			@native("abs")
			static func abs(value: Int): Int
			@native("abs")
			static func abs(value: Float): Float
			@native("sin")
			static func sin(value: Float): Float
			@native("cos")
			static func cos(value: Float): Float
			@native("tan")
			static func tan(value: Float): Float
		}
	)###", std::move(nativeMap));
}

AObject *abs(NativeFuncInData) {
	if (size != 1)
		return nullptr;
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return manager.create(static_cast<int64_t>(std::abs(obj->i)));
		case AutoLang::DefaultClass::floatClassId:
			return manager.create(std::abs(obj->f));
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

AObject *pow(NativeFuncInData) {
	if (size != 2)
		return nullptr;
	// base
	auto obj1 = args[0];
	// input
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return manager.create(integer_pow(obj1->i, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(std::pow(obj1->i, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		case AutoLang::DefaultClass::floatClassId:
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return manager.create(std::pow(obj1->f, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(std::pow(obj1->f, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

#define in_num_out_int(name, func)                                             \
	AObject *name(NativeFuncInData) {                                          \
		if (size != 1)                                                         \
			return nullptr;                                                    \
		auto obj = args[0];                                                    \
		switch (obj->type) {                                                   \
			case AutoLang::DefaultClass::intClassId:                           \
				return manager.create(static_cast<int64_t>(func(obj->i)));     \
			case AutoLang::DefaultClass::floatClassId:                         \
				return manager.create(static_cast<int64_t>(func(obj->f)));     \
			default:                                                           \
				throw std::runtime_error("Cannot run with this type");         \
		}                                                                      \
	}

#define in_num_out_float(name, func)                                           \
	AObject *name(NativeFuncInData) {                                          \
		if (size != 1)                                                         \
			return nullptr;                                                    \
		auto obj = args[0];                                                    \
		switch (obj->type) {                                                   \
			case AutoLang::DefaultClass::intClassId:                           \
				return manager.create(static_cast<double>(func(obj->i)));      \
			case AutoLang::DefaultClass::floatClassId:                         \
				return manager.create(static_cast<double>(func(obj->f)));      \
			default:                                                           \
				throw std::runtime_error("Cannot run with this type");         \
		}                                                                      \
	}

in_num_out_int(round, std::round);
in_num_out_int(floor, std::floor);
in_num_out_int(ceil, std::ceil);
in_num_out_float(sin, std::sin);
in_num_out_float(cos, std::cos);
in_num_out_float(tan, std::tan);

AObject *trunc(NativeFuncInData) {
	if (size != 1)
		return nullptr;
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return manager.create(obj->i);
		case AutoLang::DefaultClass::floatClassId:
			return manager.create(static_cast<int64_t>(std::trunc(obj->f)));
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

AObject *fmod(NativeFuncInData) {
	if (size != 2)
		return nullptr;
	// base
	auto obj1 = args[0];
	// input
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			switch (obj1->type) {
				case AutoLang::DefaultClass::intClassId:
					return manager.create(
					    static_cast<int64_t>(std::fmod(obj1->i, obj2->i)));
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(std::fmod(obj1->i, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		case AutoLang::DefaultClass::floatClassId:
			switch (obj1->type) {
				case AutoLang::DefaultClass::intClassId:
					return manager.create(std::fmod(obj1->f, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(std::fmod(obj1->f, obj2->f));
				default:
					throw std::runtime_error("Cannot run with this type");
			}
		default:
			throw std::runtime_error("Cannot run with this type");
	}
}

int64_t integer_pow(int64_t base, int64_t exp) {
	if (exp < 0)
		throw std::runtime_error("Exponent must be non-negative");

	int64_t result = 1;
	while (exp > 0) {
		if (exp & 1)
			result *= base;
		base *= base;
		exp >>= 1;
	}
	return result;
}

} // namespace Math
} // namespace Libs
} // namespace AutoLang
#endif