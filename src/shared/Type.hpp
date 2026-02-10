#ifndef TYPE_HPP
#define TYPE_HPP

#include "ankerl/unordered_dense.h"
#include <iostream>

namespace AutoLang {
struct AObject;
class ANotifier;
class ACompiler;
}

#define NativeFuncInput AutoLang::ANotifier&, AutoLang::AObject ** ,size_t
#define NativeFuncInData AutoLang::ANotifier& notifier, AutoLang::AObject **args, size_t size

using InitLibFn = const char* (*)();
using ClassId = uint32_t;
using LexerStringId = uint32_t;
using Offset = uint32_t;
using BytecodePos = uint32_t;
using MemberOffset = uint32_t;
using HashValue = int64_t;
template<
    class K,
    class V,
    class Hash  = ankerl::unordered_dense::hash<K>,
    class Equal = std::equal_to<K>
>
using HashMap = ankerl::unordered_dense::map<K, V, Hash, Equal>;
using ANativeFunction = AutoLang::AObject*(*)(NativeFuncInput);
using ANativeMap = HashMap<std::string, ANativeFunction>;

#endif