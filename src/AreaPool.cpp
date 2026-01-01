#ifndef AREAPOOL_CPP
#define AREAPOOL_CPP

#include "AreaPool.hpp"

AreaPool::~AreaPool() {
    //Free all slot
    auto* currentChunk = head;
    while (currentChunk != nullptr) {
        for (size_t i = 0; i < AreaChunk::size; ++i) {
            AreaChunkSlot& slot = currentChunk->data[i];
            if (slot.isFree) continue;
            slot.obj.free<true>();
        }
        currentChunk = currentChunk->next;
    }
    
    //Free chunk
    currentChunk = head;
    while (currentChunk != nullptr) {
        auto* nextChunk = currentChunk->next;
        delete currentChunk;
        currentChunk = nextChunk;
    }
}

#endif