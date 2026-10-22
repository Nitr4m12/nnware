#include <nn/atk/atk_Voice.h>

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
    m_Priority = priority;
    m_State = VoiceState_Stop;

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

}  // namespace nn::atk::detail
