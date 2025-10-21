#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <unordered_map>

template<typename T, typename R>
inline bool isMapExist(std::unordered_map<T, R>& map, T& obj) {
    auto it = map.find(obj);
    return it != map.end();
}

#endif