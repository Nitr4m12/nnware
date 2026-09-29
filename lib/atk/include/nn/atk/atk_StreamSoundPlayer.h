#pragma once

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_StreamBufferPool.h>
#include <nn/atk/atk_StreamSoundLoader.h>
#include <nn/atk/atk_StreamSoundPrefetchFileReader.h>
#include <nn/atk/atk_StreamTrack.h>

namespace nn::atk {

struct StreamDataInfo {
    bool loopFlag;
    int32_t sampleRate;
    int64_t loopStart;
    int64_t loopEnd;
    int64_t compatibleLoopStart;
    int64_t compatibleLoopEnd;
    int32_t channelCount;

    void Dump();
};
static_assert(sizeof(StreamDataInfo) == 0x30);

struct StreamSoundDataInfo {
    bool loopFlag;
    int32_t sampleRate;
    int64_t loopStart;
    int64_t loopEnd;
    int64_t compatibleLoopStart;
    int64_t compatibleLoopEnd;
    int32_t channelCount;

    void Dump();
};
static_assert(sizeof(StreamSoundDataInfo) == 0x30);

struct StreamSoundRegionDataInfo {
    uint32_t startSamplePosition;
    uint32_t endSamplePosition;
    int32_t regionNo;
    char regionName[64];
};
static_assert(sizeof(StreamSoundRegionDataInfo) == 0x4c);

namespace detail::driver {

class StreamSoundPlayer : public BasicSoundPlayer, public SoundThread::PlayerCallback {
public:
    enum StartOffsetType {
        StartOffsetType_Sample,
        StartOffsetType_Millisec,
    };

    StreamSoundPlayer();
    ~StreamSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif

    void Finalize() override;

    void SetLoaderManager(driver::StreamSoundLoaderManager* manager) { m_pLoaderManager = manager; }

    struct SetupArg {
        StreamBufferPool* pBufferPool;
        uint32_t allocChannelCount;
        uint16_t allocTrackFlag;
        uint8_t fileType;
        bool loopFlag;
        TrackDataInfos trackInfos;
        position_t loopStart;
        position_t loopEnd;
        float pitch;
        uint8_t mainSend;
        uint8_t fxSend[AuxBus_Count];
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
        DecodeMode decodeMode;
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    static_assert(sizeof(SetupArg) == 0x98);
#else
    static_assert(sizeof(SetupArg) == 0xa0);
#endif

    void Setup(const SetupArg& arg);

    struct PrepareBaseArg {
        StartOffsetType startOffsetType{StartOffsetType_Sample};
        position_t offset{0};
        int delayTime{0};
        int delayCount{0};
        UpdateType updateType{UpdateType_AudioFrame};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        uint32_t subMixIndex;
#endif
        StreamRegionCallback regionCallback{};
        void* regionCallbackArg{};
        char filePath[FilePathMax]{};
        const void* pExternalData{};
        size_t externalDataSize{0};
        FileStreamHookParam fileStreamHookParam;

        PrepareBaseArg() = default;
    };
    static_assert(sizeof(PrepareBaseArg) == 0x2d0);

    struct PrepareArg {
        PrepareBaseArg baseArg;
        void* cacheBuffer{};
        size_t cacheSize{0};

        PrepareArg() = default;
    };
    static_assert(sizeof(PrepareArg) == 0x2e0);

    void Prepare(const PrepareArg& arg);

    struct PreparePrefetchArg {
        PrepareBaseArg baseArg;
        const void* strmPrefetchFile;

        PreparePrefetchArg() = default;
    };
    static_assert(sizeof(PreparePrefetchArg) == 0x2d8);

    void PreparePrefetch(const PreparePrefetchArg& arg);

    void Start() override;
    void Stop() override;
    void Pause(bool flag) override;

    bool IsFinalizing() const { return m_IsFinalizing; }
    bool IsSuspendByLoadingDelay() const { return m_IsStoppedByLoadingDelay; }
    bool IsLoadingDelayState() const;
    bool IsPrepared() const { return m_IsPrepared; }

