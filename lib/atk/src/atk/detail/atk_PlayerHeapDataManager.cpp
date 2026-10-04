#include <nn/atk/atk_PlayerHeapDataManager.h>

namespace nn::atk::detail {

PlayerHeapDataManager::PlayerHeapDataManager() = default;

void PlayerHeapDataManager::Finalize() {
    if (m_IsFinalized)
        return;

    m_IsInitialized = false;
    m_IsFinalized = true;
    SetSoundArchive(nullptr);
}

}  // namespace nn::atk::detail
