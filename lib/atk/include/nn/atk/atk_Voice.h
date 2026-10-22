#pragma once

#include <nn/util/util_BitFlagSet.h>

#include <nn/atk/atk_LowLevelVoice.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

class Voice {
    NN_NO_COPY(Voice);

public:
    static const uint32_t PriorityMin{0};
    static const uint32_t PriorityMax{255};
    static const uint32_t PriorityNoDrop{255};

    Voice();
    ~Voice();

    bool AllocVoice(uint32_t priority);
    void Free();

    bool IsAvailable() const;

    void SetState(VoiceState state);

    void AppendWaveBuffer(WaveBuffer* waveBuffer);

    void SetSampleFormat(SampleFormat format) { m_SampleFormat = format; }
    void SetSampleRate(uint32_t sampleRate) { m_SampleRate = sampleRate; }
    void SetAdpcmParam(const AdpcmParam& param) { m_AdpcmParam = param; }
    void SetPriority(uint32_t priority);
    void SetVolume(float volume) { m_VoiceParam.m_Volume = volume; }
    void SetPitch(float pitch) { m_VoiceParam.m_Pitch = pitch; }
    void SetTvMix(const OutputMix& mix) { m_VoiceParam.m_TvMix = mix; }
    void SetInterpolationType(uint8_t interpolationType) {
        m_VoiceParam.m_InterpolationType = interpolationType;
    }

    void SetMonoFilter(bool enable, uint16_t cutoff);
    void SetBiquadFilter(bool enable, const BiquadFilterCoefficients* coef);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void SetSubMixIndex(int32_t subMixIndex) { m_SubMixIndex = subMixIndex; }
#else
    void SetOutputReceiver(OutputReceiver* pOutputReceiver) { m_pOutputReceiver = pOutputReceiver; }
#endif

    void UpdateParam();
    void UpdateVoiceStatus();

    VoiceState GetState() const { return m_State; }
    position_t GetPlayPosition() const;
    uint32_t GetPriority() const { return m_Priority; }
    float GetVolume() const { return m_VoiceParam.m_Volume; }
    float GetPitch() const { return m_VoiceParam.m_Pitch; }
    const OutputMix& GetTvMix() const { return m_VoiceParam.m_TvMix; }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (m_pLowLevelVoice == nullptr)
            return 0;

        audio::NodeId voiceNodeId{m_pLowLevelVoice->GetNodeId()};
        for (uint32_t voiceIndex{0}; voiceIndex < profile.rendererVoiceCount; ++voiceIndex) {
            if (profile._voiceIdTable[voiceIndex] == voiceNodeId) {
                return profile._voiceProcessTable[voiceIndex].end -
                       profile._voiceProcessTable[voiceIndex].begin;
            }
        }

        return 0;
    }

private:
    void Initialize(uint32_t priority);

    void FreeAllWaveBuffer();

    uint32_t m_Priority;
    VoiceState m_State;
    VoiceParam m_VoiceParam;
    SampleFormat m_SampleFormat;
    uint32_t m_SampleRate;
    AdpcmParam m_AdpcmParam;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    int32_t m_SubMixIndex;
#else
    OutputReceiver* m_pOutputReceiver;
#endif
    uint32_t m_VoiceId;
    position_t m_PlayPosition;
    uint32_t m_VoiceInfoEnableFlag;
    uint32_t m_CommandTag;
    WaveBuffer* m_WaveBufferListBegin;
    WaveBuffer* m_WaveBufferListEnd;
    LowLevelVoice* m_pLowLevelVoice{};
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(Voice) == 0xd8);
#else
static_assert(sizeof(Voice) == 0xe0);
#endif

class VirtualVoiceManager {
public:
    static const uint32_t InvalidVoiceId{0xffffffff};

    static const int32_t VirtualVoiceCount{256};

    static VirtualVoiceManager& GetInstance();

    void Initialize();

    uint32_t AllocVirtualVoice();
    void FreeVirtualVoice(uint32_t voiceId);

    void SetLowLevelVoice(uint32_t voiceId, LowLevelVoice* lowLevelVoice) {
        m_LowLevelVoiceTable[voiceId] = lowLevelVoice;
    }

    LowLevelVoice* GetLowLevelVoice(uint32_t voiceId) { return m_LowLevelVoiceTable[voiceId]; }

    VoiceInfo* GetVoiceInfo(uint32_t voiceId) { return &m_VoiceInfoTable[0][voiceId]; }
    const VoiceInfo* GetVoiceInfo(uint32_t voiceId) const { return &m_VoiceInfoTable[0][voiceId]; }

    void UpdateVoiceInfo();

    int32_t GetAllocatedVirtualVoiceCount() const;
    int32_t GetUnreleasedLowLevelVoiceCount() const;

    VirtualVoiceManager() { Initialize(); };

private:
    static const int32_t VirtualVoiceElementCount{8};

    uint32_t m_VirtualVoiceAllocationTable[VirtualVoiceElementCount];
    uint32_t m_VoiceInfoTableRead;
    LowLevelVoice* m_LowLevelVoiceTable[VirtualVoiceCount];
    VoiceInfo m_VoiceInfoTable[WaveChannelMax][VirtualVoiceCount];
    util::BitFlagSet<VirtualVoiceCount, void> m_VoiceInfoDirtyTable[WaveChannelMax];
};
static_assert(sizeof(VirtualVoiceManager) == 0x4868);

}  // namespace nn::atk::detail
