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

const void* PlayerHeapDataManager::GetFileAddress(SoundArchive::FileId fileId) const {
    return GetFileAddressFromTable(fileId);
}

void PlayerHeapDataManager::InvalidateData([[maybe_unused]] const void* start,
                                           [[maybe_unused]] const void* end) {
    for (int i{0}; i < FileAddressCount; ++i) {
        m_FileAddress[i].fileId = SoundArchive::InvalidId;
        m_FileAddress[i].address = nullptr;
    }
}

const void* PlayerHeapDataManager::SetFileAddressToTable(SoundArchive::FileId fileId,
                                                         const void* address) {
    for (int i{0}; i < FileAddressCount; ++i) {
        if (m_FileAddress[i].fileId == fileId) {
            const void* prev{m_FileAddress[i].address};
            m_FileAddress[i].address = address;
            return prev;
        }
    }

    for (int i{0}; i < FileAddressCount; ++i) {
        if (m_FileAddress[i].fileId == SoundArchive::InvalidId) {
            m_FileAddress[i].fileId = fileId;
            m_FileAddress[i].address = address;
            return nullptr;
        }
    }

    return nullptr;
}

const void* PlayerHeapDataManager::GetFileAddressFromTable(SoundArchive::FileId fileId) const {
    for (int i{0}; i < FileAddressCount; ++i) {
        if (m_FileAddress[i].fileId == fileId)
            return m_FileAddress[i].address;
    }

    return nullptr;
}

const void* PlayerHeapDataManager::GetFileAddressImpl(SoundArchive::FileId fileId) const {
    return GetFileAddressFromTable(fileId);
}

}  // namespace nn::atk::detail
