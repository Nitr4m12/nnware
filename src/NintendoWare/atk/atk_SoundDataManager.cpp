#include <nn/atk/atk_SoundDataManager.h>

namespace nn::atk {

SoundDataManager::SoundDataManager() = default;

SoundDataManager::~SoundDataManager() = default;

size_t SoundDataManager::GetRequiredMemSize(const SoundArchive* arc) const {
    size_t size{0};
    size += arc->detail_GetFileCount() * sizeof(FileAddress) + 4;
    size = util::align_up(size, BufferAlignSize);

    return size;
}

bool SoundDataManager::CreateTables(void** pOutBuffer, const SoundArchive* pArchive,
                                    void* endAddress) {
    size_t fileTableSize{pArchive->detail_GetFileCount()};
    fileTableSize *= sizeof(FileAddress);
    fileTableSize += 4;

    void* ep{util::BytePtr(*pOutBuffer, static_cast<ptrdiff_t>(fileTableSize))
                 .AlignUp(BufferAlignSize)
                 .Get()};
    if (util::BytePtr(endAddress).Distance(ep) > 0)
        return false;

    m_pFileTable = reinterpret_cast<FileTable*>(*pOutBuffer);
    *pOutBuffer = ep;

    m_pFileTable->count = pArchive->detail_GetFileCount();

    for (u32 i{0}; i < m_pFileTable->count; ++i)
        m_pFileTable->item[i].address = nullptr;

    return true;
}

}  // namespace nn::atk
