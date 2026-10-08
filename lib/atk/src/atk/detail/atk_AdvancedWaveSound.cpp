#include <nn/atk/detail/atk_AdvancedWaveSound.h>

namespace nn::atk::detail {

AdvancedWaveSound::AdvancedWaveSound(AdvancedWaveSoundInstanceManager& manager)
    : m_InstanceManager(manager) {}

AdvancedWaveSound::~AdvancedWaveSound() = default;

}  // namespace nn::atk::detail
