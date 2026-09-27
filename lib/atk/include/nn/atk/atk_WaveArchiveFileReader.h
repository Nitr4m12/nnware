#pragma once

#include <nn/atk/atk_WaveArchiveFile.h>

namespace nn::atk::detail {

class WaveArchiveFileReader {
public:
    static const uint32_t SignatureFile;
    static const uint32_t SignatureWarcTable;

    WaveArchiveFileReader(const void* pWaveArchiveFile, bool isIndividual);
    WaveArchiveFileReader();

    void Initialize(const void* pWaveArchiveFile, bool isIndividual);
    void Finalize();

    void InitializeFileTable();

    bool IsAvailable() const { return m_pHeader != nullptr; }

    uint32_t GetWaveFileCount() const;
    uint32_t GetWaveFileSize(uint32_t waveIndex) const;
    uint32_t GetWaveFileOffsetFromFileHead(uint32_t waveIndex) const;

    const void* GetWaveFile(uint32_t waveIndex) const;
    const void* SetWaveFile(uint32_t waveIndex, const void* pWaveFile);

    bool IsLoaded(uint32_t waveIndex) {
        return m_IsInitialized && GetWaveFile(waveIndex) != nullptr;
    }

    bool HasIndividualLoadTable() const;

    struct IndividualLoadTable {
        const void* waveFile[1];
    };

private:
    const void* GetWaveFileForWhole(uint32_t waveIndex) const {
        uint32_t offset{m_pInfoBlockBody->GetOffsetFromFileBlockBody(waveIndex)};

        return util::ConstBytePtr(&m_pHeader->GetFileBlock()->body, offset).Get();
    }

    const void* GetWaveFileForIndividual(uint32_t waveIndex) const {
        return m_pLoadTable->waveFile[waveIndex];
    }

    const WaveArchiveFile::FileHeader* m_pHeader{};
    const WaveArchiveFile::InfoBlockBody* m_pInfoBlockBody{};
    IndividualLoadTable* m_pLoadTable{};
    bool m_IsInitialized{false};
};
static_assert(sizeof(WaveArchiveFileReader) == 0x20);

}  // namespace nn::atk::detail
