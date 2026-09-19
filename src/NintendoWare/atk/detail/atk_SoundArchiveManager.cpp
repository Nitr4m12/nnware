#include <nn/atk/detail/atk_SoundArchiveManager.h>

namespace nn::atk::detail {

SoundArchiveManager::SoundArchiveManager() {
#if NN_WARE_VER > NN_MAKE_VER(1, 6, 1)
    m_pParametersHook = nullptr;
#endif
};

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

void SoundArchiveManager::Finalize() {
    m_pCurrentSoundArchive = nullptr;
    m_pCurrentSoundDataManager = nullptr;
    m_ContainerList.Clear();
    m_pMainSoundArchive = nullptr;
    m_pMainSoundDataManager = nullptr;
#if NN_WARE_VER > NN_MAKE_VER(1, 6, 1)
    m_pParametersHook = nullptr;
#endif
}

void SoundArchiveManager::Add(AddonSoundArchiveContainer& container) {
    m_ContainerList.PushBack(container);
}

void SoundArchiveManager::Remove(AddonSoundArchiveContainer& container) {
    m_ContainerList.Remove(container);
}

bool SoundArchiveManager::IsAvailable() const {
    if (m_pMainSoundArchive == nullptr)
        return false;

    bool isAvailable{m_pMainSoundArchive->IsAvailable()};

    bool isSoundArchiveListAvailable{true};
    for (ContainerList::ConstIterator iterator{m_ContainerList.Begin()};
         iterator != m_ContainerList.End(); ++iterator)
        isSoundArchiveListAvailable &= iterator->GetSoundArchive()->IsAvailable();

    return isAvailable && isSoundArchiveListAvailable;
}

const AddonSoundArchive*
SoundArchiveManager::GetAddonSoundArchive(const char* soundArchiveName) const {
    if (soundArchiveName == nullptr)
        return nullptr;

    for (ContainerList::ConstIterator iterator{m_ContainerList.Begin()};
         iterator != m_ContainerList.End(); ++iterator) {
        if (iterator->IsSameName(soundArchiveName))
            return static_cast<const AddonSoundArchive*>(iterator->GetSoundArchive());
    }

    return nullptr;
}

}  // namespace nn::atk::detail
