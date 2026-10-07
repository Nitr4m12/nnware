#pragma once

#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_WaveSoundFileReader.h>
#include <nn/atk/atk_WaveSoundLoader.h>

namespace nn::atk {

struct WaveSoundDataInfo {
    bool loopFlag;
    int32_t sampleRate;
    int64_t loopStart;
    int64_t loopEnd;
    int64_t compatibleLoopStart;
    int64_t compatibleLoopEnd;
    int32_t channelCount;

    void Dump();
};
static_assert(sizeof(WaveSoundDataInfo) == 0x30);

namespace detail::driver {

class WaveSoundPlayer : public BasicSoundPlayer,
                        public DisposeCallback,
                        public SoundThread::PlayerCallback {
public:
    static const int PauseReleaseValue{127};
    static const int MuteReleaseValue{127};
    static const int DefaultPriority{64};

    enum StartOffsetType {
        StartOffsetType_Sample,
        StartOffsetType_Millisec,
    };

    struct StartInfo {
        int32_t index;
        StartOffsetType startOffsetType;
        int32_t startOffset;
        int32_t delayTime;
        int32_t delayCount;
        int32_t waveSoundParameterFlag;
        int32_t release;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        UpdateType updateType;
        int32_t subMixIndex;
#else
        bool isContextCalculationSkipMode;
        UpdateType updateType;
#endif
    };
    static_assert(sizeof(StartInfo) == 0x24);

    struct PrepareArg {
        const void* wsdFile;
        const void* waveFile;
        int8_t waveType;
        uint8_t padding[3];

        PrepareArg() = default;
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

    bool IsPrepared() const;

    void SetLoaderManager(WaveSoundLoaderManager* manager) { m_pLoaderManager = manager; }

    void Prepare(const StartInfo& info, const PrepareArg& arg);
    void RequestLoad(const StartInfo& info, const WaveSoundLoader::Arg& arg);

    void Start() override;
    void Stop() override;
    void Pause(bool flag) override;

    void SetPanRange(float range);
    void SetChannelPriority(int32_t priority);
    void SetReleasePriorityFix(bool fix);

    float GetPanRange() const { return m_PanRange; }
    int32_t GetChannelPriority() const { return m_Priority; }

    void InvalidateData(const void* start, const void* end) override;

    position_t GetPlaySamplePosition(bool isOriginalSamplePosition) const;

    const void* GetWaveFile() const { return m_pWaveFile; }
    UpdateType GetUpdateType() const { return m_UpdateType; }

    os::Tick GetProcessTick(const SoundProfile& profile);

    void DebugUpdate();

private:
    void OnUpdateFrameSoundThread() override;
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency() override;
    void OnShutdownSoundThread() override;

    void PrepareForPlayerHeap(const PrepareArg& arg);

    bool TryAllocLoader();
    void FreeLoader();

    void FinishPlayer();

    void Update();

    bool IsChannelActive() {
        if (m_pChannel == nullptr)
            return false;

        return m_pChannel->IsActive();
    }

    bool StartChannel();
    void CloseChannel();
    void UpdateChannel();

    static void ChannelCallbackFunc(Channel* dropChannel, Channel::ChannelCallbackStatus status,
                                    void* userData);

    bool m_WavePlayFlag;
    bool m_ReleasePriorityFixFlag;
    uint8_t m_Priority;
    int8_t m_WaveType;
    float m_PanRange;
    const void* m_pWsdFile;
    const void* m_pWaveFile;
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

    enum ResState {
        ResState_Invalid,
        ResState_ReceiveLoadReq,
        ResState_AppendLoadTask,
        ResState_Assigned,
    };
    uint8_t m_ResState;

    bool m_IsInitialized{false};
    bool m_IsRegisterPlayerCallback{false};
    uint8_t m_Padding[1];
    WaveSoundLoaderManager* m_pLoaderManager{};
    WaveSoundLoader* m_pLoader{};
    WaveSoundLoader::Arg m_LoaderArg;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(WaveSoundPlayer) == 0x190);
#else
static_assert(sizeof(WaveSoundPlayer) == 0x1a0);
#endif

}  // namespace detail::driver
}  // namespace nn::atk
