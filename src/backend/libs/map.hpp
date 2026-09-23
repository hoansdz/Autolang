#ifndef LIBS_MAP_HPP
#define LIBS_MAP_HPP

#include "shared/AObject.hpp"
#include "shared/Type.hpp"

namespace Autolang {
class ACompiler;
namespace Libs {
namespace map {

struct AHashMap {
	ClassId type;
	void *data;
};

struct ObjStringHashable {
	inline size_t operator()(const AObject *s) const {
		uint64_t h = 14695981039346656037ULL;
		for (size_t i = 0; i < s->str->size; ++i) {
			h ^= (unsigned char)s->str->data[i];
			h *= 1099511628211ULL;
		}
		return static_cast<size_t>(h);
	}
};

struct ObjStringEqualable {
	inline bool operator()(const AObject *a, const AObject *b) const {
		return a->str->size == b->str->size &&
		       memcmp(a->str->data, b->str->data, a->str->size) == 0;
	}
};

using IntHashMap = HashMap<int64_t, AObject *>;
using FloatHashMap = HashMap<double, AObject *>;
using StringHashMap =
    HashMap<AObject *, AObject *, ObjStringHashable, ObjStringEqualable>;
using ObjectHashMap =
    HashMap<AObject *, AObject *, AObjectHashable, AObjectEqualable>;

AObject *constructor(ANotifier &notifier, ClassId classId, ClassId keyId);
AObject *constructor(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *is_empty(NativeFuncInData);
AObject *contains_key(NativeFuncInData);
AObject *for_each(NativeFuncInData);
AObject *keys(NativeFuncInData);
AObject *values(NativeFuncInData);
AObject *get(NativeFuncInData);
AObject *get_or_default(NativeFuncInData);
AObject *set(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *to_string(NativeFuncInData);
AObject *clone(NativeFuncInData);
AObject *filter(NativeFuncInData);
AObject *plus(NativeFuncInData);
AObject *minus(NativeFuncInData);
AObject *is_not_empty(NativeFuncInData);
AObject *contains_value(NativeFuncInData);
AObject *get_or_put(NativeFuncInData);
AObject *get_or_else(NativeFuncInData);
AObject *filter_keys(NativeFuncInData);
AObject *filter_values(NativeFuncInData);
AObject *map_values(NativeFuncInData);
AObject *map_keys(NativeFuncInData);
AObject *plus_pair(NativeFuncInData);
AObject *entries(NativeFuncInData);
AObject *put_all(NativeFuncInData);
AObject *remove_pair(NativeFuncInData);
AObject *to_list(NativeFuncInData);
AObject *any_fn(NativeFuncInData);
AObject *all_fn(NativeFuncInData);
AObject *none_fn(NativeFuncInData);
AObject *filter_not(NativeFuncInData);
std::string to_string(ANotifier &notifier, AObject *obj);
} // namespace map
} // namespace Libs
} // namespace Autolang

#endif