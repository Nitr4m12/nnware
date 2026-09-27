#pragma once

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_StreamBufferPool.h>
#include <nn/atk/atk_StreamSoundLoader.h>
#include <nn/atk/atk_StreamSoundPrefetchFileReader.h>
#include <nn/atk/atk_StreamTrack.h>

namespace nn::atk::detail {

struct StreamDataInfo {
    bool loopFlag;
    int32_t sampleRate;
    int64_t loopStart;
    int64_t loopEnd;
    int64_t compatibleLoopStart;
    int64_t compatibleLoopEnd;
    int32_t channelCount;
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
};
static_assert(sizeof(StreamSoundDataInfo) == 0x30);

struct StreamSoundRegionDataInfo {
    uint32_t startSamplePosition;
    uint32_t endSamplePosition;
    int32_t regionNo;
    char regionName[64];
};
static_assert(sizeof(StreamSoundRegionDataInfo) == 0x4c);

namespace driver {

class StreamSoundPlayer : BasicSoundPlayer, SoundThread::PlayerCallback {
public:
    enum StartOffsetType {
        StartOffsetType_Sample,
        StartOffsetType_Millisec,
    };

    struct SetupArg;
    struct ItemData {
        float pitch;
        float mainSend;
        float fxSend[3];

        void Set(const SetupArg& arg);
    };
    static_assert(sizeof(ItemData) == 0x14);

    struct TrackData {
        float volume;
        float lpfFreq;
        int32_t biquadType;
        float biquadValue;
        float pan;
        float span;
        float mainSend;
        float fxSend[3];

        void Set(const StreamTrack* pStreamTrack);
    };
    static_assert(sizeof(TrackData) == 0x28);

    struct WaveBufferInfo {
        position_t sampleBegin;
        size_t sampleLength;
        int32_t loopCount;
    };
    static_assert(sizeof(WaveBufferInfo) == 0x18);

    struct PrepareBaseArg {
        StartOffsetType startOffsetType;
        position_t offset;
        int32_t delayTime;
        int32_t delayCount;
        UpdateType updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 4, 1)
        uint32_t subMixIndex;
#endif
        StreamRegionCallback regionCallback;
        void* regionCallbackArg;
        char filePath[639];
        void* pExternalData;
        size_t externalDataSize;
        FileStreamHookParam fileStreamHookParam;
    };
    static_assert(sizeof(PrepareBaseArg) == 0x2d0);

    struct PrepareArg {
        PrepareBaseArg baseArg;
        void* cacheBuffer;
        size_t cacheSize;
    };
    static_assert(sizeof(PrepareArg) == 0x2e0);

    struct PreparePrefetchArg {
        PrepareBaseArg baseArg;
        void* strmPrefetchFile;
    };
    static_assert(sizeof(PreparePrefetchArg) == 0x2d8);

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
        uint8_t fxSend[3];
#if NN_WARE_VER >= NN_MAKE_VER(4, 4, 1)
        DecodeMode decodeMode;
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(4, 4, 1)
    static_assert(sizeof(SetupArg) == 0x98);
#else
    static_assert(sizeof(SetupArg) == 0xa0);
#endif

    struct PrefetchIndexInfo {
        void Initialize(const StreamDataInfoDetail& streamDataInfo);

        uint32_t lastBlockIndex;
        position_t loopStartInBlock;
        uint32_t loopStartBlockIndex;
        int32_t loopBlockCount;
    };
    static_assert(sizeof(PrefetchIndexInfo) == 0x18);

    struct PrefetchLoadDataParam : public LoadDataParam {
        uint32_t prefetchBlockIndex;
        uint32_t _padding;
        size_t prefetchBlockBytes;
    };
    static_assert(sizeof(PrefetchLoadDataParam) == 0xa8);

    StreamSoundPlayer();
    ~StreamSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif

    bool TryAllocLoader();

    void Finalize() override;

    void FinishPlayer();

    void FreeStreamBuffers();
    void FreeVoices();
    void FreeLoader();

    void Setup(const SetupArg& arg);
    void SetupTrack(const SetupArg& arg);

    void Prepare(const PrepareArg& arg);

    void SetPrepareBaseArg(const PrepareBaseArg& arg);

    void RequestLoadHeader(const PrepareArg& arg);

    void PreparePrefetch(const PreparePrefetchArg& arg);

    bool ReadPrefetchFile(StreamSoundPrefetchFileReader& reader);

    bool ApplyStreamDataInfo(const StreamDataInfoDetail& streamDataInfo);

    bool SetupPlayer();

    bool AllocVoices();

    bool LoadPrefetchBlocks(StreamSoundPrefetchFileReader& reader);

    void Start() override;
    void StartPlayer();

    void Stop() override;

    void Pause(bool flag) override;

    void UpdatePauseStatus();

    bool IsLoadingDelayState() const;
    bool IsBufferEmpty() const;

    bool ReadStreamDataInfo(StreamDataInfo* strmDataInfo) const;
    bool ReadStreamDataInfo(StreamSoundDataInfo* strmDataInfo) const;

    position_t GetPlaySamplePosition(bool) const;
    float GetFilledBufferPercentage() const;
    int32_t GetBufferBlockCount(WaveBuffer::Status waveBufferStatus) const;
    int32_t GetTotalBufferBlockCount() const;

    bool LoadHeader(bool result, AdpcmParam** adpcmParam, uint16_t assignNumber);

    bool CheckPrefetchRevision(const StreamDataInfoDetail& streamDataInfo) const;

    bool AllocStreamBuffers();

    void UpdateLoadingBlockIndex();

