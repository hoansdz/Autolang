#ifndef LIBS_MAP_HPP
#define LIBS_MAP_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace map {
AObject *constructor(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *get(NativeFuncInData);
AObject *set(NativeFuncInData);
AObject *clear(NativeFuncInData);
void init(ACompiler &compiler);
} // namespace list
} // namespace Libs
} // namespace AutoLang

#endif