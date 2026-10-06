#pragma once

#include <nn/util/util_BitFlagSet.h>

#include <nn/atk/atk_LowLevelVoice.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

class Voice {
public:
    constexpr static int32_t PriorityMin = 0;
    constexpr static int32_t PriorityMax = 255;
    constexpr static int32_t PriorityNoDrop = 255;

    Voice();
    ~Voice();

    void Initialize(uint32_t);

    bool IsAvailable() const;

    bool AllocVoice(uint32_t priority);
    void Free();

    void SetPriority(uint32_t priority);
    void SetState(VoiceState state);

    void AppendWaveBuffer(WaveBuffer* waveBuffer);

    void FreeAllWaveBuffer();

    void UpdateParam();

    position_t GetPlayPosition() const;

    bool SetMonoFilter(bool enable, uint16_t cutoff);
    void SetBiquadFilter(bool enable, const BiquadFilterCoefficients* coef);

    void UpdateVoiceStatus();

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (m_pLowLevelVoice != nullptr) {
            audio::NodeId voiceNodeId{m_pLowLevelVoice->GetNodeId()};
            for (uint32_t voiceIndex{0}; voiceIndex < profile.rendererVoiceCount; ++voiceIndex) {
                if (profile._voiceIdTable[voiceIndex] == voiceNodeId)
                    return profile._voiceProcessTable[voiceIndex].end -
                           profile._voiceProcessTable[voiceIndex].begin;
            }
        }

        return 0;
    }

private:
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
    LowLevelVoice* m_pLowLevelVoice;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(Voice) == 0xd8);
#else
static_assert(sizeof(Voice) == 0xe0);
#endif

class VirtualVoiceManager : Util::Singleton<VirtualVoiceManager> {
public:
    constexpr static int32_t InvalidVoiceId = -1;

    constexpr static uint32_t VirtualVoiceCount = 256;
    constexpr static uint32_t VirtualVoiceElementCount = 8;

    void Initialize();

    bool AllocVirtualVoice();
    void FreeVirtualVoice(uint32_t);

    void UpdateVoiceInfo();

    int32_t GetAllocatedVirtualVoiceCount() const;
    int32_t GetUnreleasedLowLevelVoiceCount() const;

private:
    uint32_t m_VirtualVoiceAllocationTable[VirtualVoiceElementCount];
    uint32_t m_VoiceInfoTableRead;
    LowLevelVoice* m_LowLevelVoiceTable[VirtualVoiceCount];
    VoiceInfo m_VoiceInfoTable[2][VirtualVoiceCount];
    util::BitFlagSet<VirtualVoiceCount, void> m_VoiceInfoDirtyTable[2];
};
static_assert(sizeof(VirtualVoiceManager) == 0x4868);

}  // namespace nn::atk::detail
