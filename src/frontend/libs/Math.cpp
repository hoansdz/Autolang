#ifndef LIB_MATH_CPP
#define LIB_MATH_CPP

#include "math.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/Type.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

namespace AutoLang {
namespace Libs {
namespace Math {

static std::mt19937_64 &getRng() {
    static std::mt19937_64 rng(static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    return rng;
}

// Hàm tính luỹ thừa số nguyên
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

// --- PHẦN THÂN CÁC HÀM CŨ CỦA BẠN ---

AObject *random(NativeFuncInData) {
    auto &rng = getRng();

    if (argSize == 0) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return notifier.createFloat(dist(rng));
    }

    if (argSize == 2) {
        auto obj1 = args[0];
        auto obj2 = args[1];

        // Int range
        if (obj1->type == AutoLang::DefaultClass::intClassId &&
            obj2->type == AutoLang::DefaultClass::intClassId) {

            int64_t min = obj1->i;
            int64_t max = obj2->i;

            if (min > max) {
                notifier.throwException("min > max");
                return nullptr;
            }

            std::uniform_int_distribution<int64_t> dist(min, max);
            return notifier.createInt(dist(rng));
        }

        // Float range
        if ((obj1->type == AutoLang::DefaultClass::floatClassId ||
             obj1->type == AutoLang::DefaultClass::intClassId) &&
            (obj2->type == AutoLang::DefaultClass::floatClassId ||
             obj2->type == AutoLang::DefaultClass::intClassId)) {

            double min = (obj1->type == AutoLang::DefaultClass::intClassId)
                             ? obj1->i
                             : obj1->f;

            double max = (obj2->type == AutoLang::DefaultClass::intClassId)
                             ? obj2->i
                             : obj2->f;

            if (min > max) {
                notifier.throwException("min > max");
                return nullptr;
            }

            std::uniform_real_distribution<double> dist(min, max);
            return notifier.createFloat(dist(rng));
        }

        notifier.throwException("Wrong type");
        return nullptr;
    }

    notifier.throwException("Wrong number of arguments");
    return nullptr;
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

AObject *round(NativeFuncInData) {
    auto obj1 = args[0];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId:
            return notifier.createInt(obj1->i);
        case AutoLang::DefaultClass::floatClassId:
            return notifier.createInt(static_cast<int64_t>(std::round(obj1->f)));
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
            return notifier.createInt(static_cast<int64_t>(std::floor(obj1->f)));
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
            return notifier.createInt(static_cast<int64_t>(std::ceil(obj1->f)));
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

// --- PHẦN THÂN CÁC HÀM BỔ SUNG & CẬP NHẬT ---

AObject *pow(NativeFuncInData) {
    if (argSize != 2)
        return nullptr;
    auto obj1 = args[0];
    auto obj2 = args[1];

    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId:
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    try {
                        return notifier.createInt(
                            integer_pow(obj1->i, obj2->i));
                    } catch (const std::exception &e) {
                        notifier.throwException(e.what());
                        return nullptr;
                    }
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

AObject *fmod(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];

    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId:
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createFloat(std::fmod(obj1->i, obj2->i));
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createFloat(std::fmod(obj1->i, obj2->f));
                default: {
                    notifier.throwException("Wrong type");
                    return nullptr;
                }
            }
        case AutoLang::DefaultClass::floatClassId:
            switch (obj2->type) {
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

AObject *sqrt(NativeFuncInData) {
    auto obj = args[0];
    double val =
        (obj->type == AutoLang::DefaultClass::intClassId) ? obj->i : obj->f;
    if (val < 0) {
        notifier.throwException("Square root of negative number");
        return nullptr;
    }
    return notifier.createFloat(std::sqrt(val));
}

AObject *exp(NativeFuncInData) {
    auto obj = args[0];
    double val =
        (obj->type == AutoLang::DefaultClass::intClassId) ? obj->i : obj->f;
    return notifier.createFloat(std::exp(val));
}

AObject *log(NativeFuncInData) {
    auto obj = args[0];
    double val =
        (obj->type == AutoLang::DefaultClass::intClassId) ? obj->i : obj->f;
    if (val <= 0) {
        notifier.throwException("Logarithm of non-positive number");
        return nullptr;
    }
    return notifier.createFloat(std::log(val)); 
}

AObject *min(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];

    if (obj1->type == AutoLang::DefaultClass::intClassId &&
        obj2->type == AutoLang::DefaultClass::intClassId) {
        return notifier.createInt(std::min(obj1->i, obj2->i));
    }

    double v1 =
        (obj1->type == AutoLang::DefaultClass::intClassId) ? obj1->i : obj1->f;
    double v2 =
        (obj2->type == AutoLang::DefaultClass::intClassId) ? obj2->i : obj2->f;
    return notifier.createFloat(std::min(v1, v2));
}

AObject *max(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];

    if (obj1->type == AutoLang::DefaultClass::intClassId &&
        obj2->type == AutoLang::DefaultClass::intClassId) {
        return notifier.createInt(std::max(obj1->i, obj2->i));
    }

    double v1 =
        (obj1->type == AutoLang::DefaultClass::intClassId) ? obj1->i : obj1->f;
    double v2 =
        (obj2->type == AutoLang::DefaultClass::intClassId) ? obj2->i : obj2->f;
    return notifier.createFloat(std::max(v1, v2));
}

void init(AutoLang::ACompiler &compiler) {
    auto nativeMap = ANativeMap();
    nativeMap.reserve(20);

    nativeMap.emplace("round", &Math::round);
    nativeMap.emplace("floor", &Math::floor);
    nativeMap.emplace("ceil", &Math::ceil);
    nativeMap.emplace("trunc", &Math::trunc);
    nativeMap.emplace("pow", &Math::pow);
    nativeMap.emplace("abs", &Math::abs);
    nativeMap.emplace("sin", &Math::sin);
    nativeMap.emplace("cos", &Math::cos);
    nativeMap.emplace("tan", &Math::tan);
    nativeMap.emplace("fmod", &Math::fmod);
    nativeMap.emplace("random", &Math::random);

    nativeMap.emplace("sqrt", &Math::sqrt);
    nativeMap.emplace("min", &Math::min);
    nativeMap.emplace("max", &Math::max);
    nativeMap.emplace("log", &Math::log);
    nativeMap.emplace("exp", &Math::exp);

    compiler.registerFromSource("std/math", R"###(
@no_constructor
class Math {
	// --- HẰNG SỐ ---
	static val PI: Float = 3.141592653589793
	static val E: Float  = 2.718281828459045

	// --- HÀM CƠ BẢN ---
	@native("round") static func round(value: Float): Int
	@native("floor") static func floor(value: Float): Int
	@native("ceil")  static func ceil(value: Float): Int
	@native("trunc") static func trunc(value: Float): Int
	
	@native("abs") static func abs(value: Int): Int
	@native("abs") static func abs(value: Float): Float

	// --- LŨY THỪA & CĂN ---
	@native("pow") static func pow(base: Float, exp_: Float): Float
	@native("pow") static func pow(base: Int, exp_: Int): Int
	@native("sqrt") static func sqrt(value: Float): Float
	@native("sqrt") static func sqrt(value: Int): Float
	@native("exp") static func exp(value: Float): Float
	@native("log") static func log(value: Float): Float

	// --- LƯỢNG GIÁC ---
	@native("sin") static func sin(value: Float): Float
	@native("sin") static func sin(value: Int): Float
	@native("cos") static func cos(value: Float): Float
	@native("cos") static func cos(value: Int): Float
	@native("tan") static func tan(value: Float): Float
	@native("tan") static func tan(value: Int): Float

	// --- TIỆN ÍCH ---
	@native("fmod") static func fmod(num1: Float, num2: Float): Float
	@native("min") static func min(a: Int, b: Int): Int
	@native("min") static func min(a: Float, b: Float): Float
	@native("max") static func max(a: Int, b: Int): Int
	@native("max") static func max(a: Float, b: Float): Float

	// --- RANDOM ---
	@native("random") static func random(): Float
	@native("random") static func random(minValue: Int, maxValue: Int): Int
	@native("random") static func random(minValue: Float, maxValue: Float): Float
}
    )###",
                                false, std::move(nativeMap));
}

} // namespace Math
} // namespace Libs
} // namespace AutoLang
#endif