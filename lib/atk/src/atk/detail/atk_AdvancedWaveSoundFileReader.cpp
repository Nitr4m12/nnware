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

int32_t AdvancedWaveSoundFileReader::GetWaveSoundTrackCount() const {
    return m_pInfoBlockBody->GetTrackCount();
}

int32_t AdvancedWaveSoundFileReader::GetWaveSoundClipCount(int32_t trackIndex) const {
    const AdvancedWaveSoundFile::WaveSoundTrack& waveSoundTrack{
        m_pInfoBlockBody->GetWaveSoundTrack(trackIndex),
    };

    return waveSoundTrack.GetClipCount();
}

bool AdvancedWaveSoundFileReader::ReadWaveSoundTrackInfoSet(
    AdvancedWaveSoundTrackInfoSet* pTrackInfoSet) {
    pTrackInfoSet->waveSoundTrackCount = GetWaveSoundTrackCount();
    for (int trackIndex{0}; trackIndex < pTrackInfoSet->waveSoundTrackCount; ++trackIndex) {
        AdvancedWaveSoundTrackInfo& trackInfo{pTrackInfoSet->waveSoundTrackInfo[trackIndex]};

        const AdvancedWaveSoundFile::WaveSoundTrack& waveSoundTrack{
            m_pInfoBlockBody->GetWaveSoundTrack(trackIndex),
        };

        trackInfo.waveSoundClipCount = waveSoundTrack.GetClipCount();
        for (int index{0}; index < trackInfo.waveSoundClipCount; ++index) {
            const AdvancedWaveSoundFile::WaveSoundClip& waveSoundClip{
                waveSoundTrack.GetWaveSoundClip(index),
            };

            AdvancedWaveSoundClipInfo& waveSoundClipInfo{trackInfo.waveSoundClipInfo[index]};

            waveSoundClipInfo.waveIndex = waveSoundClip.waveIndex;
            waveSoundClipInfo.position = waveSoundClip.position;
            waveSoundClipInfo.duration = waveSoundClip.duration;
            waveSoundClipInfo.startOffset = waveSoundClip.startOffset;
            waveSoundClipInfo.pitch = waveSoundClip.pitch;
            waveSoundClipInfo.volume = waveSoundClip.volume;
            waveSoundClipInfo.pan = waveSoundClip.pan;
        }
    }
    return true;
}

}  // namespace nn::atk::detail
