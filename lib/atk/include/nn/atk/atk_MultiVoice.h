#pragma once

#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_Voice.h>

namespace nn::atk::detail::driver {

class MultiVoiceManager;

class MultiVoice {
public:
    struct PreMixVolume {
        float volume[24];
    };
    static_assert(sizeof(PreMixVolume) == 0x60);

    enum VoiceCallbackStatus {
        VoiceCallbackStatus_FinishWave,
        VoiceCallbackStatus_Cancel,
        VoiceCallbackStatus_DropVoice,
        VoiceCallbackStatus_DropDsp,
    };

    using VoiceCallback = void (*)(MultiVoice*, VoiceCallbackStatus, void*);

    constexpr static uint32_t UpdateStart = 0b0000001;
    constexpr static uint32_t UpdatePause = 0b0000010;
    constexpr static uint32_t UpdateSrc = 0b0000100;
    constexpr static uint32_t UpdateMix = 0b0001000;
    constexpr static uint32_t UpdateLpf = 0b0010000;
    constexpr static uint32_t UpdateBiquad = 0b0100000;
    constexpr static uint32_t UpdateVe = 0b1000000;

    constexpr static uint32_t PriorityNoDrop = 255;

    constexpr static float VolumeMin = 0.0;
    constexpr static float VolumeDefault = 1.0;
    constexpr static float VolumeMax = 2.0;

    constexpr static float PanLeft = -1.0;
    constexpr static float PanCenter = 0.0;
    constexpr static float PanRight = 1.0;

    constexpr static float SpanFront = 0.0;
    constexpr static float SpanCenter = 1.0;
    constexpr static float SpanRear = 2.0;

    constexpr static float CutoffFreqMin = 0.0;
    constexpr static float CutoffFreqMax = 1.0;

    constexpr static float BiquadValueMin = 0.0;
    constexpr static float BiquadValueMax = 1.0;

    constexpr static float SendMin = 0.0;
    constexpr static float SendMax = 1.0;

    MultiVoice();
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    explicit MultiVoice(OutputAdditionalParam* pAdditionalParam);
#endif
    ~MultiVoice();

    bool Alloc(int32_t channelCount, int32_t priority, VoiceCallback callback, void* callbackData);
    void InitParam(VoiceCallback callback, void* callbackData);
    void Free();

    void Start();
    void Stop();
    void StopAllSdkVoice();

    void UpdateVoiceStatus();

    bool IsPlayFinished() const;

    void Pause(bool flag);

    void Calc();
    void CalcSrc(bool);
    void CalcVe();
    void CalcMix();
    void CalcLpf();
    void CalcBiquadFilter();

    void Update();

    void RunAllSdkVoice();
    void PauseAllSdkVoice();

    void SetSampleFormat(SampleFormat format);
    void SetSampleRate(int32_t sampleRate);
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetPanMode(PanMode panMode);
    void SetPanCurve(PanCurve panCurve);
    void SetLpfFreq(float lpfFreq);
    void SetBiquadFilter(int32_t type, float value);
    void SetPriority(int32_t priority);
    void SetOutputLine(uint32_t lineFlag);
    void SetOutputParamImpl(const OutputParam& in, const OutputParam& out);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetOutputAdditionalParamImpl(const SendArray* pAdditionalSend,
                                      const BusMixVolumePacket* pBusMixVolumePacket,
                                      const OutputBusMixVolume* pBusMixVolume,
                                      const VolumeThroughModePacket* pVolumeThroughModePacket);
#endif
    void SetOutputBusMixVolumeImpl(const BusMixVolumePacket& in,
                                   const OutputBusMixVolume& busMixVolume,
                                   const BusMixVolumePacket& out);
    void SetOutputVolumeThroughModePacketImpl(const VolumeThroughModePacket&,
                                              const VolumeThroughModePacket&);
    void SetTvParam(const OutputParam& param);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetTvAdditionalParam(const OutputAdditionalParam& param);
    void SetTvAdditionalParam(const SendArray* pAdditionalSend,
                              const BusMixVolumePacket* pBusMixVolumePacket,
                              const OutputBusMixVolume* pBusMixVolume,
                              const VolumeThroughModePacket* pVolumeThroughModePacket);
    void SetOutputReceiver(OutputReceiver* pOutputReceiver);
#endif
    void SetSubMixIndex(int32_t subMixIndex);

    void SetUpdateType(UpdateType updateType) { m_UpdateType = updateType; }
    UpdateType GetUpdateType() const { return m_UpdateType; }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param, int32_t channelIndex,
                          OutputDevice device);
#else
    void CalcPreMixVolume(PreMixVolume* mix, const OutputParam& param,
                          OutputAdditionalParam* pAdditionalParam, int32_t channelIndex,
                          OutputDevice device);
#endif
    void CalcTvMix(OutputMix* mix, const PreMixVolume& pre);
    void CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                     const PreMixVolume& pre);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void CalcMixImpl(OutputMix* mix, uint32_t outputDeviceIndex, const OutputParam& param,
                     OutputAdditionalParam* pAdditionalParam, const PreMixVolume& pre);
#endif

    MultiVoice* detail_GetSdkVoice(int32_t) const;

    int GetSdkVoiceCount() const { return m_ChannelCount; }

    position_t GetCurrentPlayingSample() const;
    SampleFormat GetFormat() const;

    bool IsRun() const;

    void SetInterpolationType(uint8_t interpolationType);

    void AppendWaveBuffer(int32_t channelIndex, WaveBuffer* pBuffer, bool lastFlag);

    void SetAdpcmParam(int32_t channelIndex, const AdpcmParam& param);

    static uint64_t FrameToByte(int64_t, SampleFormat);
    static void CalcOffsetAdpcmParam(AdpcmContext* context, const AdpcmParam& param,
                                     position_t offsetSamples, const void* dataAddress);

    os::Tick GetProcessTick(const SoundProfile& profile) {
        os::Tick totalTick{0};

        for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
            totalTick += m_Voice[channelIndex].GetProcessTick(profile);

        return totalTick;
    }

private:
    friend MultiVoiceManager;

    Voice m_Voice[2];
    int32_t m_ChannelCount;
    VoiceCallback m_Callback;
    void* m_pCallbackData;
    bool m_IsActive;
    bool m_IsStart;
    bool m_IsStarted;
    bool m_IsPause;
    bool m_IsPausing;
    bool m_IsInitialized;
    WaveBuffer* m_pLastWaveBuffer;
    uint16_t m_SyncFlag;
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
    OutputAdditionalParam* m_pTvAdditionalParam;
#endif
    SampleFormat m_Format;
    std::uintptr_t m_VoiceUser;
    UpdateType m_UpdateType;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* m_pOutputReceiver;
#endif
    util::IntrusiveListNode m_LinkNode;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(MultiVoice) == 0x270);
#else
static_assert(sizeof(MultiVoice) == 0x298);
#endif

}  // namespace nn::atk::detail::driver
