#include <nn/atk/atk_SubMix.h>

namespace nn::atk {

SubMix::VolumeData::VolumeData() {
    m_Volume.InitValue(1.0f);
}

SubMix::SubMix() = default;

}  // namespace nn::atk
