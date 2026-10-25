#include <nn/atk/atk_MemorySoundArchive.h>

#include <nn/atk/atk_GroupFileReader.h>
#include <nn/atk/atk_SoundArchiveFileReader.h>

namespace nn::atk {

MemorySoundArchive::MemorySoundArchive() = default;

MemorySoundArchive::~MemorySoundArchive() = default;

bool MemorySoundArchive::Initialize(const void* buffer) {
    m_ArchiveReader.Initialize(buffer);
    SoundArchive::Initialize(&m_ArchiveReader);

    m_ArchiveReader.SetInfoBlock(
        util::ConstBytePtr(buffer, m_ArchiveReader.GetInfoBlockOffset()).Get());

    int32_t stringBlockOffset{m_ArchiveReader.GetStringBlockOffset()};
    if (m_ArchiveReader.GetStringBlockSize() != detail::SoundArchiveFileReader::InvalidSize) {
        m_ArchiveReader.SetStringBlock(util::ConstBytePtr(buffer, stringBlockOffset).Get());
    }

    m_pBuffer = buffer;
    return true;
}

void MemorySoundArchive::Finalize() {
    m_pBuffer = nullptr;
    m_ArchiveReader.Finalize();
    SoundArchive::Finalize();
}

const void* MemorySoundArchive::detail_GetFileAddress(FileId fileId) const {
    SoundArchive::FileInfo fileInfo;
    if (detail_ReadFileInfo(fileId, &fileInfo)) {
        uint32_t offsetFromFileBlockHead{fileInfo.offsetFromFileBlockHead};
        if (fileInfo.offsetFromFileBlockHead != detail::SoundArchiveFileReader::InvalidOffset)
            return util::ConstBytePtr(m_pBuffer, m_ArchiveReader.GetFileBlockOffset() +
                                                     offsetFromFileBlockHead)
                .Get();
    }

    uint32_t groupCount{GetGroupCount()};
    for (uint32_t i{0}; i < groupCount; ++i) {
        SoundArchive::GroupInfo groupInfo;
        if (!ReadGroupInfo(&groupInfo, detail::Util::GetMaskedItemId(i, detail::ItemType_Group)))
            continue;

        SoundArchive::FileInfo groupFileInfo;
        if (!detail_ReadFileInfo(groupInfo.fileId, &groupFileInfo))
            continue;

        const void* groupFile{
            util::ConstBytePtr(m_pBuffer, groupFileInfo.offsetFromFileBlockHead +
                                              m_ArchiveReader.GetFileBlockOffset())
                .Get(),
        };

        detail::GroupFileReader reader{groupFile};
        uint32_t groupItemCount{reader.GetGroupItemCount()};

        for (uint32_t j{0}; j < groupItemCount; ++j) {
            detail::GroupItemLocationInfo info;
            if (reader.ReadGroupItemLocationInfo(&info, j) && info.fileId == fileId &&
                info.address != nullptr)
                return info.address;
        }
    }

    return nullptr;
}

}  // namespace nn::atk
