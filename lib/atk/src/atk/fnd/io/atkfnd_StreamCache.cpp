#include <nn/atk/fnd/io/atkfnd_StreamCache.h>

#include <cstring>

#include <nn/util/util_BytePtr.h>

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

// NON_MATCHING
size_t StreamCache::Read(void* buf, size_t length, FndResult* result, FsAccessLog* log,
                         void* pFileStream) {
    if (!IsInitialized()) {
        if (log != nullptr)
            log->Impl0(pFileStream);

        size_t readSize{m_Stream->Read(buf, length, result)};
        if (log != nullptr)
            log->Impl1(pFileStream);

        return readSize;
    }

    FndResult flushResult{FlushWriteCache()};
    if (flushResult.IsFailed()) {
        if (result != nullptr)
            *result = flushResult;

        return 0;
    }

    size_t cacheHitLength{GetReadCacheHitLength(length)};
    if (cacheHitLength == 0 || length == 0)
        return 0;

    memcpy(buf, util::BytePtr(m_CacheBuffer, m_CurrentPosition - m_CachePosition).Get(),
           cacheHitLength);

    if (length <= cacheHitLength)
        return length;

    size_t restLength{length - cacheHitLength};

    FndResult syncResult{SyncStreamCurrentPosition(m_CurrentPosition)};
    if (syncResult.IsFailed()) {
        if (result != nullptr)
            *result = syncResult;

        return 0;
    }

    if (m_CacheBufferLength < restLength) {
        if (log != nullptr)
            log->Impl0(pFileStream);

        FndResult readResult;
        restLength = m_Stream->Read(buf, restLength, &readResult);

        if (log != nullptr)
            log->Impl1(pFileStream);

        if (result != nullptr)
            *result = readResult;

        if (readResult.IsFailed()) {
            ClearCache();
            m_CachePosition = m_CurrentPosition - m_CacheBufferLength;
            memcpy(m_CacheBuffer, util::BytePtr(buf, -m_CacheBufferLength).Get(),
                   m_CacheBufferLength);
            if (result != nullptr)
                *result = FndResult{FndResultType_True};

            return length;
        }
    }

    return cacheHitLength;
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

void StreamCache::ClearCache() {
    FlushWriteCache();
    m_CachePosition = InvalidPosition;
    m_CachedLength = 0;
}

// TODO: size_t StreamCache::Write
// TODO: size_t StreamCache::GetWritableCacheLength
// TODO: FndResult StreamCache::Seek

}  // namespace nn::atk::detail::fnd