    void SetTrackVolume(uint32_t trackBitFlag, float volume);
    void SetTrackInitialVolume(uint32_t trackBitFlag, uint32_t volume);

    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine);
    void ResetTrackOutputLine(uint32_t trackBitFlag);

    void SetTrackTvVolume(uint32_t trackBitFlag, float volume);
    void SetTrackChannelTvMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                       const MixParameter& param);
    void SetTrackTvPan(uint32_t trackBitFlag, float pan);
    void SetTrackTvSurroundPan(uint32_t trackBitFlag, float span);
    void SetTrackTvMainSend(uint32_t trackBitFlag, float send);
    void SetTrackTvFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

    void SetTrackDrcVolume(uint32_t, uint32_t, float);
    void SetTrackChannelDrcMixParameter(uint32_t, uint32_t, uint32_t, const MixParameter&);
    void SetTrackDrcPan(uint32_t, uint32_t, float);
    void SetTrackDrcSurroundPan(uint32_t, uint32_t, float);
    void SetTrackDrcMainSend(uint32_t, uint32_t, float);
    void SetTrackDrcFxSend(uint32_t, uint32_t, AuxBus, float);

    bool ReadStreamSoundDataInfo(StreamDataInfo* info) const;
    bool ReadStreamSoundDataInfo(StreamSoundDataInfo* info) const;

    int GetPlayLoopCount() const { return m_PlayingBlockLoopCounter; }
    position_t GetPlaySamplePosition(bool isOriginalSamplePosition) const;
    float GetFilledBufferPercentage() const;
    int GetBufferBlockCount(WaveBuffer::Status status) const;
    int GetTotalBufferBlockCount() const;

    int GetActiveChannelCount() const {
        if (!IsActive())
            return 0;

        return m_ChannelCount;
    }

    int GetActiveTrackCount() const {
        if (!IsActive())
            return 0;

        return m_TrackCount;
    }

    StreamTrack* GetPlayerTrack(int trackNo);
    const StreamTrack* GetPlayerTrack(int trackNo) const;

    bool LoadHeader(bool result, AdpcmParam** adpcmParam, uint16_t assignNumber);
    bool LoadStreamData(bool result, const LoadDataParam& loadDataParam, uint16_t assignNumber);
    bool LoadStreamData(bool result, const LoadDataParam& loadDataParam, uint16_t assignNumber,
                        bool usePrefetchFlag, uint32_t currentPrefetchBlockIndex,
                        size_t currentPrefetchBlockBytes);

    void ForceFinish() { SetFinishFlag(true); };

    os::Tick GetProcessTick(const SoundProfile& profile);

    void* detail_SetFsAccessLog(fnd::FsAccessLog* fsAccessLog);

protected:
    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

private:
    struct ItemData {
        float pitch;
        float mainSend;
        float fxSend[AuxBus_Count];

        void Set(const SetupArg& arg);
    };
    static_assert(sizeof(ItemData) == 0x14);

    struct TrackData {
        float volume;
        float lpfFreq;
        int biquadType;
        float biquadValue;
        float pan;
        float span;
        float mainSend;
        float fxSend[AuxBus_Count];

        void Set(const StreamTrack* track);
    };
    static_assert(sizeof(TrackData) == 0x28);

    struct PrefetchLoadDataParam : LoadDataParam {
        uint32_t prefetchBlockIndex;
        size_t prefetchBlockBytes;

        PrefetchLoadDataParam() = default;
    };
    // static_assert(sizeof(PrefetchLoadDataParam) == 0xa0);

    struct PrefetchIndexInfo {
        uint32_t lastBlockIndex;
        position_t loopStartInBlock;
        uint32_t loopStartBlockIndex;
        int loopBlockCount;

        void Initialize(const StreamDataInfoDetail& streamDataInfo);

        bool IsOverLastBlock(uint32_t blockIndex) { return blockIndex > lastBlockIndex; }

