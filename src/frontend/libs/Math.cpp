#ifndef LIB_MATH_CPP
#define LIB_MATH_CPP

#include "./Math.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/Type.hpp"

namespace AutoLang {
namespace Libs {
namespace Math {

void init(AutoLang::ACompiler &compiler) {
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
	nativeMap.emplace("fmod", &fmod);

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
			static func pow(base: Float, exp: Float): Float
			@native("abs")
			static func abs(value: Int): Int
			@native("abs")
			static func abs(value: Float): Float
			@native("sin")
			static func sin(value: Float): Float
			@native("sin")
			static func sin(value: Int): Float
			@native("cos")
			static func cos(value: Float): Float
			@native("cos")
			static func cos(value: Int): Float
			@native("tan")
			static func tan(value: Float): Float
			@native("tan")
			static func tan(value: Int): Float
			@native("fmod")
			static func fmod(num1: Float, num2: Float): Float
		}
	)###",
	                            std::move(nativeMap));
}

AObject *abs(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(static_cast<int64_t>(std::abs(obj->i)));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createFloat(std::abs(obj->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
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
					return notifier.createFloat(std::pow(obj1->i, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat(std::pow(obj1->i, obj2->f));
				default: {
					notifier.throwException("Wrong type");
					return nullptr;
				}
			}
		case AutoLang::DefaultClass::floatClassId:
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat(std::pow(obj1->f, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat(std::pow(obj1->f, obj2->f));
				default: {
					notifier.throwException("Wrong type");
					return nullptr;
				}
			}
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *round(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(obj1->i);
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createInt(std::round(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *floor(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(obj1->i);
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createInt(std::floor(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *ceil(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(obj1->i);
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createInt(std::ceil(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *sin(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createFloat(std::sin(obj1->i));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createFloat(std::sin(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *cos(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createFloat(std::cos(obj1->i));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createFloat(std::cos(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *tan(NativeFuncInData) {
	auto obj1 = args[0];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createFloat(std::tan(obj1->i));
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createFloat(std::tan(obj1->f));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *trunc(NativeFuncInData) {
	auto obj = args[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::intClassId:
			return notifier.createInt(obj->i);
		case AutoLang::DefaultClass::floatClassId:
			return notifier.createInt(static_cast<int64_t>(std::trunc(obj->f)));
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
	}
}

AObject *fmod(NativeFuncInData) {
	// base
	auto obj1 = args[0];
	// input
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId:
			switch (obj1->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat(
					    static_cast<int64_t>(std::fmod(obj1->i, obj2->i)));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat(std::fmod(obj1->i, obj2->f));
				default: {
					notifier.throwException("Wrong type");
					return nullptr;
				}
			}
		case AutoLang::DefaultClass::floatClassId:
			switch (obj1->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat(std::fmod(obj1->f, obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat(std::fmod(obj1->f, obj2->f));
				default: {
					notifier.throwException("Wrong type");
					return nullptr;
				}
			}
		default: {
			notifier.throwException("Wrong type");
			return nullptr;
		}
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