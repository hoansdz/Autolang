#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include "ankerl/unordered_dense.h"

template <typename T, typename R>
inline bool isMapExist(HashMap<T, R> &map, T &obj)
{
    auto it = map.find(obj);
    return it != map.end();
}

#endif