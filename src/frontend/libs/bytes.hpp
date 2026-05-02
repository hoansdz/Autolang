#ifndef LIB_BYTES_HPP
#define LIB_BYTES_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace bytes {

void init(AutoLang::ACompiler &compiler);
AObject *constructor(NativeFuncInData);
AObject *resize(NativeFuncInData);
AObject *append(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *is_empty(NativeFuncInData);
AObject *get(NativeFuncInData);
AObject *set(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *slice(NativeFuncInData);
AObject *to_string(NativeFuncInData);

} // namespace file
} // namespace Libs
} // namespace AutoLang
#endif