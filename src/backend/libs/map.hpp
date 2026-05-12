#ifndef LIBS_MAP_HPP
#define LIBS_MAP_HPP

#include "shared/AObject.hpp"
#include "shared/Type.hpp"


namespace AutoLang {
class ACompiler;
namespace Libs {
namespace map {

struct AHashMap {
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
std::string to_string(ANotifier &notifier, AObject *obj);
} // namespace map
} // namespace Libs
} // namespace AutoLang

#endif