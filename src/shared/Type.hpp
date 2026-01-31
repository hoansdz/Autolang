#ifndef TYPE_HPP
#define TYPE_HPP

#include "ankerl/unordered_dense.h"
#include <iostream>

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

#endif