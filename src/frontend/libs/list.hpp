#ifndef LIBS_LIST_HPP
#define LIBS_LIST_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace list {
AObject *add(NativeFuncInData);
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