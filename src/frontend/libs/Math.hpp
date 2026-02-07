#ifndef LIB_MATH_HPP
#define LIB_MATH_HPP

#include <cmath>

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace Math {

void init(AutoLang::ACompiler &compiler);
int64_t integer_pow(int64_t base, int64_t exp);
AObject *abs(NativeFuncInData);
AObject *pow(NativeFuncInData);
AObject *round(NativeFuncInData);
AObject *floor(NativeFuncInData);
AObject *ceil(NativeFuncInData);
AObject *trunc(NativeFuncInData);
AObject *fmod(NativeFuncInData);
AObject *sin(NativeFuncInData);
AObject *cos(NativeFuncInData);
AObject *tan(NativeFuncInData);
inline bool isIntegerFloat(double x) {
    return std::floor(x) == x;
}

} // namespace Math
} // namespace Libs
} // namespace AutoLang
#endif