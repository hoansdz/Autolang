#ifndef OBJECTMANAGER_CPP
#define OBJECTMANAGER_CPP

#include "ObjectManager.hpp"

namespace AutoLang {

void ObjectManager::destroy() {
    for (size_t i = 0; i < intObjects.index; ++i) {
        intObjects.objects[i]->flags = AObject::Flags::OBJ_IS_FREE;
    }
    for (size_t i = 0; i < floatObjects.index; ++i) {
        floatObjects.objects[i]->flags = AObject::Flags::OBJ_IS_FREE;
    }
    areaAllocator.destroy();
}

}

#endif