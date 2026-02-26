#ifndef AREA_ALLOCATOR_CPP
#define AREA_ALLOCATOR_CPP

#include "AreaAllocator.hpp"

namespace AutoLang {

template <size_t size> void AreaAllocator<size>::destroy(ANotifier& notifier) {
	auto *currentChunk = head;
	// Free all slot
	while (currentChunk != nullptr) {
		for (size_t i = 0; i < size; ++i) {
			auto &slot = currentChunk->data[i];
			if (slot.obj.flags & AObject::Flags::OBJ_IS_FREE)
				continue;
			slot.obj.template free<true>(notifier);
		}
		currentChunk = currentChunk->next;
	}
	currentChunk = head;

	// Free chunk
	while (currentChunk != nullptr) {
		auto *nextChunk = currentChunk->next;
		delete currentChunk;
		currentChunk = nextChunk;
	}
	freeSlot = nullptr;
	head = nullptr;
	countObject = 0;
}

}

#endif