#ifndef LIBS_SET_HPP
#define LIBS_SET_HPP

#include "shared/Type.hpp"
#include "frontend/parser/node/Node.hpp"

namespace AutoLang {
class ANotifier;
namespace Libs {
namespace set {
struct AUnorderedSet {
	ClassId type;
	void *data;
};

struct ObjStringHashable {
	inline size_t operator()(const AObject *s) const {
		size_t h = 1469598103934665603ULL;
		for (size_t i = 0; i < s->str->size; ++i) {
			h ^= (unsigned char)s->str->data[i];
			h *= 1099511628211ULL;
		}
		return h;
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
using ObjectHashSet = HashSet<AObject *>;
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
} // namespace set
} // namespace Libs
} // namespace AutoLang

#endif