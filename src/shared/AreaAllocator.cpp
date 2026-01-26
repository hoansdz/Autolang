#ifndef AREAALLOCATOR_CPP
#define AREAALLOCATOR_CPP

#include "AreaAllocator.hpp"

void AreaAllocator::destroy() {
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