        uint32_t GetBlockOffsetFromLoopEnd(uint32_t blockIndex) {
            if (blockIndex - lastBlockIndex == 0)
                return 0;

            return (blockIndex - lastBlockIndex) - ((blockIndex - lastBlockIndex) / loopBlockCount) * loopBlockCount;
        }

        bool IsLoopStartBlock(uint32_t blockIndex) { return loopStartInBlock == blockIndex; }

        bool IsLastBlock(uint32_t blockIndex, uint32_t blockOffsetFromLoopEnd) {
            if (lastBlockIndex != 0)
                return false;

            if (blockIndex != lastBlockIndex)
                return false;

            if (blockOffsetFromLoopEnd == 0 || IsOverLastBlock(blockIndex))
                return false;

            return true;
        }
    };
    static_assert(sizeof(PrefetchIndexInfo) == 0x18);

    void StartPlayer();
    void FinishPlayer();
    bool SetupPlayer();

    void Update();
    void UpdateBuffer();
    void UpdateVoiceParams(StreamTrack* track);

    void SetOutputParam(const OutputParam* pOutOutputParam, const OutputParam& trackParam,
                        const TrackData& trackData);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void MixSettingForOutputParam(OutputParam* pOutOutputParam, int channelIndex, MixMode mixMode);
#else
    void MixSettingForOutputParam(OutputParam* pOutOutputParam,
                                  OutputBusMixVolume* pOutOutputBusMixVolume, int32_t channelIndex,
                                  MixMode mixMode);
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void ApplyTvOutputParamForMultiChannel(const OutputParam& outputParam, MultiVoice* MultiVoice,
                                           int32_t channelIndex, MixMode mixMode);
    void ApplyDrcOutputParamForMultiChannel(const OutputParam& outputParam, MultiVoice* MultiVoice,
                                            int32_t channelIndex, MixMode mixMode, uint32_t);
#else
    void ApplyTvOutputParamForMultiChannel(
        const OutputParam& outputParam, const OutputAdditionalParam* const pOutputAdditionalParam,
        MultiVoice* MultiVoice, int32_t channelIndex, MixMode mixMode);
    void ApplyDrcOutputParamForMultiChannel(const OutputParam& outputParam, MultiVoice* MultiVoice,
                                            int32_t channelIndex, MixMode mixMode, uint32_t);
#endif

    bool AllocVoices();
    void FreeVoices();

    bool TryAllocLoader();
    void FreeLoader();

    bool AllocStreamBuffers();
    void FreeStreamBuffers();

    void UpdateLoadingBlockIndex();
    void UpdatePauseStatus();

    bool CheckDiskDriveError() const;

    bool IsBufferEmpty() const;

    bool IsStoppedByLoadingDelay() const;

    bool SetupTrack(const SetupArg& arg);
    void SetPrepareBaseArg(const PrepareBaseArg& baseArg);

    void RequestLoadHeader(const PrepareArg& arg);
    bool ReadPrefetchFile(StreamSoundPrefetchFileReader& reader);

    bool ApplyStreamDataInfo(const StreamDataInfoDetail& streamDataInfo);

    position_t GetOriginalPlaySamplePosition(position_t playSamplePosition,
                                             const StreamDataInfoDetail& streamDataInfo) const;

    int GetOriginalLoopCount(position_t playSamplePosition,
                             const StreamDataInfoDetail& streamDataInfo) const;

    position_t GetStartOffsetSamples(const StreamDataInfoDetail& streamDataInfo);

    bool IsValidStartOffset(const StreamDataInfoDetail& streamDataInfo);

    void ApplyTrackDataInfo(const StreamDataInfoDetail& streamDataInfo);

    bool CheckPrefetchRevision(const StreamDataInfoDetail& streamDataInfo) const;

    bool LoadPrefetchBlocks(StreamSoundPrefetchFileReader& reader);

    void PreparePrefetchOnLastBlock(PrefetchLoadDataParam* param,
                                    const PrefetchIndexInfo& indexInfo);

    void PreparePrefetchOnLoopStartBlock(PrefetchLoadDataParam* param,
                                         const PrefetchIndexInfo& indexInfo,
                                         StreamSoundPrefetchFileReader& reader);

