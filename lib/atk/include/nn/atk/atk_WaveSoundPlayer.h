#pragma once

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_WaveSoundFileReader.h>
#include <nn/atk/atk_WaveSoundLoader.h>

namespace nn::atk::detail {

struct WaveSoundDataInfo {
    bool loopFlag;
    int32_t sampleRate;
    int64_t loopStart;
    int64_t loopEnd;
    int64_t compatibleLoopStart;
    int64_t compatibleLoopEnd;
    int32_t channelCount;
};
static_assert(sizeof(WaveSoundDataInfo) == 0x30);

namespace driver {

class WaveSoundPlayer : BasicSoundPlayer, DisposeCallback, SoundThread::PlayerCallback {
public:
    enum StartOffsetType {
        StartOffsetType_Sample,
        StartOffsetType_Millisec,
    };

    enum ResState {
        ResState_Invalid,
        ResState_ReceiveLoadReq,
        ResState_AppendLoadTask,
        ResState_Assigned,
    };

    constexpr static int32_t SignatureFile = 0x44535746;  // FWSD

    constexpr static uint32_t PauseReleaseValue = 127;
    constexpr static uint32_t MuteReleaseValue = 127;
    constexpr static uint32_t DefaultPriority = 64;

    struct StartInfo {
        int32_t index;
        StartOffsetType startOffsetType;
        int32_t startOffset;
        int32_t delayTime;
        int32_t delayCount;
        int32_t waveSoundParameterFlag;
        int32_t release;
        bool isContextCalculationSkipMode;
        UpdateType updateType;
    };
    static_assert(sizeof(StartInfo) == 0x24);

    struct PrepareArg {
        void* wsdFile;
        void* waveFile;
        int8_t waveType;
        uint8_t padding[3];
    };
    static_assert(sizeof(PrepareArg) == 0x18);

    WaveSoundPlayer();
    ~WaveSoundPlayer() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void Initialize() override;
#else
    void Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void FinishPlayer();

    void CloseChannel();

    void FreeLoader();

    void Prepare(const StartInfo& info, const PrepareArg& arg);
    void PrepareForPlayerHeap(const PrepareArg& arg);

    void RequestLoad(const StartInfo& info, const WaveSoundLoader::Arg& arg);

    void Start() override;
    void Stop() override;
    void Pause(bool flag) override;

    void SetPanRange(float range);
    void SetChannelPriority(int32_t priority);
    void SetReleasePriorityFix(bool fix);

    void InvalidateData(const void* start, const void* end) override;

    position_t GetPlaySamplePosition(bool) const;

    void Update();

    bool TryAllocLoader();

    bool StartChannel();
    void UpdateChannel();

    static void ChannelCallbackFunc(Channel* dropChannel, Channel::ChannelCallbackStatus status,
                                    void* userData);

    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

private:
    bool m_WavePlayFlag;
    bool m_ReleasePriorityFixFlag;
    uint8_t m_Priority;
    int8_t m_WaveType;
    float m_PanRange;
    void* m_pWsdFile;
    void* m_pWaveFile;
    int32_t m_WaveSoundIndex;
    StartOffsetType m_StartOffsetType;
    position_t m_StartOffset;
    int32_t m_DelayCount;
    int32_t m_Release;
    int32_t m_WaveSoundParameterFlag;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    bool m_IsContextCalculationSkipMode;
#endif
    CurveLfoParam m_LfoParam;
    WaveSoundInfo m_WaveSoundInfo;
    Channel* m_pChannel;
    UpdateType m_UpdateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    uint32_t m_SubMixIndex;
#endif
    uint8_t m_ResState;
    bool m_IsInitialized;
    bool m_IsRegisterPlayerCallback;
    uint8_t m_Padding[1];
    WaveSoundLoaderManager* m_pLoaderManager;
    WaveSoundLoader* m_pLoader;
    WaveSoundLoader::Arg m_LoaderArg;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(WaveSoundPlayer) == 0x190);
#else
static_assert(sizeof(WaveSoundPlayer) == 0x1a0);
#endif

}  // namespace driver
}  // namespace nn::atk::detail
