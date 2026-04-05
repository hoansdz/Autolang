#ifndef LIB_REGEX_HPP
#define LIB_REGEX_HPP

#include "shared/Type.hpp"
#include <regex>

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace regex {

void init(AutoLang::ACompiler &compiler);

AObject *constructor(NativeFuncInData);
AObject *is_match(NativeFuncInData);
AObject *find_all(NativeFuncInData);
AObject *replace(NativeFuncInData);

} // namespace RegexLib
} // namespace Libs
} // namespace AutoLang
#endif