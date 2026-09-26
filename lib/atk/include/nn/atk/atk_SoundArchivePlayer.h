#pragma once

#include <nn/audio/audio_MemoryPoolTypes.h>

#include <nn/atk/atk_SequenceSoundRuntime.h>
#include <nn/atk/atk_StreamSoundRuntime.h>
#include <nn/atk/atk_WaveSoundRuntime.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundRuntime.h>

namespace nn::atk {

class SoundArchivePlayer : SoundStartable {
public:
    constexpr static uint32_t BufferAlignSize = 4096;
    constexpr static uint32_t StreamBufferTimesMax = 4;
    constexpr static uint8_t UserParamBoundary = 4;

    struct InitializeParam {
        SoundArchive* pSoundArchive;
        SoundDataManager* pSoundDataManager;
        void* pSetupBuffer;
        size_t setupBufferSize;
        void* pStreamBuffer;
        size_t streamBufferSize;
        void* pStreamCacheBuffer;
        size_t streamCacheSize;
        bool enablePreparingStreamInstanceBufferFromSetupBuffer;
        void* pStreamInstanceBuffer;
        size_t streamInstanceBufferSize;
        size_t userParamSizePerSound;
        int32_t addonSoundArchiveCount;
    };
    static_assert(sizeof(InitializeParam) == 0x68);

    struct StreamSoundInstanceState {
        int32_t activeStreamSoundInstanceCount;
        int32_t activeStreamChannelCount;
        int32_t activeStreamTrackCount;
    };
    static_assert(sizeof(StreamSoundInstanceState) == 0xc);

    SoundArchivePlayer();
    ~SoundArchivePlayer() override;

    bool IsAvailable() const;

    bool Initialize(const SoundArchive* arc, const SoundDataManager* manager, void* buffer,
                    size_t size, void* strmBuffer, size_t strmBufferSize,
                    size_t userParamSizePerSound);
    bool Initialize(const InitializeParam& param);

    void Finalize();

    void StopAllSound(int32_t, bool);

    void DisposeInstances();

    static size_t GetRequiredMemSize(const SoundArchive* arc);
    static size_t GetRequiredMemSize(const SoundArchive* arc, size_t userParamSizePerSound,
                                     int32_t addonSoundArchiveCount);
    static size_t GetRequiredMemSize(const SoundArchive* arc, size_t userParamSizePerSound);
    static size_t GetRequiredMemSize(const InitializeParam& param);

    static size_t GetRequiredStreamInstanceSize(const SoundArchive* arc);

    size_t GetRequiredStreamBufferSize(const SoundArchive* arc) const;
    size_t GetRequiredStreamBufferTimes(const SoundArchive* arc) const;

    static size_t GetRequiredStreamCacheSize(const SoundArchive* arc, size_t);

    bool SetupMram(const SoundArchive* pArc, void* buffer, size_t size,
                   size_t userParamSizePerSound, int32_t addonSoundArchiveCount,
                   void* streamSoundInstanceBuffer, size_t streamSoundInstanceBufferSize);

    bool SetupSoundPlayer(const SoundArchive* pArc, void** pOutAllocatedAddr, const void* endAddr);
    bool SetupAddonSoundArchiveContainer(int32_t containerCount, void** pOutAllocatedAddr,
                                         const void* endAddr);
    bool SetupUserParamForBasicSound(const SoundArchive::SoundArchivePlayerInfo& playerInfo,
                                     void** pOutAllocatedAddr, const void* endAddr, size_t);

    detail::PlayerHeap* CreatePlayerHeap(void** pOutAllocatedAddr, const void* endAddr, size_t);

    void Update();

    SoundPlayer* GetSoundPlayer(uint32_t);

    SoundArchive* GetSoundArchive() const;
    AddonSoundArchive* GetAddonSoundArchive(const char*) const;
    AddonSoundArchive* GetAddonSoundArchive(int32_t) const;
    char* GetAddonSoundArchiveName(int32_t) const;
    os::Tick* GetAddonSoundArchiveAddTick(int32_t) const;
    SoundDataManager* GetAddonSoundDataManager(const char*) const;

    SoundPlayer* GetSoundPlayer(uint32_t) const;
    SoundPlayer* GetSoundPlayer(const char*);
    SoundPlayer* GetSoundPlayer(const char*) const;

    void* detail_GetFileAddress(SoundArchive::FileId fileId) const;

    void AddAddonSoundArchive(const char*, const AddonSoundArchive*, const SoundDataManager*);
    void RemoveAddonSoundArchive(const AddonSoundArchive*);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetDefaultOutputReceiver(OutputReceiver* pOutputReceiver);
#endif

