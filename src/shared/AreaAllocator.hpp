#ifndef AREAALLOCATOR_HPP
#define AREAALLOCATOR_HPP

#include "shared/AObject.hpp"

template <typename T, size_t size> class AreaAllocator {
  public:
	struct AreaChunkSlot {
		T obj;
		AreaChunkSlot *nextFree;
		bool isFree;
		AreaChunkSlot() : isFree(true), nextFree(nullptr) {}
	};

	struct AreaChunk {
		AreaChunkSlot data[size];
		AreaChunk *next;
		AreaChunk() : next(nullptr) {}
	};

  private:
	AreaChunk *head;
	AreaChunkSlot *freeSlot;

  public:
	AreaAllocator() : head(nullptr), freeSlot(nullptr) {}
	inline T *getObject() {
		if (freeSlot != nullptr) {
			freeSlot->isFree = false;
			auto *obj = &freeSlot->obj;
			freeSlot = freeSlot->nextFree;
			return obj;
		}
		auto *newChunk = new AreaChunk();
		newChunk->data[0].isFree = false;
		newChunk->next = head;
		head = newChunk;

		constexpr size_t s = size - 1;
		for (size_t i = 1; i < s; ++i) {
			newChunk->data[i].nextFree = &newChunk->data[i + 1];
		}
		freeSlot = &newChunk->data[1];
		return &newChunk->data[0].obj;
	}
	inline void release(T *obj) {
		AreaChunkSlot *slot = reinterpret_cast<AreaChunkSlot *>(obj);
		slot->isFree = true;
		slot->nextFree = freeSlot;
		freeSlot = slot;
	}
	void destroy();
};

#endif