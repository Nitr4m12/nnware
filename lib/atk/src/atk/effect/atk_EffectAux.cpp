#include <nn/atk/atk_EffectAux.h>

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

}  // namespace nn::atk
