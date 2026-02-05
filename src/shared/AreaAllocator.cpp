#ifndef AREAALLOCATOR_CPP
#define AREAALLOCATOR_CPP

#include "AreaAllocator.hpp"

template <typename T, size_t size> void AreaAllocator<T, size>::destroy() {
	auto *currentChunk = head;
	if constexpr (std::is_same_v<T, AutoLang::AObject>) {
		// Free all slot
		while (currentChunk != nullptr) {
			for (size_t i = 0; i < size; ++i) {
				auto &slot = currentChunk->data[i];
				if (slot.isFree)
					continue;
				slot.obj.template free<true>();
			}
			currentChunk = currentChunk->next;
		}
		currentChunk = head;
	}

	// Free chunk
	while (currentChunk != nullptr) {
		auto *nextChunk = currentChunk->next;
		delete currentChunk;
		currentChunk = nextChunk;
	}
	freeSlot = nullptr;
	head = nullptr;
}

#endif