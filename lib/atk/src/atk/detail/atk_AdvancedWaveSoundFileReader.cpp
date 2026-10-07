#include <nn/atk/detail/atk_AdvancedWaveSoundFileReader.h>

namespace nn::atk::detail {

namespace {

bool IsValidBinaryFileHeader(const void* file, const char* signatureString,
                             uint32_t minimumSupportedFileVersion,
                             uint32_t currentSupportedFileVersion) {
    const util::BinaryFileHeader& fileHeader{*static_cast<const util::BinaryFileHeader*>(file)};

    bool isValidSignature{fileHeader.signature.IsValid(signatureString)};
    bool isValidByteOrderMark{
        fileHeader._byteOrderMark == util::ByteOrderMark::ByteOrderMark_Normal,
    };
    bool isSupportedVersion{
        fileHeader.version.GetPacked() >= minimumSupportedFileVersion &&
            fileHeader.version.GetPacked() <= currentSupportedFileVersion,
    };

    return isValidSignature && isValidByteOrderMark && isSupportedVersion;
}

bool IsValidBinaryBlock(const void* binaryblock, const char* signatureString) {
    const util::BinaryBlockHeader& blockHeader{
        *static_cast<const util::BinaryBlockHeader*>(binaryblock),
    };

    bool isValidSignature{blockHeader.signature.IsValid(signatureString)};

    return isValidSignature;
}

const char SignatureForFileAwsd[]{"BAWSD   "};
const char SignatureForInfoBlockAwsd[]{"INFO"};

const uint32_t MinimumSupportedFileVersion{0x10000};
const uint32_t CurrentSupportedFileVersion{0x10000};

}  // anonymous namespace

AdvancedWaveSoundFileReader::AdvancedWaveSoundFileReader(const void* pFile) {
    if (!IsValidBinaryFileHeader(pFile, SignatureForFileAwsd, MinimumSupportedFileVersion,
                                 CurrentSupportedFileVersion))
        return;

    const AdvancedWaveSoundFile& pAdvancedWaveSoundFile{
        *static_cast<const AdvancedWaveSoundFile*>(pFile),
    };

    const AdvancedWaveSoundFile::InfoBlock* pInfoBlock{pAdvancedWaveSoundFile.GetBlock()};

    if (pInfoBlock != nullptr && IsValidBinaryBlock(pInfoBlock, SignatureForInfoBlockAwsd))
        m_pInfoBlockBody = &pInfoBlock->body;
}

}  // namespace nn::atk::detail
