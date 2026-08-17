#ifndef SMALL_VECTOR_HPP
#define SMALL_VECTOR_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <vector>

namespace Autolang {

template <typename T, uint32_t N>
class SmallVector {
	alignas(T) unsigned char inlineBuffer_[sizeof(T) * N];
	T *data_;
	uint32_t size_;
	uint32_t capacity_;

	T *inlinePtr() { return reinterpret_cast<T *>(inlineBuffer_); }
	const T *inlinePtr() const {
		return reinterpret_cast<const T *>(inlineBuffer_);
	}
	bool isInline() const { return data_ == inlinePtr(); }

	void destroyRange(T *first, T *last) {
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (; first != last; ++first) {
				first->~T();
			}
		}
	}

	void freeHeap() {
		if (!isInline()) {
			::operator delete(data_);
		}
	}

	T *allocateHeap(uint32_t cap) {
		return static_cast<T *>(::operator new(sizeof(T) * cap));
	}

	void moveElements(T *dst, T *src, uint32_t count) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(dst, src, sizeof(T) * count);
		} else {
			for (uint32_t i = 0; i < count; ++i) {
				new (&dst[i]) T(std::move(src[i]));
				src[i].~T();
			}
		}
	}

	void copyElements(T *dst, const T *src, uint32_t count) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(dst, src, sizeof(T) * count);
		} else {
			for (uint32_t i = 0; i < count; ++i) {
				new (&dst[i]) T(src[i]);
			}
		}
	}

	void grow(uint32_t minCap) {
		uint32_t newCap = capacity_ * 2;
		if (newCap < minCap)
			newCap = minCap;
		T *newData = allocateHeap(newCap);
		moveElements(newData, data_, size_);
		freeHeap();
		data_ = newData;
		capacity_ = newCap;
	}

	void shiftRight(uint32_t idx, uint32_t count) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memmove(data_ + idx + count, data_ + idx,
			             sizeof(T) * (size_ - idx));
		} else {
			// Construct new elements at the end
			for (uint32_t i = size_ + count - 1; i >= idx + count; --i) {
				new (&data_[i]) T(std::move(data_[i - count]));
				data_[i - count].~T();
			}
		}
	}

	void shiftLeft(uint32_t idx, uint32_t count) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memmove(data_ + idx, data_ + idx + count,
			             sizeof(T) * (size_ - idx - count));
		} else {
			for (uint32_t i = idx; i < size_ - count; ++i) {
				new (&data_[i]) T(std::move(data_[i + count]));
				data_[i + count].~T();
			}
		}
	}

