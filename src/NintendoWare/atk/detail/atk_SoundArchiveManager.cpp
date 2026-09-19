#include <nn/atk/detail/atk_SoundArchiveManager.h>

namespace nn::atk::detail {

SoundArchiveManager::SoundArchiveManager() = default;

SoundArchiveManager::~SoundArchiveManager() = default;

void SoundArchiveManager::Initialize(const SoundArchive* pSoundArchive,
                                     const SoundDataManager* pSoundDataManager) {
    m_pMainSoundArchive = pSoundArchive;
    m_pMainSoundDataManager = pSoundDataManager;
    m_ContainerList.Clear();
    m_pCurrentSoundArchive = m_pMainSoundArchive;
    m_pCurrentSoundDataManager = m_pMainSoundDataManager;
}

}  // namespace nn::atk::detail
