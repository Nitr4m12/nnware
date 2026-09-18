#include <nn/atk/atk_SoundDataManager.h>

#include <nn/atk/atk_DriverCommand.h>

namespace nn::atk {

SoundDataManager::SoundDataManager() = default;

SoundDataManager::~SoundDataManager() = default;

size_t SoundDataManager::GetRequiredMemSize(const SoundArchive* arc) const {
    size_t size{0};
    size += arc->detail_GetFileCount() * sizeof(FileAddress) + 4;
    size = util::align_up(size, BufferAlignSize);

    return size;
}

bool SoundDataManager::Initialize(const SoundArchive* pArchive, void* buffer, size_t size) {
    void* endp{util::BytePtr(buffer, static_cast<ptrdiff_t>(size)).Get()};
    void* buf{buffer};

    if (!CreateTables(&buf, pArchive, endp))
        return false;

    SetSoundArchive(pArchive);
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<detail::DriverCommandDisposeCallback>()};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    if (command == nullptr)
        return false;
#endif

    command->id = detail::DriverCommandId_RegistDisposeCallback;
    command->callback = this;
    cmdmgr.PushCommand(command);
    return true;
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
