#include <nn/atk/atk_ChannelMixVolume.h>

namespace nn::atk {

ChannelMixVolume::ChannelMixVolume() = default;

void ChannelMixVolume::InitializeChannelVolume() {}

ChannelMixVolume::ChannelMixVolume(const MixVolume& mixVolume) {
    for (int i{0}; i < ChannelIndex_Count; ++i)
        m_ChannelVolume[i] = mixVolume.channel[i];
}

}  // namespace nn::atk
