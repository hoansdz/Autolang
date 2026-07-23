#ifndef LIBS_LIST_HPP
#define LIBS_LIST_HPP

#include "shared/Type.hpp"

namespace Autolang {
class ACompiler;
namespace Libs {
namespace array {
AObject *add(NativeFuncInData);
AObject *reserve(NativeFuncInData);
AObject *insert(NativeFuncInData);
AObject *pop(NativeFuncInData);
AObject *index_of(NativeFuncInData);
AObject *for_each(NativeFuncInData);
AObject *for_each_with_index(NativeFuncInData);
AObject *slice(NativeFuncInData);
AObject *filter(NativeFuncInData);
AObject *sort(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *is_empty(NativeFuncInData);
AObject *get(NativeFuncInData);
AObject *set(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *contains(NativeFuncInData);
AObject *to_string(NativeFuncInData);
std::string to_string(ANotifier &notifier, AObject *obj);
} // namespace array
} // namespace Libs
} // namespace Autolang

#endif