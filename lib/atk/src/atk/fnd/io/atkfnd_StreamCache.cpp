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
    m_CachePosition = InvalidPosition;
    m_CachedLength = 0;
    m_CacheState = CacheState_None;
}

void StreamCache::Finalize() {
    m_Stream = nullptr;
    m_CurrentPosition = 0;
    m_CacheBuffer = nullptr;
    m_CacheBufferLength = 0;
    m_CachePosition = InvalidPosition;
    m_CachedLength = 0;
    m_CacheState = CacheState_None;
}

FndResult StreamCache::FlushWriteCache() {
    if (m_CachedLength == 0 || m_CacheState != CacheState_ForWrite)
        return FndResult{FndResultType_False};

    FndResult result{SyncStreamCurrentPosition(m_CachePosition)};
    if (result.IsFailed())
        return result;

    result = FndResult{FndResultType_True};
    m_Stream->Write(m_CacheBuffer, m_CachedLength, &result);
    m_CacheState = CacheState_None;
    m_CachePosition = InvalidPosition;
    m_CachedLength = 0;
    return result;
}

size_t StreamCache::GetReadCacheHitLength(size_t length) const {
    if (!IsInitialized())
        return 0;

    if (m_CacheState != CacheState_ForRead)
        return 0;

    if (m_CachePosition == InvalidPosition)
        return 0;

    if (m_CachePosition > m_CurrentPosition)
        return 0;

    size_t offset{static_cast<size_t>(m_CurrentPosition - m_CachePosition)};
    if (m_CachedLength >= offset + length)
        return length;

    length = m_CachedLength - offset;
    return m_CachedLength > offset ? length : 0;
}

FndResult StreamCache::SyncStreamCurrentPosition(position_t position) {
    if (m_Stream->GetCurrentPosition() == position)
        return FndResult{FndResultType_False};

    if (!m_Stream->CanSeek())
        return FndResult{FndResultType_Failed};

    return m_Stream->Seek(position, Stream::SeekOrigin_Begin);
}

}  // namespace nn::atk::detail::fnd
