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
AObject *reversed(NativeFuncInData);
AObject *filter(NativeFuncInData);
AObject *sort(NativeFuncInData);
AObject *sort_default(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *is_empty(NativeFuncInData);
AObject *get(NativeFuncInData);
AObject *set(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *contains(NativeFuncInData);
AObject *to_string(NativeFuncInData);
AObject *join_to_string(NativeFuncInData);
AObject *clone(NativeFuncInData);
AObject *sorted(NativeFuncInData);
AObject *map(NativeFuncInData);
AObject *map_indexed(NativeFuncInData);
AObject *reduce(NativeFuncInData);
AObject *fold(NativeFuncInData);
AObject *first(NativeFuncInData);
AObject *first_or_null(NativeFuncInData);
AObject *last(NativeFuncInData);
AObject *last_or_null(NativeFuncInData);
AObject *take(NativeFuncInData);
AObject *drop(NativeFuncInData);
AObject *any(NativeFuncInData);
AObject *any_fn(NativeFuncInData);
AObject *all_fn(NativeFuncInData);
AObject *none(NativeFuncInData);
AObject *none_fn(NativeFuncInData);
AObject *count_fn(NativeFuncInData);
std::string to_string(ANotifier &notifier, AObject *obj);
} // namespace array
} // namespace Libs
} // namespace Autolang

#endif