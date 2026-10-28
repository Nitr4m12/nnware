#pragma once

#include <nn/atk/atk_AddonSoundArchive.h>
#include <nn/atk/atk_SequenceSoundRuntime.h>
#include <nn/atk/atk_StreamSoundRuntime.h>
#include <nn/atk/atk_WaveSoundRuntime.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundRuntime.h>

namespace nn::atk {

class SoundArchivePlayer : public SoundStartable {
public:
    struct InitializeParam {
        const SoundArchive* pSoundArchive{};
        const SoundDataManager* pSoundDataManager{};
        void* pSetupBuffer{};
        size_t setupBufferSize{0};
        void* pStreamBuffer{};
        size_t streamBufferSize{0};
        void* pStreamCacheBuffer{};
        size_t streamCacheSize{0};
        bool enablePreparingStreamInstanceBufferFromSetupBuffer{true};
        void* pStreamInstanceBuffer{};
        size_t streamInstanceBufferSize{0};
        size_t userParamSizePerSound{0};
        int32_t addonSoundArchiveCount{0};

        InitializeParam() = default;
    };
    static_assert(sizeof(InitializeParam) == 0x68);

    struct StreamSoundInstanceState {
        int32_t activeStreamSoundInstanceCount;
        int32_t activeStreamChannelCount;
        int32_t activeStreamTrackCount;
    };
    static_assert(sizeof(StreamSoundInstanceState) == 0xc);

    static const uint32_t BufferAlignSize{4096};
    static const uint32_t StreamBufferTimesMax{4};
    static const uint32_t UserParamBoundary{4};

    static const int32_t AddonSoundArchiveNameLengthMax{64};

    SoundArchivePlayer();
    ~SoundArchivePlayer() override;

    static size_t GetRequiredMemSize(const SoundArchive* arc);
    static size_t GetRequiredMemSize(const SoundArchive* arc, size_t userParamSizePerSound);
    static size_t GetRequiredMemSize(const SoundArchive* arc, size_t userParamSizePerSound,
                                     int32_t addonSoundArchiveCount);
    static size_t GetRequiredMemSize(const InitializeParam& param);

    static size_t GetRequiredStreamInstanceSize(const SoundArchive* pArchive);

    size_t GetRequiredStreamBufferSize(const SoundArchive* arc) const;
    size_t GetRequiredStreamBufferTimes(const SoundArchive* arc) const;

    bool Initialize(const SoundArchive* arc, const SoundDataManager* manager, void* buffer,
                    size_t size, void* strmBuffer, size_t strmBufferSize,
                    size_t userParamSizePerSound);

    static size_t GetRequiredStreamCacheSize(const SoundArchive* arc, size_t cacheSizePerSound);

    bool Initialize(const InitializeParam& param);

    void Finalize();

    bool IsAvailable() const;

    void Update();

    const SoundArchive& GetSoundArchive() const;

    detail::SoundArchiveManager& GetSoundArchiveManager() { return m_SoundArchiveManager; }

    const AddonSoundArchive* GetAddonSoundArchive(const char* soundArchiveName) const;
    os::Tick GetAddonSoundArchiveLastAddTick() const { return m_AddonSoundArchiveLastAddTick; }

    int32_t GetAddonSoundArchiveCount() const;

    const AddonSoundArchive* GetAddonSoundArchive(int32_t index) const;
    const char* GetAddonSoundArchiveName(int32_t index) const;
    os::Tick GetAddonSoundArchiveAddTick(int32_t index) const;

    const SoundDataManager* GetAddonSoundDataManager(const char* soundArchiveName) const;

    SoundPlayer& GetSoundPlayer(SoundArchive::ItemId playerId);
    SoundPlayer& GetSoundPlayer(const char* pStr);

    const SoundPlayer& GetSoundPlayer(SoundArchive::ItemId playerId) const;
    const SoundPlayer& GetSoundPlayer(const char* pStr) const;

    size_t GetSoundUserParamSize() const { return m_SoundUserParamSize; }
    uint32_t GetSoundPlayerCount() const { return m_SoundPlayerCount; }

    int32_t GetFreeSequenceSoundCount() const { return m_SequenceSoundRuntime.GetFreeCount(); }
    int32_t GetFreeWaveSoundCount() const { return m_WaveSoundRuntime.GetFreeWaveSoundCount(); }
    int32_t GetFreeStreamSoundCount() const { return m_StreamSoundRuntime.GetFreeCount(); }

