#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include "third_party/ankerl/unordered_dense.h"

template <typename Map, typename Key>
inline bool isMapExist(const Map &map, const Key &obj)
{
    auto it = map.find(obj);
    return it != map.end();
}

#endif