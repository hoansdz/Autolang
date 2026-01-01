#ifndef OBJECTMANAGER_CPP
#define OBJECTMANAGER_CPP

#include "ObjectManager.hpp"

ObjectManager::~ObjectManager() {
    for (size_t i = 0; i < intObjects.index; ++i) {
        reinterpret_cast<AreaChunkSlot*>(intObjects.objects[i])->isFree = true;
    }
    for (size_t i = 0; i < floatObjects.index; ++i) {
        reinterpret_cast<AreaChunkSlot*>(floatObjects.objects[i])->isFree = true;
    }
}

#endif