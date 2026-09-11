#ifndef LIBS_SET_HPP
#define LIBS_SET_HPP

#include "shared/AObject.hpp"
#include "shared/Type.hpp"

namespace Autolang {
class ANotifier;
namespace Libs {
namespace set {
struct AUnorderedSet {
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

using IntHashSet = HashSet<int64_t>;
using FloatHashSet = HashSet<double>;
using StringHashSet = HashSet<AObject *, ObjStringHashable, ObjStringEqualable>;
using ObjectHashSet = HashSet<AObject *, AObjectHashable, AObjectEqualable>;
AObject *constructor(ANotifier &notifier, ClassId classId, ClassId keyId);
AObject *constructor(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *is_empty(NativeFuncInData);
AObject *set_union(NativeFuncInData);
AObject *intersect(NativeFuncInData);
AObject *difference(NativeFuncInData);
AObject *for_each(NativeFuncInData);
AObject *to_array(NativeFuncInData);
AObject *contains(NativeFuncInData);
AObject *add(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *to_string(NativeFuncInData);
AObject *clone(NativeFuncInData);
AObject *filter(NativeFuncInData);
AObject *map(NativeFuncInData);
AObject *first(NativeFuncInData);
AObject *first_or_null(NativeFuncInData);
AObject *any(NativeFuncInData);
AObject *any_fn(NativeFuncInData);
AObject *all_fn(NativeFuncInData);
AObject *none(NativeFuncInData);
AObject *none_fn(NativeFuncInData);
std::string to_string(ANotifier &notifier, AObject *obj);
} // namespace set
} // namespace Libs
} // namespace Autolang

#endif