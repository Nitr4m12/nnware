#include <nn/atk/fnd/basis/atkfnd_WorkBufferAllocator.h>

namespace nn::atk::detail::fnd {

WorkBufferAllocator::WorkBufferAllocator(void* buffer, size_t size)
    : m_Buffer{reinterpret_cast<uintptr_t>(buffer)}, m_Size{size} {}

}  // namespace nn::atk::detail::fnd