    int32_t GetActiveWaveSoundCount() const { return m_WaveSoundRuntime.GetActiveCount(); }
    int32_t GetActiveSequenceSoundCount() const { return m_SequenceSoundRuntime.GetActiveCount(); }
    int32_t GetActiveStreamSoundCount() const { return m_StreamSoundRuntime.GetActiveCount(); }

    void SetSequenceUserProcCallback(SequenceUserProcCallback callback, void* arg);

    static void SetSequenceSkipIntervalTick(int32_t intervalTick);
    static int32_t GetSequenceSkipIntervalTick();

    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo,
                                 SoundArchive::ItemId waveSoundId) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo, const char* waveSoundName) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo, SoundArchive::ItemId waveSoundId,
                                 const SoundArchive* pSoundArchive,
                                 const SoundDataManager* pSoundDataManager) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo, SoundArchive::ItemId waveSoundId,
                                 const char* soundArchiveName) const;
    Result ReadWaveSoundDataInfo(WaveSoundDataInfo* pOutInfo, const char* waveSoundName,
                                 const char* soundArchiveName) const;

    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                   SoundArchive::ItemId streamSoundId) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                   const char* streamSoundName) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo, const SoundArchive* pSoundArchive,
                                   SoundArchive::ItemId streamSoundId) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo,
                                   SoundArchive::ItemId streamSoundId,
                                   const char* soundArchiveName) const;
    Result ReadStreamSoundDataInfo(StreamSoundDataInfo* pOutInfo, const char* streamSoundName,
                                   const char* soundArchiveName) const;

    static size_t GetRequiredWorkBufferSizeToReadStreamSoundHeader();

    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         SoundArchive::ItemId streamSoundId, const char* regionName,
                                         void* buffer, size_t bufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         const char* streamSoundName, const char* regionName,
                                         void* buffer, size_t bufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         SoundArchive::ItemId streamSoundId,
                                         const char* const* pRegionName, int32_t infoCount,
                                         void* buffer, size_t bufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         const char* streamSoundName,
                                         const char* const* pRegionName, int32_t infoCount,
                                         void* buffer, size_t bufferSize) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         const char* streamSoundName, const char* regionName,
                                         void* buffer, size_t bufferSize,
                                         const char* soundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         SoundArchive::ItemId streamSoundId, const char* regionName,
                                         void* buffer, size_t bufferSize,
                                         const char* soundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         const char* streamSoundName,
                                         const char* const* pRegionName, int32_t infoCount,
                                         void* buffer, size_t bufferSize,
                                         const char* soundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         SoundArchive::ItemId streamSoundId,
                                         const char* const* pRegionName, int32_t infoCount,
                                         void* buffer, size_t bufferSize,
                                         const char* soundArchiveName) const;
    Result ReadStreamSoundRegionDataInfo(StreamSoundRegionDataInfo* pOutInfo,
                                         SoundArchive::ItemId streamSoundId,
                                         const char* const* pRegionName, int32_t infoCount,
                                         const SoundArchive* pSoundArchive, void* buffer,
                                         size_t bufferSize) const;

    void DumpMemory() const;

    bool ReadStreamSoundInstanceState(StreamSoundInstanceState* pOutInfo) const;

    Result CheckStreamSoundFileExisting(SoundArchive::ItemId streamSoundId) const;
    Result CheckStreamSoundFileExisting(const char* streamSoundName) const;

    Result CheckStreamSoundFileExisting(SoundArchive::ItemId streamSoundId,
                                        const char* soundArchiveName) const;
    Result CheckStreamSoundFileExisting(const char* streamSoundName,
                                        const char* soundArchiveName) const;

    StartResult detail_SetupSoundImpl(SoundHandle* handle, uint32_t soundId,
                                      detail::BasicSound::AmbientInfo* ambientArgInfo,
                                      SoundActor* actor, bool holdFlag,
                                      const char* soundArchiveName, const StartInfo* startInfo);

    SoundArchive::ItemId detail_GetItemId(const char* pString) override;
    SoundArchive::ItemId detail_GetItemId(const char* pString,
                                          const char* soundArchiveName) override;

    bool detail_IsEnableWarningPrint() const { return m_IsEnableWarningPrint; }

    void detail_SetMmlParser(detail::driver::MmlParser* parser) {
        m_SequenceSoundRuntime.SetMmlParser(parser);
    }

    const void* detail_GetFileAddress(SoundArchive::FileId fileId) const;

    detail::SoundArchiveFilesHook* detail_GetSoundArchiveFilesHook() const {
        return m_pSoundArchiveFilesHook;
    }
    void detail_SetSoundArchiveFilesHook(detail::SoundArchiveFilesHook* pHook) {
        m_pSoundArchiveFilesHook = pHook;
    }

    bool detail_SetupStreamBufferPool(void* buffer, size_t bufferSize,
                                      detail::driver::StreamBufferPool* pool);

    void AddAddonSoundArchive(const char* name, const AddonSoundArchive* pSoundArchive,
                              const SoundDataManager* pSoundDataManager);
    void RemoveAddonSoundArchive(const AddonSoundArchive* pSoundArchive);

    bool SetStreamBlockCount(int32_t count) {
        return m_StreamSoundRuntime.SetStreamBlockCount(count);
    }
    int32_t GetStreamBlockCount() { return m_StreamSoundRuntime.GetStreamBlockCount(); }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetDefaultOutputReceiver(OutputReceiver* pOutputReceiver);
