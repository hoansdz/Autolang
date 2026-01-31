#ifndef OBJECTMANAGER_CPP
#define OBJECTMANAGER_CPP

#include "ObjectManager.hpp"

void ObjectManager::destroy() {
    for (size_t i = 0; i < intObjects.index; ++i) {
        reinterpret_cast<AreaAllocator<AObject, 64>::AreaChunkSlot*>(intObjects.objects[i])->isFree = true;
    }
    for (size_t i = 0; i < floatObjects.index; ++i) {
        reinterpret_cast<AreaAllocator<AObject, 64>::AreaChunkSlot*>(floatObjects.objects[i])->isFree = true;
    }
    areaAllocator.destroy();
}

#endif