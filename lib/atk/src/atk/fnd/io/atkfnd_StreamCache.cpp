#include <nn/atk/fnd/io/atkfnd_StreamCache.h>

namespace nn::atk::detail::fnd {

StreamCache::StreamCache() = default;

StreamCache::StreamCache(Stream* sourceStream, void* buffer, size_t length)
    : m_Stream{sourceStream}, m_CacheBuffer{buffer}, m_CacheBufferLength{length} {}

void StreamCache::Initialize(Stream* sourceStream, void* buffer, size_t length) {
    m_Stream = sourceStream;
    m_CurrentPosition = 0;
    m_CacheBuffer = buffer;
    m_CacheBufferLength = length;
    m_CacheState = CacheState_None;
    m_CachePosition = InvalidPosition;
    m_CachedLength = 0;
}

}  // namespace nn::atk::detail::fnd