#endif

protected:
    StartResult detail_SetupSound(SoundHandle* handle, uint32_t soundId, bool holdFlag,
                                  const char* soundArchiveName,
                                  const StartInfo* startInfo) override;

private:
    void EnableHook(const SoundArchive* pSoundArchive, bool isHookDisabled);

    bool IsSoundArchiveFileHooksEnabled() const;
    void LockSoundArchiveFileHooks();
    void UnlockSoundArchiveFileHooks();

    bool IsSequenceSoundEdited(const char* soundName) const;
    bool IsStreamSoundEdited(const char* soundName) const;
    bool IsWaveSoundEdited(const char* soundName) const;

    detail::PlayerHeap* CreatePlayerHeap(void** pOutAllocatedAddr, const void* endAddr,
                                         size_t heapSize);

    bool SetupMram(const SoundArchive* pArc, void* buffer, size_t size,
                   size_t userParamSizePerSound, int32_t addonSoundArchiveCount,
                   void* streamSoundInstanceBuffer, size_t streamSoundInstanceBufferSize);

    bool SetupSoundPlayer(const SoundArchive* pArc, void** pOutAllocatedAddr, const void* endAddr);

    bool SetupAddonSoundArchiveContainer(int32_t addonSoundArchiveCount, void** pOutAllocatedAddr,
                                         const void* endAddr);

    bool SetupUserParamForBasicSound(const SoundArchive::SoundArchivePlayerInfo& info,
                                     void** pOutAllocatedAddr, const void* endAddr,
                                     size_t userParamSizePerSound);

    void SetCommonSoundParam(detail::BasicSound* sound, const SoundArchive::SoundInfo* commonInfo);

    bool IsLoadWaveArchive() const;

    StartResult PreprocessSinglePlay(const SoundArchive::SoundInfo& info, uint32_t soundId,
                                     SoundPlayer& player);

    void StopAllSound(int32_t fadeFrames, bool isCommandFlushed);

    void DisposeInstances();

    Result CheckStreamSoundFileExisting(const SoundArchive* pSoundArchive,
                                        SoundArchive::ItemId streamSoundId) const;

    detail::SoundArchiveManager m_SoundArchiveManager;
    uint32_t m_SoundPlayerCount{0};
    SoundPlayer* m_pSoundPlayers{};
    detail::SequenceSoundRuntime m_SequenceSoundRuntime;
    detail::WaveSoundRuntime m_WaveSoundRuntime;
    detail::AdvancedWaveSoundRuntime m_AdvancedWaveSoundRuntime;
    detail::StreamSoundRuntime m_StreamSoundRuntime;
    size_t m_SoundUserParamSize{0};
    int32_t m_ArchiveContainerCount{0};
    detail::AddonSoundArchiveContainer* m_pArchiveContainers{};
    os::Tick m_AddonSoundArchiveLastAddTick{0};
    audio::MemoryPoolType m_MemoryPoolForStreamInstance;
    bool m_IsMemoryPoolForStreamInstanceAttached{false};
    audio::MemoryPoolType m_MemoryPoolForPlayerHeap;
    bool m_IsMemoryPoolForPlayerHeapAttached{false};
    detail::SoundArchiveFilesHook* m_pSoundArchiveFilesHook{};
    bool m_IsEnableWarningPrint{false};
    bool m_IsInitialized{false};
    bool m_IsAdvancedWaveSoundEnabled{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* m_pDefaultOutputReceiver{};
    uint8_t m_Padding[1];
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(SoundArchivePlayer) == 0x2e0);
#else
static_assert(sizeof(SoundArchivePlayer) == 0x310);
#endif

}  // namespace nn::atk
