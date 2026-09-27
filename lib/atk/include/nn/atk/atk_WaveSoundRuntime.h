#pragma once

#include <nn/atk/atk_StartInfoReader.h>
#include <nn/atk/atk_WaveSound.h>

namespace nn::atk::detail {

class WaveSoundRuntime {
public:
    WaveSoundRuntime();
    ~WaveSoundRuntime();

    bool Initialize(int32_t soundCount, void** pOutAllocatedAddr, const void* endAddr);
    void Finalize();

    static size_t
    GetRequiredMemorySize(const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo,
                          int32_t alignment);

    int32_t GetActiveCount() const;
    int32_t GetFreeWaveSoundCount() const;

    void SetupUserParam(void** startAddr, size_t adjustSize);

    void Update();

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    WaveSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority, int32_t ambientPriority,
                          BasicSound::AmbientInfo* ambientArgInfo);
#else
    WaveSound* AllocSound(SoundArchive::ItemId soundId, int32_t priority, int32_t ambientPriority,
                          BasicSound::AmbientInfo* ambientArgInfo, OutputReceiver* pOutputReceiver);
#endif

    SoundStartable::StartResult PrepareImpl(const SoundArchive* pSoundArchive,
                                            const SoundDataManager* pSoundDataManager,
                                            SoundArchive::ItemId soundId, WaveSound* sound,
                                            const SoundArchive::SoundInfo* commonInfo,
                                            const StartInfoReader& startInfoReader);

    void DumpMemory(const SoundArchive*) const;

private:
    WaveSoundInstanceManager m_WaveSoundInstanceManager;
    driver::WaveSoundLoaderManager m_WaveSoundLoaderManager;
    SoundArchiveFilesHook* m_pSoundArchiveFilesHook;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(WaveSoundRuntime) == 0x80);
#else
static_assert(sizeof(WaveSoundRuntime) == 0x88);
#endif

}  // namespace nn::atk::detail