    void PreparePrefetchOnLoopBlock(PrefetchLoadDataParam* param,
                                    const PrefetchIndexInfo& indexInfo,
                                    uint32_t blockOffsetFromLoopEnd);

    bool PreparePrefetchOnNormalBlock(PrefetchLoadDataParam* param, uint32_t blockIndex,
                                      StreamSoundPrefetchFileReader* reader);

    bool SetAdpcmInfo(StreamSoundPrefetchFileReader& reader,
                      const StreamDataInfoDetail& streamDataInfo, AdpcmParam* adpcmParam,
                      AdpcmContextNotAligned* adpcmContext);

    bool SetAdpcmLoopInfo(StreamSoundPrefetchFileReader& reader,
                          const StreamDataInfoDetail& streamDataInfo, AdpcmParam* adpcmParam,
                          AdpcmContextNotAligned* adpcmContext);

    static void VoiceCallbackFunc(MultiVoice* voice, MultiVoice::VoiceCallbackStatus status,
                                  void* arg);

    struct WaveBufferInfo {
        position_t sampleBegin;
        size_t sampleLength;
        int loopCount;
    };
    static_assert(sizeof(WaveBufferInfo) == 0x18);

    bool m_IsInitialized{false};
    bool m_IsPrepared;
    bool m_IsFinalizing;
    bool m_IsPreparedPrefetch;
    bool m_PauseStatus;
    bool m_LoadWaitFlag;
    bool m_LoadFinishFlag;
    bool m_ReportLoadingDelayFlag;
    bool m_IsStoppedByLoadingDelay;
    bool m_IsRegisterPlayerCallback{false};
    bool m_UseDelayCount;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    uint8_t m_Padding1[2];
#endif
    int m_LoopCounter;
    int m_PlayingBlockLoopCounter;
    int m_PrepareCounter;
    StreamSoundLoaderManager* m_pLoaderManager{};
    StreamSoundLoader* m_pLoader{};
    detail::driver::StreamBufferPool* m_pBufferPool;
    int m_BufferBlockCount;
    uint32_t m_LoadingBufferBlockIndex;
    uint32_t m_PlayingBufferBlockIndex;
    uint32_t m_LastPlayFinishBufferBlockIndex;
    StartOffsetType m_StartOffsetType;
    position_t m_StartOffset;
    int m_DelayCount;
    uint16_t m_AssignNumber;
    uint8_t m_FileType;
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    DecodeMode m_DecodeMode;
#endif
    bool m_LoopFlag;
    uint8_t m_Padding2[2];
    StreamDataInfoDetail m_StreamDataInfo;
    position_t m_LoopStart;
    position_t m_LoopEnd;
    ItemData m_ItemData;
    const void* m_pStreamPrefetchFile{};
    AdpcmParam m_PrefetchAdpcmParam[StreamChannelCount];
    StreamSoundPrefetchFileReader::PrefetchDataInfo m_PrefetchDataInfo;
    position_t m_PrefetchOffset;
    bool m_IsPrefetchRevisionCheckEnabled;
    uint32_t m_PrefetchRevisionValue;
    int m_ChannelCount{0};
    int m_TrackCount{0};
    StreamChannel m_Channels[StreamChannelCount];
    StreamTrack m_Tracks[StreamTrackCount];
    UpdateType m_UpdateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    uint32_t m_SubMixIndex;
#endif
    WaveBufferInfo m_WaveBufferInfo[StreamDataLoadTaskMax];
    PrepareArg m_PrepareArg;
    bool m_IsSucceedPrepare{false};
    SetupArg m_SetupArg;

    static uint16_t g_AssignNumberCount;

    position_t m_PlaySamplePosition;
    position_t m_OriginalPlaySamplePosition;

    static uint16_t g_TaskRequestIndexCount;
};
static_assert(sizeof(StreamSoundPlayer) == 0x11740);

}  // namespace detail::driver
}  // namespace nn::atk