    StartResult detail_SetupSound(SoundHandle* handle, uint32_t soundId, bool holdFlag,
                                  const char* soundArchiveName,
                                  const StartInfo* startInfo) override;
    StartResult detail_SetupSoundImpl(SoundHandle* handle, uint32_t soundId,
                                      detail::BasicSound::AmbientInfo* ambientArgInfo,
                                      SoundActor* actor, bool holdFlag,
                                      const char* soundArchiveName, const StartInfo* startInfo);

    bool IsSoundArchiveFileHooksEnabled() const;

    void LockSoundArchiveFileHooks();

    bool IsSequenceSoundEdited(const char*) const;
    bool IsStreamSoundEdited(const char*) const;
    bool IsWaveSoundEdited(const char*) const;

    void EnableHook(const SoundArchive*, bool);

    StartResult PreprocessSinglePlay(const SoundArchive::SoundInfo& info, uint32_t soundId,
                                     SoundPlayer& player);

    void SetCommonSoundParam(detail::BasicSound* pSound, const SoundArchive::SoundInfo* info);

    void UnlockSoundArchiveFileHooks();

    void SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* arg);

    static void SetSequenceSkipIntervalTick(int32_t tick);
    static int32_t GetSequenceSkipIntervalTick();

    Result ReadWaveSoundDataInfo(detail::WaveSoundDataInfo*, uint32_t, const SoundArchive*,
                                 const SoundDataManager*) const;
    Result ReadWaveSoundDataInfo(detail::WaveSoundDataInfo*, uint32_t, const char*) const;
    Result ReadWaveSoundDataInfo(detail::WaveSoundDataInfo*, const char*, const char*) const;
    Result ReadWaveSoundDataInfo(detail::WaveSoundDataInfo*, uint32_t) const;
    Result ReadWaveSoundDataInfo(detail::WaveSoundDataInfo*, const char*) const;

    Result ReadStreamSoundDataInfo(StreamSoundDataInfo*, const SoundArchive*,
                                   uint32_t) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo*, uint32_t, const char*) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo*, const char*, const char*) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo*, uint32_t) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo*, const char*) const;

    static size_t GetRequiredWorkBufferSizeToReadStreamSoundHeader();

    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, uint32_t,
                                         const char* const*, int32_t, const SoundArchive*, void*,
                                         size_t) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, uint32_t, const char*,
                                         void*, size_t) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, uint32_t,
                                         const char* const*, int32_t, void*, size_t,
                                         const char*) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, const char*,
                                         const char*, void*, size_t) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, const char*,
                                         const char* const*, int32_t, void*, size_t,
                                         const char*) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, uint32_t,
                                         const char* const*, int32_t, void*, size_t) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, const char*,
                                         const char* const*, int32_t, void*, size_t) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, const char*,
                                         const char*, void*, size_t,
                                         const char* soundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo*, uint32_t, const char*,
                                         void*, size_t, const char* soundArchiveName) const;

    void DumpMemory() const;

    bool ReadStreamSoundInstanceState(StreamSoundInstanceState*) const;

    Result CheckStreamSoundFileExisting(uint32_t) const;
    Result CheckStreamSoundFileExisting(uint32_t, const char* soundArchiveName) const;
    Result CheckStreamSoundFileExisting(char* streamSoundName) const;
    Result CheckStreamSoundFileExisting(const char*, const char*) const;
    Result CheckStreamSoundFileExisting(const SoundArchive*, uint32_t) const;

    SoundArchive::ItemId detail_GetItemId(const char* pString) override;
    SoundArchive::ItemId detail_GetItemId(const char* pString,
                                          const char* soundArchiveName) override;

private:
    detail::SoundArchiveManager m_SoundArchiveManager;
    uint32_t m_SoundPlayerCount;
    SoundPlayer* m_pSoundPlayers;
    detail::SequenceSoundRuntime m_SequenceSoundRuntime;
    detail::WaveSoundRuntime m_WaveSoundRuntime;
    detail::AdvancedWaveSoundRuntime m_AdvancedWaveSoundRuntime;
    detail::StreamSoundRuntime m_StreamSoundRuntime;
    size_t m_SoundUserParamSize;
    int32_t m_ArchiveContainerCount;
    detail::AddonSoundArchiveContainer* m_pArchiveContainers;
    os::Tick m_AddonSoundArchiveLastAddTick;
    audio::MemoryPoolType m_MemoryPoolForStreamInstance;
    bool m_IsMemoryPoolForStreamInstanceAttached;
    audio::MemoryPoolType m_MemoryPoolForPlayerHeap;
    bool m_IsMemoryPoolForPlayerHeapAttached;
    detail::SoundArchiveFilesHook* m_pSoundArchiveFilesHook;
    bool m_IsEnableWarningPrint;
    bool m_IsInitialized;
    bool m_IsAdvancedWaveSoundEnabled;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* m_pDefaultOutputReceiver;
    uint8_t m_Padding[1];
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SoundArchivePlayer) == 0x2e0);
#else
static_assert(sizeof(SoundArchivePlayer) == 0x310);
#endif

}  // namespace nn::atk
