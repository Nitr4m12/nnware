#include <nn/atk/atk_PlayerHeapDataManager.h>

namespace nn::atk::detail {

PlayerHeapDataManager::PlayerHeapDataManager() = default;

PlayerHeapDataManager::~PlayerHeapDataManager() {
    Finalize();
}

void PlayerHeapDataManager::Finalize() {
    if (m_IsFinalized)
        return;

    m_IsInitialized = false;
    m_IsFinalized = true;
    SetSoundArchive(nullptr);
}

void PlayerHeapDataManager::Initialize(const SoundArchive* arc) {
    if (m_IsInitialized)
        return;

    m_IsInitialized = true;
    m_IsFinalized = false;

    for (int i{0}; i < FileAddressCount; ++i) {
        m_FileAddress[i].address = nullptr;
        m_FileAddress[i].fileId = SoundArchive::InvalidId;
    }

    SetSoundArchive(arc);
}

const void* PlayerHeapDataManager::SetFileAddress(SoundArchive::FileId fileId,
                                                  const void* address) {
    return SetFileAddressToTable(fileId, address);
}

}  // namespace nn::atk::detail
