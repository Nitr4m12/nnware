#include <nn/atk/atk_SubMix.h>

namespace nn::atk {

SubMix::SubMixParam::SubMixParam() {
    for (int i{0}; i < BusCountMax; ++i)
        m_IsSoundSendClampEnabledArray[i] = true;
}

void SubMix::SubMixParam::SetSrcBusCount(int32_t count) {
    m_SrcBusCount = count;
}

int32_t SubMix::SubMixParam::GetSrcBusCount() const {
    return m_SrcBusCount;
}

void SubMix::SubMixParam::SetSrcChannelCount(int32_t count) {
    m_SrcChannelCount = count;
}

int32_t SubMix::SubMixParam::GetSrcChannelCount() const {
    return m_SrcChannelCount;
}

void SubMix::SubMixParam::SetDstBusCount(int32_t count) {
    m_DstBusCount = count;
}

SubMix::VolumeData::VolumeData() {
    m_Volume.InitValue(1.0f);
}

SubMix::SubMix() = default;

}  // namespace nn::atk
