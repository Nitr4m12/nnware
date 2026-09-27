#include <nn/atk/atk_SequenceSoundFileReader.h>

namespace nn::atk::detail {

namespace {

const uint32_t SignatureDataBlockSeq{0x41544144};   // DATA
const uint32_t SignatureLabelBlockSeq{0x4c42414c};  // LABL

const uint32_t SupportedFileVersionSeq{0x10000};
const uint32_t CurrentFileVersionSeq{0x20000};

bool IsValidFileHeaderSeq(const void* sequenceFile) {
    const BinaryFileHeader* header{util::ConstBytePtr(sequenceFile).Get<BinaryFileHeader>()};

    bool isSupportedVersion{header->signature == SequenceSoundFileReader::SignatureFile &&
                            header->byteOrder == BinaryFileHeader::ValidByteOrderMark &&
                            header->version >= SupportedFileVersionSeq &&
                            header->version <= CurrentFileVersionSeq};

    return isSupportedVersion;
}

}  // anonymous namespace

SequenceSoundFileReader::SequenceSoundFileReader(const void* sequenceFile) {
    if (!IsValidFileHeaderSeq(sequenceFile))
        return;

    m_pHeader = reinterpret_cast<const SequenceSoundFile::FileHeader*>(sequenceFile);

    const SequenceSoundFile::DataBlock* dataBlock{m_pHeader->GetDataBlock()};
    if (dataBlock->header.kind != SignatureDataBlockSeq)
        return;

    const SequenceSoundFile::LabelBlock* labelBlock{m_pHeader->GetLabelBlock()};
    if (labelBlock->header.kind != SignatureLabelBlockSeq)
        return;

    m_pDataBlockBody = &dataBlock->body;
    m_pLabelBlockBody = &labelBlock->body;
}

const void* SequenceSoundFileReader::GetSequenceData() const {
    return m_pDataBlockBody;
}

bool SequenceSoundFileReader::GetOffsetByLabel(const char* label, uint32_t* offsetPtr) const {
    return m_pLabelBlockBody->GetOffsetByLabel(label, offsetPtr);
}

const char* SequenceSoundFileReader::GetLabelByOffset(uint32_t offset) const {
    return m_pLabelBlockBody->GetLabelByOffset(offset);
}

}  // namespace nn::atk::detail
