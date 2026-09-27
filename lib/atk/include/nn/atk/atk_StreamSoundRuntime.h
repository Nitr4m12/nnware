#pragma once

#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/atk_StreamSound.h>

namespace nn::atk::detail {

class StreamSoundRuntime {
public:
    constexpr static uint32_t DefaultStreamBlockCount = 5;

    StreamSoundRuntime();
    ~StreamSoundRuntime();

    bool Initialize(int32_t soundCount, void** pOutAllocatedAddr, const void* endAddr,
                    void* streamInstanceBuffer, size_t streamInstanceBufferSize);

    static size_t GetRequiredStreamInstanceSize(int32_t soundCount);

    bool SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                           size_t strmBufferSize);
    bool SetupStreamBuffer(const SoundArchive* pSoundArchive, void* strmBuffer,
                           size_t strmBufferSize, driver::StreamBufferPool* strmBufferPool);

    size_t GetRequiredStreamBufferSize(const SoundArchive* soundArchive) const;
    static uint32_t GetRequiredStreamBufferTimes(const SoundArchive* soundArchive);

    bool SetupStreamCacheBuffer(const SoundArchive* pSoundArchive, void* streamCacheBuffer,
                                size_t streamCacheSize);

    void Finalize();

    static size_t
    GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo,
                          int32_t alignment);

    static size_t GetRequiredStreamCacheSize(const SoundArchive* pSoundArchive, size_t);

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

private:
    StreamSoundInstanceManager m_StreamSoundInstanceManager;
    driver::StreamSoundLoaderManager m_StreamSoundLoaderManager;
    driver::StreamBufferPool m_StreamBufferPool;
    SoundArchiveFilesHook* m_pSoundArchiveFilesHook;
    int32_t m_StreamBlockCount;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(StreamSoundRuntime) == 0xb0);
#else
static_assert(sizeof(StreamSoundRuntime) == 0xb8);
#endif

}  // namespace nn::atk::detail
