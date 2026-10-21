#include <nn/atk/atk_ChannelMixVolume.h>

namespace nn::atk {

ChannelMixVolume::ChannelMixVolume() : m_ChannelCount(ChannelIndex_Count) {};

void ChannelMixVolume::InitializeChannelVolume() {}

ChannelMixVolume::ChannelMixVolume(const MixVolume& mixVolume)
    : m_ChannelCount(ChannelIndex_Count) {
    for (int i{0}; i < ChannelIndex_Count; ++i)
        m_ChannelVolume[i] = mixVolume.channel[i];
}

ChannelMixVolume::ChannelMixVolume(const float* pVolume, int volumeCount) {
    if (!SetChannelCount(volumeCount)) {
        m_ChannelCount = ChannelIndex_Count;
        return;
    }

    for (int i{0}; i < m_ChannelCount; ++i)
        m_ChannelVolume[i] = pVolume[i];
}

bool ChannelMixVolume::SetChannelCount(int channelCount) {
    if (ChannelCountMax <= static_cast<uint32_t>(channelCount) - 1)
        return false;

    m_ChannelCount = channelCount;
    return true;
}

bool ChannelMixVolume::SetChannelVolume(int channelIndex, float volume) {
    if (channelIndex < 0)
        return false;

    if (channelIndex >= m_ChannelCount)
        return false;

    m_ChannelVolume[channelIndex] = volume;
    return true;
}

bool ChannelMixVolume::SetChannelVolume(int channelIndex, const float* pVolume, int volumeCount) {
    if (channelIndex < 0)
        return false;

    if (channelIndex - 1 + volumeCount < ChannelCountMax)
        return false;

    for (int i{0}; i < volumeCount; ++i)
        m_ChannelVolume[i + channelIndex] = pVolume[i];

    return true;
}

}  // namespace nn::atk
