#include <nn/atk/detail/atk_AdvancedWaveSound.h>

namespace nn::atk::detail {

AdvancedWaveSound::AdvancedWaveSound(AdvancedWaveSoundInstanceManager& manager)
    : m_InstanceManager(manager) {}

AdvancedWaveSound::~AdvancedWaveSound() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool AdvancedWaveSound::Initialize()
#else
bool AdvancedWaveSound::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool result{BasicSound::Initialize()};
#else
    bool result{BasicSound::Initialize(pOutputReceiver)};
#endif

    if (!result)
        return false;

    m_pTempSpecialHandle = nullptr;
    m_IsInitialized = true;
    return true;
}

void AdvancedWaveSound::Finalize() {
    if (m_IsInitialized) {
        m_IsInitialized = false;
        m_pTempSpecialHandle = nullptr;
        BasicSound::Finalize();
        m_InstanceManager.Free(this);
    }
}

}  // namespace nn::atk::detail
