#include <nn/atk/atk_SubMix.h>

namespace nn::atk {

SubMix::SubMixParam::SubMixParam() {
    for (int i{0}; i < BusCountMax; ++i)
        m_IsSoundSendClampEnabledArray[i] = true;
}

void SubMix::SubMixParam::SetSrcBusCount(int32_t count) {
    m_SrcBusCount = count;
}

SubMix::VolumeData::VolumeData() {
    m_Volume.InitValue(1.0f);
}

SubMix::SubMix() = default;

}  // namespace nn::atk
