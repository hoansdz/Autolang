#ifndef LIB_MATH_HPP
#define LIB_MATH_HPP

#include <cmath>
#include "../Debugger.hpp"

namespace AutoLang
{
    namespace Libs
    {
        namespace Math
        {

            void init(CompiledProgram &compile);
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

        }
    }
}
#endif