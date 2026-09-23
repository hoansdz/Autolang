#ifndef LIB_MATH_HPP
#define LIB_MATH_HPP

#include "shared/Type.hpp"
#include <cmath>

namespace Autolang {
class ACompiler;

namespace Libs {
namespace Math {

void init(Autolang::ACompiler &compiler);
int64_t integer_pow(int64_t base, int64_t exp);

AObject *abs(NativeFuncInData);
AObject *pow(NativeFuncInData);
AObject *round(NativeFuncInData);
AObject *round_to_int(NativeFuncInData);
AObject *floor(NativeFuncInData);
AObject *ceil(NativeFuncInData);
AObject *trunc(NativeFuncInData);
AObject *fmod(NativeFuncInData);

AObject *sin(NativeFuncInData);
AObject *cos(NativeFuncInData);
AObject *tan(NativeFuncInData);

AObject *random(NativeFuncInData);
AObject *sqrt(NativeFuncInData);
AObject *exp(NativeFuncInData);
AObject *log(NativeFuncInData);
AObject *m_min(NativeFuncInData);
AObject *m_max(NativeFuncInData);

AObject *sign(NativeFuncInData);
AObject *ln(NativeFuncInData);
AObject *log2(NativeFuncInData);
AObject *log10(NativeFuncInData);
AObject *asin(NativeFuncInData);
AObject *acos(NativeFuncInData);
AObject *atan(NativeFuncInData);
AObject *atan2(NativeFuncInData);
AObject *sinh(NativeFuncInData);
AObject *cosh(NativeFuncInData);
AObject *tanh(NativeFuncInData);
AObject *asinh(NativeFuncInData);
AObject *acosh(NativeFuncInData);
AObject *atanh(NativeFuncInData);
AObject *hypot(NativeFuncInData);
AObject *ieee_rem(NativeFuncInData);
AObject *cbrt(NativeFuncInData);
AObject *expm1(NativeFuncInData);
AObject *ln1p(NativeFuncInData);
AObject *log_base(NativeFuncInData);
AObject *with_sign(NativeFuncInData);
AObject *next_up(NativeFuncInData);
AObject *next_down(NativeFuncInData);
AObject *next_after(NativeFuncInData);
AObject *ulp(NativeFuncInData);

inline bool isIntegerFloat(double x) { return std::floor(x) == x; }

} // namespace Math
} // namespace Libs
} // namespace Autolang
#endif