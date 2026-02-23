#ifndef AREA_ALLOCATOR_HPP
#define AREA_ALLOCATOR_HPP

#include "shared/AObject.hpp"

namespace AutoLang {

template <size_t size> class AreaAllocator {
  public:
	struct AreaChunkSlot {
		AObject obj;
		AreaChunkSlot *nextFree;
		AreaChunkSlot() : obj(), nextFree(nullptr) {
			obj.flags &= AObject::Flags::OBJ_IS_FREE;
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
	AreaAllocator() : head(nullptr), freeSlot(nullptr) {}
	inline AObject *getObject() {
		if (freeSlot != nullptr) {
			auto *obj = &freeSlot->obj;
			obj->flags = 0;
			freeSlot = freeSlot->nextFree;
			return obj;
		}
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
		AreaChunkSlot *slot = reinterpret_cast<AreaChunkSlot *>(obj);
		obj->flags = AObject::Flags::OBJ_IS_FREE;
		slot->nextFree = freeSlot;
		freeSlot = slot;
	}
	void destroy();
};

} // namespace AutoLang

#endif