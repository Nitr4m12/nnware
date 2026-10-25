#include <nn/atk/atk_SoundArchiveFileReader.h>

namespace nn::atk::detail {

namespace {

const uint32_t SupportedFileVersionSar{NN_MAKE_VER(1, 0, 0)};
const uint32_t CurrentFileVersionSar{NN_MAKE_VER(2, 5, 0)};

const uint32_t StrmsendSupportedVersionSar{NN_MAKE_VER(2, 1, 0)};
const uint32_t FilterSupportedVersionSar{NN_MAKE_VER(2, 1, 0)};
const uint32_t StrmprefetchSupportedVersionSar{NN_MAKE_VER(2, 2, 0)};

bool IsValidFileHeaderSar(const void* soundArchiveData) {
    const BinaryFileHeader& header{*static_cast<const BinaryFileHeader*>(soundArchiveData)};

    bool isSupportedVersion{
        header.signature == SoundArchiveFileReader::SignatureFile &&
            header.byteOrder == BinaryFileHeader::ValidByteOrderMark &&
            header.version >= SupportedFileVersionSar && header.version <= CurrentFileVersionSar,
    };

    return isSupportedVersion;
}

const SoundArchiveFile::StreamTrackInfo*
GetStreamTrackInfo(uint32_t index, SoundArchiveFile::StreamTrackInfoTable& table) {}

}  // anonymous namespace

SoundArchiveFileReader::SoundArchiveFileReader() = default;

void SoundArchiveFileReader::Initialize(const void* soundArchiveData) {
    if (IsValidFileHeaderSar(soundArchiveData))
        m_Header = *static_cast<const SoundArchiveFile::FileHeader*>(soundArchiveData);
}

void SoundArchiveFileReader::Finalize() {
    m_pStringBlockBody = nullptr;
    m_pInfoBlockBody = nullptr;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_pFileBlock = nullptr;
#endif
}

bool SoundArchiveFileReader::IsStreamSendAvailable() const {
    return m_Header.version >= StrmsendSupportedVersionSar;
}

bool SoundArchiveFileReader::IsFilterSupportedVersion() const {
    return m_Header.version >= FilterSupportedVersionSar;
}

bool SoundArchiveFileReader::IsStreamPrefetchAvailable() const {
    return m_Header.version >= StrmprefetchSupportedVersionSar;
}

void SoundArchiveFileReader::SetStringBlock(const void* stringBlock) {
    m_pStringBlockBody = &static_cast<const SoundArchiveFile::StringBlock*>(stringBlock)->body;
}

void SoundArchiveFileReader::SetInfoBlock(const void* infoBlock) {
    m_pInfoBlockBody = &static_cast<const SoundArchiveFile::InfoBlock*>(infoBlock)->body;
}

uint32_t SoundArchiveFileReader::GetStringCount() const {
    return m_pStringBlockBody->GetStringCount();
}

const char* SoundArchiveFileReader::GetString(SoundArchive::StringId stringId) const {
    return m_pStringBlockBody->GetString(stringId);
}

void SoundArchiveFileReader::DumpTree() const {
    m_pStringBlockBody->DumpTree();
}

SoundArchive::ItemId SoundArchiveFileReader::GetItemId(const char* pStr) const {
    if (m_pStringBlockBody == nullptr)
        return SoundArchive::InvalidId;

    return m_pStringBlockBody->GetItemId(pStr);
}

const char* SoundArchiveFileReader::GetItemLabel(SoundArchive::ItemId id) const {
    if (m_pStringBlockBody == nullptr)
        return nullptr;

    SoundArchive::StringId stringId{m_pInfoBlockBody->GetItemStringId(id)};
    if (stringId == SoundArchive::InvalidId)
        return nullptr;

    return m_pStringBlockBody->GetString(stringId);
}

}  // namespace nn::atk::detail
