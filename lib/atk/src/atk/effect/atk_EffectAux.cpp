#include <nn/atk/atk_EffectAux.h>

namespace nn::atk {

void EffectAux::ResetChannelIndex() {
    for (int i{0}; i < ChannelCountMax; ++i)
        m_ChannelSetting[i] = static_cast<ChannelIndex>(i);
}

}  // namespace nn::atk
