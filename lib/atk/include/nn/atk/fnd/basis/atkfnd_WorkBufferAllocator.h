#pragma once

#include <cstddef>
#include <cstdint>

namespace nn::atk::detail::fnd {

class WorkBufferAllocator {
public:
    WorkBufferAllocator(void* buffer, size_t size);
    WorkBufferAllocator(void* buffer, size_t size, size_t requiredAlignment);

    void* Allocate(size_t size);
    void* Allocate(size_t size, size_t alignment);
    void* Allocate(size_t size, size_t alignment, int32_t count);

    size_t GetSize() const { return m_Size; }
    size_t GetAllocSize() const;
    size_t GetFreeSize() const;

private:
    uintptr_t m_Buffer;
    size_t m_Offset{0};
    size_t m_Size;
};
static_assert(sizeof(WorkBufferAllocator) == 0x18);

}  // namespace nn::atk::detail::fnd
