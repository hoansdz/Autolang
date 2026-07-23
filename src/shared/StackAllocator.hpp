#ifndef STACKALLOCATOR_HPP
#define STACKALLOCATOR_HPP

#include "ObjectManager.hpp"
#include "shared/AObject.hpp"
#include <cstring>
#include <vector>

namespace Autolang {

class AVM;

class StackAllocator {
  private:
    static constexpr size_t CHUNK_SHIFT = 8;
    static constexpr size_t CHUNK_SIZE = 1 << CHUNK_SHIFT; // 256
    static constexpr size_t CHUNK_MASK = CHUNK_SIZE - 1;   // 255 (0xFF)

    std::vector<AObject **> chunks; // Manages the list of fixed-size memory blocks
    size_t topIndex;                // The absolute global index representing the top of the stack

    // Helper: Allocates new chunks if the required chunk index exceeds current capacity
    inline void ensureChunks(size_t chunkCount) {
        while (chunks.size() < chunkCount) {
            chunks.push_back(new AObject *[CHUNK_SIZE] {});
        }
    }

  public:
    size_t maxSize; 
    int peak;       
    AObject **currentPtr; // Retained for backwards compatibility

    StackAllocator(size_t maxSize = 0)
        : maxSize(maxSize), topIndex(0), peak(0) {
        chunks.push_back(new AObject *[CHUNK_SIZE] {});
        currentPtr = chunks[0];
    }

    ~StackAllocator() {
        for (auto chunk : chunks) {
            delete[] chunk;
        }
    }

    // Helper: Gets the absolute memory pointer for a given global index.
    inline AObject **getAbsolute(size_t index) {
        size_t chunkIdx = index >> CHUNK_SHIFT;
        // FIX: Tự động đảm bảo chunk tồn tại trước khi truy cập, chặn đứng Vector Out-of-bounds
        ensureChunks(chunkIdx + 1); 
        return &chunks[chunkIdx][index & CHUNK_MASK];
    }

    inline void setTop(size_t top) {
        this->topIndex = top;
        size_t chunkIdx = topIndex >> CHUNK_SHIFT;
        size_t slotIdx = topIndex & CHUNK_MASK;

        ensureChunks(chunkIdx + 1);
        currentPtr = chunks[chunkIdx] + slotIdx;
    }

    inline size_t getTop() const { return topIndex; }

    inline void ensure(size_t size) {
        // FIX: Xóa bỏ logic "padding nhảy chunk" để giữ index luôn liền mạch y hệt mảng phẳng ban đầu.
        size_t requiredTop = topIndex + size;
        size_t chunkIdx = requiredTop >> CHUNK_SHIFT;
        ensureChunks(chunkIdx + 1);

        if (static_cast<int>(requiredTop) > peak) {
            peak = static_cast<int>(requiredTop);
        }
    }

    inline void clear(ObjectManager &manager, size_t from, size_t to) {
        for (size_t i = from; i <= to; ++i) {
            size_t chunkIdx = i >> CHUNK_SHIFT;
            
            // FIX: Bỏ qua an toàn nếu VM cố gắng clear vùng nhớ (to) vượt quá các chunk đã cấp phát
            if (chunkIdx >= chunks.size()) {
                break;
            }

            AObject **slot = &chunks[chunkIdx][i & CHUNK_MASK];
            if (*slot) {
                manager.release(*slot);
                *slot = nullptr;
            }
        }
    }

    inline void freeTo(size_t newTop) { 
        setTop(newTop); 
    }

    inline void restart() {
        if (!chunks.empty()) {
            std::memset(chunks[0], 0, CHUNK_SIZE * sizeof(AObject *));
        }

        for (size_t i = 1; i < chunks.size(); ++i) {
            delete[] chunks[i];
        }

        chunks.resize(1);
        setTop(0);
        peak = 0;
    }

    template <size_t size> inline void clearTemp(ObjectManager &manager) {
        if constexpr (size > 0) {
            constexpr size_t idx = size - 1;
            AObject **obj = getAbsolute(idx);

            if (*obj) {
                manager.release(*obj);
                *obj = nullptr;
            }
            clearTemp<size - 1>(manager);
        }
    }

    inline void set(ObjectManager &manager, size_t index, AObject *object) {
        // FIX: Tính toán địa chỉ tuyệt đối động thay vì dựa vào currentPtr để không bao giờ bị tràn 256
        AObject *&last = *getAbsolute(topIndex + index);
        if (last != nullptr) {
            manager.release(last);
        }
        last = object;
    }

    inline AObject *&operator[](size_t index) { 
        // FIX: Tương tự hàm set(), vượt qua ranh giới chunk một cách an toàn
        return *getAbsolute(topIndex + index); 
    }
};

} // namespace Autolang

#endif