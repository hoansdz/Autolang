#ifndef STRINGARENA_HPP
#define STRINGARENA_HPP

#include "shared/Type.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>

#ifdef PAGE_SIZE
#undef PAGE_SIZE
#endif

class StringArena {
public:
	static constexpr size_t ARENA_PAGE_SIZE = 8192; // 8 KB

private:
	std::vector<char *> pages;
	std::vector<char *> large_pages;
	size_t current_page_index = 0;
	size_t current_page_offset = ARENA_PAGE_SIZE; // Set = ARENA_PAGE_SIZE so the first allocation creates page 0

public:
	StringArena() = default;

	~StringArena() {
		clear();
	}

	StringArena(const StringArena &) = delete;
	StringArena &operator=(const StringArena &) = delete;

	void clear() {
		for (char *p : pages) {
			delete[] p;
		}
		pages.clear();
		for (char *lp : large_pages) {
			delete[] lp;
		}
		large_pages.clear();
		current_page_index = 0;
		current_page_offset = ARENA_PAGE_SIZE;
	}

	void reset() {
		clear();
	}

	// Allocate string in 8 KB page, returning char* pointer with stable address
	char *allocate(std::string_view str) {
		size_t len = str.length();
		size_t needed = len + 1;

		if (needed > ARENA_PAGE_SIZE) {
			// String larger than 8 KB page capacity: allocate dedicated large block
			char *large_buf = new char[needed];
			memcpy(large_buf, str.data(), len);
			large_buf[len] = '\0';
			large_pages.push_back(large_buf);
			return large_buf;
		}

		if (current_page_offset + needed > ARENA_PAGE_SIZE) {
			char *new_page = new char[ARENA_PAGE_SIZE];
			pages.push_back(new_page);
			current_page_index = pages.size() - 1;
			current_page_offset = 0;
		}

		char *dest = pages[current_page_index] + current_page_offset;
		memcpy(dest, str.data(), len);
		dest[len] = '\0';
		current_page_offset += needed;
		return dest;
	}

	// Allocate string and return std::string_view pointing directly into StringArena
	std::string_view allocateView(std::string_view str) {
		char *dest = allocate(str);
		return std::string_view(dest, str.length());
	}

	// Compatible with push_back returning StringArenaOffset (uint32_t)
	// 13 bits for page offset (0..8191), 18 bits for page index (0..262143), bit 31 for large block
	StringArenaOffset push_back(std::string_view str) {
		size_t len = str.length();
		size_t needed = len + 1;

		if (needed > ARENA_PAGE_SIZE) {
			char *large_buf = new char[needed];
			memcpy(large_buf, str.data(), len);
			large_buf[len] = '\0';
			uint32_t idx = static_cast<uint32_t>(large_pages.size());
			large_pages.push_back(large_buf);
			return (1u << 31) | idx;
		}

		if (current_page_offset + needed > ARENA_PAGE_SIZE) {
			char *new_page = new char[ARENA_PAGE_SIZE];
			pages.push_back(new_page);
			current_page_index = pages.size() - 1;
			current_page_offset = 0;
		}

		uint32_t page_idx = static_cast<uint32_t>(current_page_index);
		uint32_t page_off = static_cast<uint32_t>(current_page_offset);

		char *dest = pages[page_idx] + page_off;
		memcpy(dest, str.data(), len);
		dest[len] = '\0';
		current_page_offset += needed;

		return (page_idx << 13) | (page_off & 0x1FFFu);
	}

	const char *get(StringArenaOffset offset) const {
		if (offset & (1u << 31)) {
			uint32_t idx = offset & ~(1u << 31);
			if (idx < large_pages.size()) {
				return large_pages[idx];
			}
			return "";
		}
		uint32_t page_idx = offset >> 13;
		uint32_t page_off = offset & 0x1FFFu;
		if (page_idx < pages.size()) {
			return pages[page_idx] + page_off;
		}
		return "";
	}
};

#endif