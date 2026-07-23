#ifndef STRINGARENA_HPP
#define STRINGARENA_HPP

#include "shared/Type.hpp"
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>


class StringArena {
  public:
	char *buffer;
	uint32_t current_size;
	uint32_t capacity;
	static constexpr uint32_t initial_capacity = 1024;

//   public:
	StringArena() : current_size(0), capacity(initial_capacity) {
		buffer = static_cast<char *>(malloc(capacity));
	}

	~StringArena() { free(buffer); }

	StringArena(const StringArena &) = delete;
	StringArena &operator=(const StringArena &) = delete;

	StringArenaOffset push_back(std::string_view str) {
		uint32_t len = str.length();
		uint32_t needed_size = current_size + len + 1;
		if (needed_size > capacity) {
			while (capacity < needed_size) {
				capacity *= 2;
			}
			buffer = static_cast<char *>(realloc(buffer, capacity));
		}
		uint32_t offset = current_size;
		memcpy(buffer + offset, str.data(), len);
		buffer[offset + len] = '\0';

		current_size += len + 1;
		return offset;
	}

	const char *get(uint32_t offset) const { return buffer + offset; }

	void reset() {
		if (capacity == initial_capacity) {
			current_size = 0;
			return;
		}
    
		char* new_buffer = static_cast<char *>(realloc(buffer, initial_capacity));
		if (new_buffer) {
			buffer = new_buffer;
			capacity = initial_capacity;
		}
		current_size = 0;
	}
};

#endif