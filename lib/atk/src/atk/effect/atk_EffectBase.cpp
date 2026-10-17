#include <nn/atk/atk_EffectBase.h>

namespace nn::atk {

EffectBase::EffectBase() = default;

EffectBase::~EffectBase() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] audio::FinalMixType* pFinalMix) {
    return true;
}

bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] audio::SubMixType* pSubMix) {
    return true;
}
#else
bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] OutputMixer* pOutputMixer) {
    return true;
}
#endif

void EffectBase::SetEffectInputOutput([[maybe_unused]] const int8_t* input,
                                      [[maybe_unused]] const int8_t* output,
                                      [[maybe_unused]] int inputCount,
                                      [[maybe_unused]] int outputCount) {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void EffectBase::RemoveEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                              [[maybe_unused]] audio::FinalMixType* pFinalMix) {}

void EffectBase::RemoveEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                              [[maybe_unused]] audio::SubMixType* pSubMix) {}
#else
void EffectBase::RemoveEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                              [[maybe_unused]] OutputMixer* pOutputMixer) {}
#endif

void EffectBase::OnChangeOutputMode() {}

void EffectBase::SetEffectBuffer(void* effectBuffer, size_t effectBufferSize) {
    m_EffectBuffer = effectBuffer;
    m_EffectBufferSize = effectBufferSize;
}

int32_t EffectBase::ConvertChannelModeToInt(ChannelMode channelMode) {
    switch (channelMode) {
    case ChannelMode_1Ch:
        return 1;
    case ChannelMode_2Ch:
        return 2;
    case ChannelMode_4Ch:
        return 4;
    case ChannelMode_6Ch:
        return 6;
    default:
        NN_UNEXPECTED_DEFAULT;
    }
}

void EffectBase::UpdateBuffer([[maybe_unused]] int numChannels, [[maybe_unused]] void** buffer,
                              [[maybe_unused]] size_t bufferSize,
                              [[maybe_unused]] SampleFormat format, [[maybe_unused]] int sampleRate,
                              [[maybe_unused]] OutputMode mode) {}

EffectBase::SampleRate EffectBase::GetSampleRate() const {
    return m_SampleRate;
}

bool EffectBase::SetSampleRate(SampleRate sampleRate) {
    if (m_IsActive)
        return false;

    m_SampleRate = sampleRate;
    return true;
}

}  // namespace nn::atk
