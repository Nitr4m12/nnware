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

}  // namespace nn::atk::detail