public:
	using value_type = T;
	using iterator = T *;
	using const_iterator = const T *;
	using reference = T &;
	using const_reference = const T &;
	using size_type = size_t;

	SmallVector() : data_(inlinePtr()), size_(0), capacity_(N) {}

	SmallVector(const SmallVector &other)
	    : data_(inlinePtr()), size_(0), capacity_(N) {
		reserve(other.size_);
		copyElements(data_, other.data_, other.size_);
		size_ = other.size_;
	}

	SmallVector(SmallVector &&other) noexcept
	    : data_(inlinePtr()), size_(0), capacity_(N) {
		if (other.isInline()) {
			moveElements(data_, other.data_, other.size_);
			size_ = other.size_;
			other.size_ = 0;
		} else {
			data_ = other.data_;
			size_ = other.size_;
			capacity_ = other.capacity_;
			other.data_ = other.inlinePtr();
			other.size_ = 0;
			other.capacity_ = N;
		}
	}

	SmallVector(std::initializer_list<T> init)
	    : data_(inlinePtr()), size_(0), capacity_(N) {
		reserve(static_cast<uint32_t>(init.size()));
		for (const auto &v : init) {
			new (&data_[size_++]) T(v);
		}
	}

	// Convert from std::vector (move)
	SmallVector(std::vector<T> &&v)
	    : data_(inlinePtr()), size_(0), capacity_(N) {
		uint32_t vsize = static_cast<uint32_t>(v.size());
		reserve(vsize);
		for (auto &elem : v) {
			new (&data_[size_++]) T(std::move(elem));
		}
		v.clear();
	}

	// Convert from std::vector (copy)
	SmallVector(const std::vector<T> &v)
	    : data_(inlinePtr()), size_(0), capacity_(N) {
		uint32_t vsize = static_cast<uint32_t>(v.size());
		reserve(vsize);
		for (const auto &elem : v) {
			new (&data_[size_++]) T(elem);
		}
	}

	SmallVector &operator=(const SmallVector &other) {
		if (this != &other) {
			destroyRange(data_, data_ + size_);
			if (other.size_ > capacity_) {
				freeHeap();
				data_ = allocateHeap(other.size_);
				capacity_ = other.size_;
			}
			size_ = 0;
			copyElements(data_, other.data_, other.size_);
			size_ = other.size_;
		}
		return *this;
	}

	SmallVector &operator=(SmallVector &&other) noexcept {
		if (this != &other) {
			destroyRange(data_, data_ + size_);
			freeHeap();
			if (other.isInline()) {
				data_ = inlinePtr();
				capacity_ = N;
				size_ = 0;
				moveElements(data_, other.data_, other.size_);
				size_ = other.size_;
				other.size_ = 0;
			} else {
				data_ = other.data_;
				size_ = other.size_;
				capacity_ = other.capacity_;
				other.data_ = other.inlinePtr();
				other.size_ = 0;
				other.capacity_ = N;
			}
		}
		return *this;
	}

	~SmallVector() {
		destroyRange(data_, data_ + size_);
		freeHeap();
	}

	// Capacity
	void reserve(uint32_t newCap) {
		if (newCap > capacity_) {
			grow(newCap);
		}
	}

	void reserve(size_t newCap) { reserve(static_cast<uint32_t>(newCap)); }

	void resize(uint32_t newSize) {
		if (newSize > size_) {
			reserve(newSize);
			if constexpr (std::is_trivially_constructible_v<T>) {
				std::memset(data_ + size_, 0,
				            sizeof(T) * (newSize - size_));
			} else {
				for (uint32_t i = size_; i < newSize; ++i) {
					new (&data_[i]) T();
				}
			}
		} else {
			destroyRange(data_ + newSize, data_ + size_);
		}
		size_ = newSize;
	}

	void resize(size_t newSize) { resize(static_cast<uint32_t>(newSize)); }

	// Modifiers
	void push_back(const T &value) {
		if (size_ == capacity_)
			grow(capacity_ + 1);
		new (&data_[size_++]) T(value);
	}

	void push_back(T &&value) {
		if (size_ == capacity_)
			grow(capacity_ + 1);
		new (&data_[size_++]) T(std::move(value));
	}

	template <typename... Args> T &emplace_back(Args &&...args) {
		if (size_ == capacity_)
			grow(capacity_ + 1);
		T *p = new (&data_[size_++]) T(std::forward<Args>(args)...);
		return *p;
	}

	void pop_back() {
		--size_;
		if constexpr (!std::is_trivially_destructible_v<T>) {
			data_[size_].~T();
		}
	}

	iterator insert(const_iterator pos, const T &value) {
		uint32_t idx = static_cast<uint32_t>(pos - data_);
		if (size_ == capacity_)
			grow(capacity_ + 1);
		shiftRight(idx, 1);
		new (&data_[idx]) T(value);
		++size_;
		return data_ + idx;
	}

	iterator insert(const_iterator pos, T &&value) {
		uint32_t idx = static_cast<uint32_t>(pos - data_);
		if (size_ == capacity_)
			grow(capacity_ + 1);
		shiftRight(idx, 1);
		new (&data_[idx]) T(std::move(value));
		++size_;
		return data_ + idx;
	}

	// Range insert (supports both SmallVector and std::vector iterators)
	template <typename InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last) {
		uint32_t idx = static_cast<uint32_t>(pos - data_);
		uint32_t count = static_cast<uint32_t>(last - first);
		if (count == 0)
			return data_ + idx;

		if (size_ + count > capacity_)
			grow(size_ + count);

		shiftRight(idx, count);
		for (uint32_t i = 0; i < count; ++i) {
			new (&data_[idx + i]) T(first[i]);
		}
		size_ += count;
		return data_ + idx;
	}

	iterator erase(const_iterator pos) {
		uint32_t idx = static_cast<uint32_t>(pos - data_);
		if constexpr (!std::is_trivially_destructible_v<T>) {
			data_[idx].~T();
		}
		shiftLeft(idx, 1);
		--size_;
		return data_ + idx;
	}

	iterator erase(const_iterator first, const_iterator last) {
		uint32_t idx = static_cast<uint32_t>(first - data_);
		uint32_t count = static_cast<uint32_t>(last - first);
		if (count == 0)
			return data_ + idx;
		destroyRange(data_ + idx, data_ + idx + count);
		shiftLeft(idx, count);
		size_ -= count;
		return data_ + idx;
	}

	void clear() {
		destroyRange(data_, data_ + size_);
		size_ = 0;
	}

	// Access
	T &operator[](size_t i) { return data_[i]; }
	const T &operator[](size_t i) const { return data_[i]; }

	T &back() { return data_[size_ - 1]; }
	const T &back() const { return data_[size_ - 1]; }

	T &front() { return data_[0]; }
	const T &front() const { return data_[0]; }

	T *data() { return data_; }
	const T *data() const { return data_; }

	// Iterators
	iterator begin() { return data_; }
	iterator end() { return data_ + size_; }
	const_iterator begin() const { return data_; }
	const_iterator end() const { return data_ + size_; }

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rbegin() const {
		return const_reverse_iterator(end());
	}
	const_reverse_iterator rend() const {
		return const_reverse_iterator(begin());
	}

	// Size
	uint32_t size() const { return size_; }
	bool empty() const { return size_ == 0; }
	uint32_t capacity() const { return capacity_; }
};

} // namespace Autolang

#endif
