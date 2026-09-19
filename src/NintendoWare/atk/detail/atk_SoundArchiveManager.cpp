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

void SoundArchiveManager::ChangeTargetArchive(const char* soundArchiveName) {
    m_pCurrentSoundArchive = m_pMainSoundArchive;
    m_pCurrentSoundDataManager = m_pMainSoundDataManager;

    if (soundArchiveName == nullptr)
        return;

    for (ContainerList::ConstIterator iterator{m_ContainerList.Begin()};
         iterator != m_ContainerList.End(); ++iterator) {
        if (iterator->IsSameName(soundArchiveName)) {
            m_pCurrentSoundArchive = iterator->GetSoundArchive();
            m_pCurrentSoundDataManager = iterator->GetSoundDataManager();
            return;
        }
    }
}

}  // namespace nn::atk::detail
