#pragma once

#include <nn/atk/fnd/io/atkfnd_Stream.h>

namespace nn::atk::detail::fnd {

struct FsAccessLog;

class StreamCache {
    NN_NO_COPY(StreamCache);

public:
    static const position_t InvalidPosition{0xffffffff};

    enum CacheState {
        CacheState_None,
        CacheState_ForRead,
        CacheState_ForWrite,
    };

    StreamCache();
    StreamCache(Stream* sourceStream, void* buffer, size_t length);

    virtual ~StreamCache() = default;

    void Initialize(Stream* sourceStream, void* buffer, size_t length);
    void Finalize();

    bool IsInitialized() const {
        return m_Stream != nullptr && m_CacheBuffer != nullptr && m_CacheBufferLength != 0;
    }

    bool CanRead() const { return m_Stream->CanRead(); }
    bool CanWrite() const { return m_Stream->CanWrite(); }
    bool CanSeek() const { return m_Stream->CanSeek(); }

    size_t Read(void* buf, size_t length, FndResult* result, FsAccessLog* log, void* pFileStream);
    size_t Write(const void* buf, size_t length, FndResult* result);
    FndResult Seek(position_t offset, Stream::SeekOrigin origin);

    size_t GetCurrentPosition() const { return m_CurrentPosition; }

    void ClearCache();

    position_t GetCachePosition();
    size_t GetCachedLength();

private:
    size_t GetReadCacheHitLength(size_t length) const;
    size_t GetWritableCacheLength(size_t length) const;

    FndResult SyncStreamCurrentPosition(position_t position);

    void FlushWriteCache();

    Stream* m_Stream{};
    position_t m_CurrentPosition{0};
    void* m_CacheBuffer{};
    size_t m_CacheBufferLength{0};
    position_t m_CachePosition{InvalidPosition};
    size_t m_CachedLength{0};
    uint8_t m_CacheState{CacheState_None};
    uint8_t m_Padding[3];
};
static_assert(sizeof(StreamCache) == 0x40);

}  // namespace nn::atk::detail::fnd
