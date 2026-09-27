#include <nn/atk/atk_WaveArchiveFileReader.h>

namespace nn::atk::detail {

const uint32_t WaveArchiveFileReader::SignatureFile{0x52415746};       // FWAR
const uint32_t WaveArchiveFileReader::SignatureWarcTable{0x54415746};  // FWAT

namespace {

const uint32_t SupportedFileVersionWar{0x10000};
const uint32_t CurrentFileVersionWar{0x10000};

bool IsValidFileHeaderWar(const void* waveArchiveData) {
    const BinaryFileHeader& header{*util::ConstBytePtr(waveArchiveData).Get<BinaryFileHeader>()};

    bool isSupportedVersion{header.signature == WaveArchiveFileReader::SignatureFile &&
                            header.byteOrder == BinaryFileHeader::ValidByteOrderMark &&
                            header.version >= SupportedFileVersionWar &&
                            header.version <= CurrentFileVersionWar};

    return isSupportedVersion;
}

}  // anonymous namespace

WaveArchiveFileReader::WaveArchiveFileReader() = default;

WaveArchiveFileReader::WaveArchiveFileReader(const void* pWaveArchiveFile, bool isIndividual) {
    Initialize(pWaveArchiveFile, isIndividual);
}

void WaveArchiveFileReader::Initialize(const void* pWaveArchiveFile, bool isIndividual) {
    if (pWaveArchiveFile == nullptr || !IsValidFileHeaderWar(pWaveArchiveFile))
        return;

    m_pHeader = reinterpret_cast<const WaveArchiveFile::FileHeader*>(pWaveArchiveFile);

    m_pInfoBlockBody = &m_pHeader->GetInfoBlock()->body;
    m_IsInitialized = true;

    m_pLoadTable = nullptr;

    if (!isIndividual || !HasIndividualLoadTable())
        return;

    m_pLoadTable =
        util::BytePtr(const_cast<void*>(pWaveArchiveFile))
            .Advance(static_cast<ptrdiff_t>(m_pHeader->GetFileBlockOffset() +
                                            sizeof(WaveArchiveFileReader::SignatureWarcTable)))
            .Get<IndividualLoadTable>();
}

void WaveArchiveFileReader::Finalize() {
    if (m_IsInitialized) {
        m_pHeader = nullptr;
        m_pInfoBlockBody = nullptr;
        m_pLoadTable = nullptr;
        m_IsInitialized = false;
    }
}

void WaveArchiveFileReader::InitializeFileTable() {
    for (uint32_t i{0}; i < GetWaveFileCount(); ++i)
        m_pLoadTable->waveFile[i] = nullptr;
}

uint32_t WaveArchiveFileReader::GetWaveFileCount() const {
    if (!m_IsInitialized)
        return 0;

    return m_pInfoBlockBody->GetWaveFileCount();
}

uint32_t WaveArchiveFileReader::GetWaveFileSize(uint32_t waveIndex) const {
    if (!m_IsInitialized)
        return 0;

    return m_pInfoBlockBody->GetSize(waveIndex);
}

uint32_t WaveArchiveFileReader::GetWaveFileOffsetFromFileHead(uint32_t waveIndex) const {
    uint32_t result{0};

    if (m_IsInitialized)
        result = m_pHeader->GetFileBlockOffset() + offsetof(WaveArchiveFile::FileBlock, body) +
                 m_pInfoBlockBody->GetOffsetFromFileBlockBody(waveIndex);

    return result;
}

const void* WaveArchiveFileReader::GetWaveFile(uint32_t waveIndex) const {
    if (!m_IsInitialized)
        return nullptr;

    if (waveIndex >= m_pInfoBlockBody->GetWaveFileCount())
        return nullptr;

    if (m_pLoadTable != nullptr)
        return GetWaveFileForIndividual(waveIndex);

    return GetWaveFileForWhole(waveIndex);
}

const void* WaveArchiveFileReader::SetWaveFile(uint32_t waveIndex, const void* pWaveFile) {
    if (!m_IsInitialized)
        return nullptr;

    if (m_pLoadTable == nullptr)
        return nullptr;

    if (waveIndex >= m_pInfoBlockBody->GetWaveFileCount())
        return nullptr;

    const void* preAddress{GetWaveFileForIndividual(waveIndex)};
    m_pLoadTable->waveFile[waveIndex] = pWaveFile;

    return preAddress;
}

bool WaveArchiveFileReader::HasIndividualLoadTable() const {
    if (!m_IsInitialized)
        return false;

    const uint32_t* signature{
        util::ConstBytePtr(m_pHeader, m_pHeader->GetFileBlockOffset()).Get<uint32_t>()};

    return *signature == SignatureWarcTable;
}

}  // namespace nn::atk::detail
