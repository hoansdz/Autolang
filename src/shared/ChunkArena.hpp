#ifndef CHUNK_ARENA_HPP
#define CHUNK_ARENA_HPP

#include "shared/AObject.hpp"

template <typename T, size_t size> class ChunkArena {
  public:
	struct ListChunk {
		T *chunkArena = static_cast<T *>(::operator new(sizeof(T) * size));
		ListChunk *next = nullptr;
	};

	ListChunk *listChunk = nullptr;
	uint32_t index = 0;

	ChunkArena() {}
	template <typename... Args> inline T *push(Args &&...args) {
		if (!listChunk) {
			listChunk = new ListChunk();
			T *obj = &listChunk->chunkArena[index++];
			new (obj) T(std::forward<Args>(args)...);
			return obj;
		}
		T *obj = &listChunk->chunkArena[index++];
		new (obj) T(std::forward<Args>(args)...);
		if (index == size) {
			auto old = listChunk;
			listChunk = new ListChunk();
			listChunk->next = old;
			index = 0;
		}
		return obj;
	}
	void destroy() {
		if (listChunk) {
			for (size_t i = 0; i < index; ++i) {
				listChunk->chunkArena[i].~T();
			}
			::operator delete(listChunk->chunkArena);
			ListChunk *current = listChunk->next;
			delete listChunk;
			listChunk = nullptr;
			while (current != nullptr) {
				for (size_t i = 0; i < size; ++i) {
					current->chunkArena[i].~T();
				}
				::operator delete(current->chunkArena);
				auto old = current;
				current = current->next;
				delete old;
			}
			index = 0;
		}
	}
	~ChunkArena() { destroy(); }
};

#endif