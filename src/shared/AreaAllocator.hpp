#ifndef AREA_ALLOCATOR_HPP
#define AREA_ALLOCATOR_HPP

#include "shared/AObject.hpp"

namespace Autolang {

template <size_t size> class AreaAllocator {
  public:
	struct AreaChunkSlot {
		AObject obj;
		AreaChunkSlot *nextFree;
		AreaChunkSlot() : obj(), nextFree(nullptr) {
			obj.flags = AObject::Flags::OBJ_IS_FREE;
		}
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
	size_t countObject;
	size_t maxManagedMemory;
	size_t currentManagedMemory;
	bool changedMemory;
	AreaAllocator()
	    : head(nullptr), freeSlot(nullptr), countObject(0),
	      maxManagedMemory(32 * 1024 * 1024), currentManagedMemory(0),
	      changedMemory(false) {}

	inline AObject *getObject() {
		currentManagedMemory += sizeof(AObject);
		changedMemory = true;
		if (freeSlot != nullptr) {
			auto *obj = &freeSlot->obj;
			obj->flags = 0;
			freeSlot = freeSlot->nextFree;
			return obj;
		}
		countObject += size;
		auto *newChunk = new AreaChunk();
		newChunk->data[0].obj.flags = 0;
		newChunk->next = head;
		head = newChunk;

		constexpr size_t s = size - 1;
		for (size_t i = 1; i < s; ++i) {
			newChunk->data[i].nextFree = &newChunk->data[i + 1];
		}
		freeSlot = &newChunk->data[1];
		return &newChunk->data[0].obj;
	}
	inline void release(AObject *obj) {
		if (currentManagedMemory >= sizeof(AObject)) {
			currentManagedMemory -= sizeof(AObject);
		} else {
			currentManagedMemory = 0;
		}
		changedMemory = true;
		AreaChunkSlot *slot = reinterpret_cast<AreaChunkSlot *>(obj);
		obj->flags = AObject::Flags::OBJ_IS_FREE;
		slot->nextFree = freeSlot;
		freeSlot = slot;
	}

	inline void addManagedMemory(int64_t delta) {
		if (delta >= 0) {
			currentManagedMemory += static_cast<size_t>(delta);
		} else {
			size_t neg = static_cast<size_t>(-delta);
			if (currentManagedMemory >= neg) {
				currentManagedMemory -= neg;
			} else {
				currentManagedMemory = 0;
			}
		}
		changedMemory = true;
	}

	void destroy(ANotifier &notifier);
};

} // namespace Autolang

#endif