    bool LoadStreamData(bool result, const LoadDataParam& loadDataParam, uint16_t assignNumber);
    bool LoadStreamData(bool result, const LoadDataParam& loadDataParam, uint16_t assignNumber,
                        bool usePrefetchFlag, uint32_t currentPrefetchBlockIndex,
                        size_t currentPrefetchBlockBytes);

    bool IsStoppedByLoadingDelay() const;

    static void VoiceCallbackFunc(MultiVoice* voice, MultiVoice::VoiceCallbackStatus status,
                                  void* arg);

    void Update();
    void UpdateBuffer();
    void UpdateVoiceParams(StreamTrack* track);

    bool CheckDiskDriveError();

    void SetOutputParam(const OutputParam*, const OutputParam&, const TrackData&);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void ApplyTvOutputParamForMultiChannel(OutputParam* outputParam, MultiVoice* MultiVoice,
                                           int32_t channelIndex, MixMode mixMode);
#else
    void ApplyTvOutputParamForMultiChannel(OutputParam* outputParam,
                                           OutputAdditionalParam* pOutputAdditionalParam,
                                           MultiVoice* MultiVoice, int32_t channelIndex,
                                           MixMode mixMode);
#endif

    void MixSettingForOutputParam(OutputParam* outputParam, int32_t channelIndex, MixMode mixMode);

    position_t GetOriginalPlaySamplePosition(position_t,
                                             const StreamDataInfoDetail& streamDataInfo) const;

    bool IsValidStartOffset(const StreamDataInfoDetail& streamDataInfo);

    void ApplyTrackDataInfo(const StreamDataInfoDetail& streamDataInfo);

    uint64_t GetStartOffsetSamples(const StreamDataInfoDetail& streamDataInfo);

    void PreparePrefetchOnLastBlock(PrefetchLoadDataParam*, const PrefetchIndexInfo&);
    void PreparePrefetchOnLoopStartBlock(PrefetchLoadDataParam*, const PrefetchIndexInfo&,
                                         StreamSoundPrefetchFileReader& reader);
    void PreparePrefetchOnLoopBlock(PrefetchLoadDataParam*, const PrefetchIndexInfo&, uint32_t);
    bool PreparePrefetchOnNormalBlock(PrefetchLoadDataParam*, uint32_t,
                                      StreamSoundPrefetchFileReader* reader);

    bool SetAdpcmLoopInfo(StreamSoundPrefetchFileReader& reader,
                          const StreamDataInfoDetail& streamDataInfo, AdpcmParam* adpcmParam,
                          AdpcmContextNotAligned* adpcmContext);
    bool SetAdpcmInfo(StreamSoundPrefetchFileReader& reader,
                      const StreamDataInfoDetail& streamDataInfo, AdpcmParam* adpcmParam,
                      AdpcmContextNotAligned* adpcmContext);

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

    StreamTrack* GetPlayerTrack(int32_t index);
    StreamTrack* GetPlayerTrack(int32_t index) const;

    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

private:
    bool m_IsInitialized;
    bool m_IsPrepared;
    bool m_IsFinalizing;
    bool m_IsPreparedPrefetch;
    bool m_PauseStatus;
    bool m_LoadWaitFlag;
    bool m_LoadFinishFlag;
    bool m_ReportLoadingDelayFlag;
    bool m_IsStoppedByLoadingDelay;
    bool m_IsRegisterPlayerCallback;
    bool m_UseDelayCount;
#if NN_WARE_VER >= NN_MAKE_VER(5, 3, 0)
    uint8_t m_Padding1[2];
#endif
    int32_t m_LoopCounter;
    int32_t m_PlayingBlockLoopCounter;
    int32_t m_PrepareCounter;
    StreamSoundLoaderManager* m_pLoaderManager;
    StreamSoundLoader* m_pLoader;
    detail::driver::StreamBufferPool* m_pBufferPool;
    int32_t m_BufferBlockCount;
    uint32_t m_LoadingBufferBlockIndex;
    uint32_t m_PlayingBufferBlockIndex;
    uint32_t m_LastPlayFinishBufferBlockIndex;
    StartOffsetType m_StartOffsetType;
    position_t m_StartOffset;
    int32_t m_DelayCount;
    uint16_t m_AssignNumber;
    uint8_t m_FileType;
#if NN_WARE_VER >= NN_MAKE_VER(4, 4, 1)
    DecodeMode m_DecodeMode;
#endif
    bool m_LoopFlag;
    uint8_t m_Padding2[2];
    StreamDataInfoDetail m_StreamDataInfo;
    position_t m_LoopStart;
    position_t m_LoopEnd;
    ItemData m_ItemData;
    void* m_pStreamPrefetchFile;
    AdpcmParam m_PrefetchAdpcmParam[16];
    StreamSoundPrefetchFileReader::PrefetchDataInfo m_PrefetchDataInfo;
    position_t m_PrefetchOffset;
    bool m_IsPrefetchRevisionCheckEnabled;
    uint32_t m_PrefetchRevisionValue;
    int32_t m_ChannelCount;
    int32_t m_TrackCount;
    StreamChannel m_Channels[16];
    StreamTrack m_Tracks[8];
    UpdateType m_UpdateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 4, 1)
    uint32_t m_SubMixIndex;
#endif
    WaveBufferInfo m_WaveBufferInfo[32];
    PrepareArg m_PrepareArg;
    bool m_IsSucceedPrepare;
    SetupArg m_SetupArg;
    position_t m_PlaySamplePosition;
    position_t m_OriginalPlaySamplePosition;

    static uint16_t g_TaskRequestIndexCount;
    static uint16_t g_AssignNumberCount;
};
static_assert(sizeof(StreamSoundPlayer) == 0x11740);

}  // namespace driver
}  // namespace nn::atk::detail
