#include <nn/atk/atk_Voice.h>

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

}  // namespace nn::atk::detail
