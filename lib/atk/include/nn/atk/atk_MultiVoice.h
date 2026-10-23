#pragma once

#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_Voice.h>

namespace nn::atk::detail::driver {

class MultiVoice {
    NN_NO_COPY(MultiVoice);

public:
    static const int32_t UpdateStart{1 << 0};
    static const int32_t UpdatePause{1 << 1};
    static const int32_t UpdateSrc{1 << 2};
    static const int32_t UpdateMix{1 << 3};
    static const int32_t UpdateLpf{1 << 4};
    static const int32_t UpdateBiquad{1 << 5};
    static const int32_t UpdateVe{1 << 6};

    enum VoiceCallbackStatus {
        VoiceCallbackStatus_FinishWave,
        VoiceCallbackStatus_Cancel,
        VoiceCallbackStatus_DropVoice,
        VoiceCallbackStatus_DropDsp,
    };

    using VoiceCallback = void (*)(MultiVoice*, VoiceCallbackStatus, void*);

    static const float VolumeMin;
    static const float VolumeDefault;
    static const float VolumeMax;

    static const float PanLeft;
    static const float PanCenter;
    static const float PanRight;

    static const float SpanFront;
    static const float SpanCenter;
    static const float SpanRear;

    static const float CutoffFreqMin;
    static const float CutoffFreqMax;

    static const float BiquadValueMin;
    static const float BiquadValueMax;

    static const float SendMin;
    static const float SendMax;

    static const uint32_t PriorityNoDrop{255};

    MultiVoice();
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    explicit MultiVoice(OutputAdditionalParam* pAdditionalParam);
#endif
    ~MultiVoice();

    bool Alloc(int32_t channelCount, int32_t priority, VoiceCallback callback, void* callbackData);
    void Free();

    void Start();
    void Stop();
    void UpdateVoiceStatus();
    void Pause(bool flag);

    void Calc();

    void Update();

    bool IsActive() const { return m_IsActive; }
    bool IsRun() const;
    bool IsPause() const { return m_IsPause; }
    bool IsPlayFinished() const;

    void SetSampleFormat(SampleFormat format);
    void SetSampleRate(int32_t sampleRate);

    float GetVolume() const { return m_Volume; }
    void SetVolume(float volume);

    float GetPitch() const { return m_Pitch; }
    void SetPitch(float pitch);

    void SetPanMode(PanMode panMode);
    void SetPanCurve(PanCurve panCurve);

    float GetLpfFreq() const { return m_LpfFreq; }
    void SetLpfFreq(float lpfFreq);

    int32_t GetBiquadType() const { return m_BiquadType; }
    float GetBiquadValue() const { return m_BiquadValue; }
    void SetBiquadFilter(int32_t type, float value);

    int32_t GetPriority() const { return m_Priority; }
    void SetPriority(int32_t priority);

    void SetInterpolationType(uint8_t interpolationType);

    int32_t GetSdkVoiceCount() const { return m_ChannelCount; }

    const Voice& detail_GetSdkVoice(int32_t channelIndex) const;

    void SetOutputLine(uint32_t lineFlag);
    uint32_t GetOutputLine() const { return m_OutputLineFlag; }

    void SetTvParam(const OutputParam& param);
    const OutputParam& GetTvParam() const { return m_TvParam; }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void SetSubMixIndex(int32_t subMixIndex);
#endif

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetTvAdditionalParam(const OutputAdditionalParam& param);

    void SetTvAdditionalParam(const SendArray* pAdditionalSend,
                              const BusMixVolumePacket* pBusMixVolumePacket,
                              const OutputBusMixVolume* pBusMixVolume,
                              const VolumeThroughModePacket* pVolumeThroughModePacket);

    const OutputAdditionalParam* const GetTvAdditionalParamAddr() const {
        return m_pTvAdditionalParam;
    }
#endif

    position_t GetCurrentPlayingSample() const;
    SampleFormat GetFormat() const;

