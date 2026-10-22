#include <nn/atk/atk_Voice.h>

#include <nn/util/util_BitUtil.h>

#include <nn/atk/atk_HardwareManager.h>

namespace nn::atk::detail {

namespace {

const BiquadFilterCoefficients BiquadFilterCoefZero{.b0 = 0, .b1 = 0, .b2 = 0, .a1 = 0, .a2 = 0};

const OutputMix DefaultTvMix{
    {
        1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    },
};

}  // anonymous namespace

Voice::Voice() = default;

Voice::~Voice() = default;

void Voice::Initialize(uint32_t priority) {
    m_State = VoiceState_Stop;
    m_Priority = priority;

    m_VoiceParam.Initialize();
    m_VoiceParam.m_TvMix = DefaultTvMix;
    m_VoiceParam.m_MonoFilterFlag = false;
    m_VoiceParam.m_BiquadFilterFlag = false;
    m_VoiceParam.m_BiquadFilterCoefficients = BiquadFilterCoefZero;
    m_VoiceParam.m_MonoFilterCutoff = 0;
    m_VoiceParam.m_InterpolationType = 0;

    m_SampleFormat = SampleFormat_PcmS16;
    m_SampleRate = 32000;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = 0;
#else
    m_pOutputReceiver = nullptr;
#endif
    m_PlayPosition = 0;
}

bool Voice::IsAvailable() const {
    if (m_pLowLevelVoice == nullptr)
        return false;

    return m_pLowLevelVoice->IsAvailable();
}

bool Voice::AllocVoice(uint32_t priority) {
    LowLevelVoice* lowLevelVoice{
        driver::HardwareManager::GetInstance().GetLowLevelVoiceAllocator().AllocVoice(),
    };
    if (lowLevelVoice == nullptr)
        return false;

    lowLevelVoice->SetPriority(priority);
    lowLevelVoice->SetVoice(this);
    m_pLowLevelVoice = lowLevelVoice;
    Initialize(priority);
    return true;
}

void Voice::Free() {
    m_State = VoiceState_Stop;

    if (m_pLowLevelVoice == nullptr)
        return;

    driver::HardwareManager::GetInstance().GetLowLevelVoiceAllocator().FreeVoice(m_pLowLevelVoice);
    m_pLowLevelVoice = nullptr;
}

void Voice::SetPriority(uint32_t priority) {
    m_Priority = priority;

    if (m_pLowLevelVoice == nullptr)
        return;

    m_pLowLevelVoice->SetPriority(priority);
}

void Voice::SetState(VoiceState state) {
    m_State = state;

    if (m_pLowLevelVoice == nullptr)
        return;

    switch (m_State) {
    case VoiceState_Play:
        m_pLowLevelVoice->SetSampleRate(m_SampleRate);
        m_pLowLevelVoice->SetSampleFormat(m_SampleFormat);
        m_pLowLevelVoice->SetAdpcmParam(m_AdpcmParam);
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        m_pLowLevelVoice->SetSubMixIndex(m_SubMixIndex);
#else
        m_pLowLevelVoice->SetOutputReceiver(m_pOutputReceiver);
#endif
        break;
    case VoiceState_Stop:
        m_pLowLevelVoice->FreeAllWaveBuffer();
        break;
    case VoiceState_Pause:
        break;
    }

    m_pLowLevelVoice->SetState(state);
}

void Voice::AppendWaveBuffer(WaveBuffer* waveBuffer) {
    if (m_pLowLevelVoice == nullptr)
        return;

    m_pLowLevelVoice->AppendWaveBuffer(waveBuffer);
}

void Voice::FreeAllWaveBuffer() {}

void Voice::UpdateParam() {
    if (m_pLowLevelVoice == nullptr)
        return;

    m_pLowLevelVoice->SetVoiceParam(m_VoiceParam);
}

position_t Voice::GetPlayPosition() const {
    if (m_pLowLevelVoice == nullptr)
        return 0;

    return m_pLowLevelVoice->GetPlayPosition();
}

void Voice::SetMonoFilter(bool enable, uint16_t cutoff) {
    m_VoiceParam.m_MonoFilterFlag = enable;
    if (enable)
        m_VoiceParam.m_MonoFilterCutoff = cutoff;
}

void Voice::SetBiquadFilter(bool enable, const BiquadFilterCoefficients* coef) {
    m_VoiceParam.m_BiquadFilterFlag = enable;
    if (enable)
        m_VoiceParam.m_BiquadFilterCoefficients = *coef;
}

VirtualVoiceManager& VirtualVoiceManager::GetInstance() {
    static VirtualVoiceManager instance;
    return instance;
}

void VirtualVoiceManager::Initialize() {
    m_VoiceInfoTableRead = 0;

    m_VoiceInfoDirtyTable[0].Set();
    m_VoiceInfoDirtyTable[1].Set();

    for (int i{0}; i < VirtualVoiceElementCount; ++i)
        m_VirtualVoiceAllocationTable[i] = InvalidVoiceId;

    for (uint32_t i{0}; i < VirtualVoiceCount; ++i) {
        m_LowLevelVoiceTable[i] = nullptr;
        m_VoiceInfoTable[0][i].voiceState = VoiceState_Stop;
        m_VoiceInfoTable[1][i].voiceState = VoiceState_Stop;
    }
}

// TODO: uint32_t VirtualVoiceManager::AllocVirtualVoice

void VirtualVoiceManager::FreeVirtualVoice(uint32_t voiceId) {
    uint32_t i{voiceId >> 5};
    uint32_t shift{static_cast<uint32_t>(1 << (voiceId & 31))};
    m_VirtualVoiceAllocationTable[i] = m_VirtualVoiceAllocationTable[i] | shift;
}

void VirtualVoiceManager::UpdateVoiceInfo() {
    uint32_t writeIndex{m_VoiceInfoTableRead == 0};

    for (uint32_t i{0}; i < VirtualVoiceCount; ++i) {
        VoiceInfo* voiceInfo{&m_VoiceInfoTable[writeIndex][i]};
        if (m_LowLevelVoiceTable[i] != nullptr) {
            m_LowLevelVoiceTable[i]->UpdateVoiceInfo(voiceInfo);
            m_VoiceInfoDirtyTable[writeIndex].Set(i);
        } else {
            if (m_VoiceInfoDirtyTable[writeIndex].Test(i)) {
                voiceInfo->voiceState = VoiceState_Stop;
                voiceInfo->waveBufferStatus = WaveBuffer::Status_Free;
                voiceInfo->userId = nullptr;
                m_VoiceInfoDirtyTable[writeIndex].Reset(i);
            }
        }
    }

    m_VoiceInfoTableRead = writeIndex;
}

int32_t VirtualVoiceManager::GetAllocatedVirtualVoiceCount() const {
    int32_t allocatedCount{0};

    for (int i{0}; i < VirtualVoiceElementCount; ++i) {
        uint32_t allocatedFlag{~m_VirtualVoiceAllocationTable[i]};
        allocatedCount += util::popcount(allocatedFlag);
    }

    return allocatedCount;
}

int32_t VirtualVoiceManager::GetUnreleasedLowLevelVoiceCount() const {
    int32_t count{0};

    for (int i{0}; i < VirtualVoiceCount; ++i) {
        if (m_LowLevelVoiceTable[i] != nullptr)
            ++count;
    }

    return count;
}

}  // namespace nn::atk::detail
