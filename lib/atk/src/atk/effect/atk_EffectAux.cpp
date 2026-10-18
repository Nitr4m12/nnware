#include <nn/atk/atk_EffectAux.h>
#include "nn/audio.h"

namespace nn::atk {

EffectAux::EffectAux() {
    ResetChannelIndex();
}

void EffectAux::ResetChannelIndex() {
    for (int i{0}; i < ChannelCountMax; ++i)
        m_ChannelSetting[i] = static_cast<ChannelIndex>(i);
}

EffectAux::~EffectAux() = default;

bool EffectAux::Initialize() {
    return true;
}

void EffectAux::Finalize() {}

void EffectAux::OnChangeOutputMode() {}

size_t EffectAux::GetRequiredMemSize(const audio::AudioRendererParameter& parameter) const {
    size_t bufferSize{
        audio::GetRequiredBufferSizeForAuxSendReturnBuffer(&parameter, m_AudioFrameCount,
                                                           m_ChannelCount),
    };
    return bufferSize * 3;
}

}  // namespace nn::atk
