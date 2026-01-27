#ifndef BITSET_HPP
#define BITSET_HPP

#include <iostream>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdint>

class InheritanceBitset {
  private:
	uint32_t size;
	uint64_t *bits;

  public:
	explicit InheritanceBitset(uint32_t maxSize = 0)
	    : size((maxSize + 63) / 64),
	      bits(new uint64_t[size]{}) {}
	InheritanceBitset(const InheritanceBitset&) = delete;
	InheritanceBitset& operator=(const InheritanceBitset&) = delete;
	InheritanceBitset(InheritanceBitset&& other) noexcept
		: size(other.size), bits(other.bits) {
		other.bits = nullptr;
	}
	void resize(uint32_t maxSize) { // At compiler time
		uint32_t newSize = std::max((maxSize + 63) / 64, size);
		uint64_t* newBits = new uint64_t[newSize]{};
		std::memcpy(newBits, bits, sizeof(uint64_t) * size);
		delete[] bits;
		bits = newBits;
		size = newSize;
	}
	void from(InheritanceBitset& inheritance, uint32_t maxSize) { // At compiler time
		delete[] bits;
		size = std::max((maxSize + 63) / 64, inheritance.size);
		bits = new uint64_t[size]{};
		std::memcpy(bits, inheritance.bits, sizeof(uint64_t) * inheritance.size);
	}
	inline uint32_t getSize() { return size; }
	void set(uint32_t index) {
		uint32_t bitPosition = index >> 6;
		assert(bitPosition < size);
		bits[bitPosition] |= static_cast<uint64_t>(1) << (index & 63);
	}
	bool get(uint32_t index) {
		uint32_t bitPosition = index >> 6;
		if (bitPosition >= size)
			return false;
		return (bits[bitPosition]) & (static_cast<uint64_t>(1) << (index & 63));
	}
	~InheritanceBitset() { if (bits) delete[] bits; }
};

#endif