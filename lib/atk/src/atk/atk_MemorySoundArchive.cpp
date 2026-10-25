#include <nn/atk/atk_MemorySoundArchive.h>

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

}  // namespace nn::atk
