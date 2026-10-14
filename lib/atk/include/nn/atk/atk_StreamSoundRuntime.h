#pragma once

#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/atk_StreamSound.h>

namespace nn::atk::detail {

class StreamSoundRuntime {
public:
    StreamSoundRuntime();
    ~StreamSoundRuntime();

    bool Initialize(int32_t soundCount, void** pOutAllocatedAddr, const void* endAddr,
                    void* streamInstanceBuffer, size_t streamInstanceBufferSize);
    void Finalize();

    bool SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                           size_t strmBufferSize);
    bool SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                           size_t strmBufferSize, driver::StreamBufferPool* pStreamBufferPool);
    bool SetupStreamCacheBuffer(const SoundArchive* pSoundArchive, void* streamCacheBuffer,
                                size_t streamCacheSize);

    static size_t
    GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo,
                          int32_t alignment);

    size_t GetRequiredStreamBufferSize(const SoundArchive* soundArchive) const;
    static int32_t GetRequiredStreamBufferTimes(const SoundArchive* soundArchive);
    static size_t GetRequiredStreamCacheSize(const SoundArchive* soundArchive,
                                             size_t cacheSizePerSound);
    static size_t GetRequiredStreamInstanceSize(int32_t soundCount);

    void SetSoundArchiveFilesHook(SoundArchiveFilesHook* pSoundArchiveFilesHook) {
        m_pSoundArchiveFilesHook = pSoundArchiveFilesHook;
    }

    int32_t GetActiveCount() const;
    int32_t GetActiveChannelCount() const;
    int32_t GetActiveTrackCount() const;
    int32_t GetFreeCount() const;

    void SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize);

    void Update();

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    StreamSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority, int32_t ambientPriority,
                            BasicSound::AmbientInfo* ambientArgInfo);
#else
    StreamSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority, int32_t ambientPriority,
                            BasicSound::AmbientInfo* ambientArgInfo,
                            OutputReceiver* pOutputReceiver);
#endif

    SoundStartable::StartResult PrepareImpl(const SoundArchive* pSoundArchive,
                                            const SoundDataManager* pSoundDataManager,
                                            SoundArchive::ItemId soundId, StreamSound* sound,
                                            const SoundArchive::SoundInfo* commonInfo,
                                            const StartInfoReader& startInfoReader);

    void DumpMemory(const SoundArchive* pSoundArchive) const;

    bool SetStreamBlockCount(int32_t count) {
        m_StreamBlockCount = count;
        return true;
    }

    int32_t GetStreamBlockCount() { return m_StreamBlockCount; }

private:
    static const int32_t DefaultStreamBlockCount{5};

    StreamSoundInstanceManager m_StreamSoundInstanceManager;
    driver::StreamSoundLoaderManager m_StreamSoundLoaderManager;
    driver::StreamBufferPool m_StreamBufferPool;
    SoundArchiveFilesHook* m_pSoundArchiveFilesHook{};
    int32_t m_StreamBlockCount{DefaultStreamBlockCount};
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(StreamSoundRuntime) == 0xb0);
#else
static_assert(sizeof(StreamSoundRuntime) == 0xb8);
#endif

}  // namespace nn::atk::detail