    static size_t FrameToByte(position_t sample, SampleFormat format);

    static void CalcOffsetAdpcmParam(AdpcmContext* context, const AdpcmParam& param,
                                     position_t offsetSamples, const void* dataAddress);

    void AppendWaveBuffer(int32_t channelIndex, WaveBuffer* pBuffer, bool lastFlag);

    void SetAdpcmParam(int32_t channelIndex, const AdpcmParam& param);

    void SetUpdateType(UpdateType updateType) { m_UpdateType = updateType; }
    UpdateType GetUpdateType() const { return m_UpdateType; }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetOutputReceiver(OutputReceiver* pOutputReceiver);
#endif

    os::Tick GetProcessTick(const SoundProfile& profile) {
        os::Tick totalTick{0};

        for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
            totalTick += m_Voice[channelIndex].GetProcessTick(profile);

        return totalTick;
    }

private:
    struct PreMixVolume {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        float volume[ChannelIndex_Count];
#else
        float volume[OutputMix::ChannelCountMax];
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static_assert(sizeof(PreMixVolume) == 0x18);
#else
    static_assert(sizeof(PreMixVolume) == 0x60);
#endif

    void InitParam(VoiceCallback callback, void* callbackData);

    void CalcSrc(bool initialUpdate);
    void CalcVe();
    void CalcMix();
    void CalcLpf();
    void CalcBiquadFilter();

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param, int32_t channelIndex,
                          OutputDevice device);
#else
    void CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param,
                          const OutputAdditionalParam* const pAdditionalParam, int32_t channelIndex,
                          OutputDevice device);
#endif

    void CalcTvMix(OutputMix* mix, const PreMixVolume& preMix);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                     const PreMixVolume& pre);
#else
    void CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                     OutputAdditionalParam* pAdditionalParam, const PreMixVolume& pre);
#endif

    void RunAllSdkVoice();
    void StopAllSdkVoice();
    void PauseAllSdkVoice();

    void SetOutputParamImpl(const OutputParam& in, OutputParam& out);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void
    SetOutputAdditionalParamImpl(const SendArray* const pAdditionalSend,
                                 const BusMixVolumePacket* const pBusMixVolumePacket,
                                 const OutputBusMixVolume* const pBusMixVolume,
                                 const VolumeThroughModePacket* const pVolumeThroughModePacket);
    void SetOutputBusMixVolumeImpl(const BusMixVolumePacket& in,
                                   const OutputBusMixVolume& busMixVolume, BusMixVolumePacket& out);
    void SetOutputVolumeThroughModePacketImpl(const VolumeThroughModePacket& in,
                                              VolumeThroughModePacket& out);
#endif

    Voice m_Voice[WaveChannelMax];
    int32_t m_ChannelCount{0};
    VoiceCallback m_Callback{};
    void* m_pCallbackData;
    bool m_IsActive{false};
    bool m_IsStart{false};
    bool m_IsStarted{false};
    bool m_IsPause{false};
    bool m_IsPausing;
    bool m_IsInitialized;
    WaveBuffer* m_pLastWaveBuffer;
    uint16_t m_SyncFlag{0};
    uint8_t m_BiquadType;
    bool m_IsEnableFrontBypass;
    float m_Volume;
    float m_Pitch;
    PanMode m_PanMode;
    PanCurve m_PanCurve;
    float m_LpfFreq;
    float m_BiquadValue;
    int32_t m_Priority;
    uint32_t m_OutputLineFlag;
    OutputParam m_TvParam;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputAdditionalParam* m_pTvAdditionalParam{};
#endif
    SampleFormat m_Format;
    uintptr_t m_VoiceUser;
    UpdateType m_UpdateType{UpdateType_AudioFrame};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* m_pOutputReceiver{};
#endif

public:
    util::IntrusiveListNode m_LinkNode;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(MultiVoice) == 0x270);
#else
static_assert(sizeof(MultiVoice) == 0x298);
#endif

}  // namespace nn::atk::detail::driver
