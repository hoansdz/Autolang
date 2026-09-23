#ifndef LIB_REGEX_HPP
#define LIB_REGEX_HPP

#include "shared/Type.hpp"
#include <regex>

namespace Autolang {
class ACompiler;

namespace Libs {
namespace regex {

void init(Autolang::ACompiler &compiler);

AObject *constructor(NativeFuncInData);
AObject *is_match(NativeFuncInData);
AObject *match_entire(NativeFuncInData);
AObject *find(NativeFuncInData);
AObject *find_all(NativeFuncInData);
AObject *replace(NativeFuncInData);
AObject *replace_eval(NativeFuncInData);
AObject *split(NativeFuncInData);
AObject *get_pattern(NativeFuncInData);

} // namespace RegexLib
} // namespace Libs
} // namespace Autolang
#endif