#include <nn/atk/atk_ChannelMixVolume.h>

namespace nn::atk {

ChannelMixVolume::ChannelMixVolume() = default;

void ChannelMixVolume::InitializeChannelVolume() {}

ChannelMixVolume::ChannelMixVolume(const MixVolume& mixVolume) {
    for (int i{0}; i < ChannelIndex_Count; ++i)
        m_ChannelVolume[i] = mixVolume.channel[i];
}

bool ChannelMixVolume::SetChannelCount(int channelCount) {
    if (ChannelCountMax <= static_cast<uint32_t>(channelCount) - 1)
        return false;

    m_ChannelCount = channelCount;
    return true;
}

}  // namespace nn::atk
