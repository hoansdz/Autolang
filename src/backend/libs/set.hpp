#ifndef LIBS_SET_HPP
#define LIBS_SET_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ANotifier;
namespace Libs {
namespace set {
AObject *constructor(ANotifier& notifier, ClassId classId, ClassId keyId);
AObject *constructor(NativeFuncInData);
AObject *remove(NativeFuncInData);
AObject *size(NativeFuncInData);
AObject *contains(NativeFuncInData);
AObject *insert(NativeFuncInData);
AObject *clear(NativeFuncInData);
AObject *to_string(NativeFuncInData);
} // namespace list
} // namespace Libs
} // namespace AutoLang

#endif