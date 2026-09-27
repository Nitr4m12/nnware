#pragma once

#include <nn/atk/atk_StreamSoundPrefetchFile.h>
#include <nn/atk/detail/atk_IRegionInfoReadable.h>

namespace nn::atk::detail {

class StreamSoundPrefetchFileReader : public IRegionInfoReadable {
public:
    struct PrefetchDataInfo {
        uint32_t startFrame;
        uint32_t prefetchSize;
        const void* dataAddress;
    };

    StreamSoundPrefetchFileReader();
    ~StreamSoundPrefetchFileReader() override;

    void Initialize(const void* streamSoundPrefetchFile);
    void Finalize() {
        m_pHeader = nullptr;
        m_pInfoBlockBody = nullptr;
        m_pPrefetchDataBlockBody = nullptr;
        m_RegionDataOffset = 0;
        m_RegionInfoBytes = 0;
    }

    bool IsAvailable() const { return m_pHeader != nullptr; }

    bool IsIncludeRegionInfo() const;
    bool IsCrc32CheckAvailable() const;
    bool IsRegionIndexCheckAvailable() const;

    bool IsValidFileHeader(const void* streamSoundPrefetchFile) const;

    bool ReadStreamSoundInfo(StreamSoundFile::StreamSoundInfo* strmInfo) const;
    bool ReadDspAdpcmChannelInfo(DspAdpcmParam* pParam, DspAdpcmLoopParam* pLoopParam,
                                 int channelIndex) const;
    bool ReadPrefetchDataInfo(PrefetchDataInfo* pDataInfo, int prefetchIndex) const;

    bool ReadRegionInfo(StreamSoundFile::RegionInfo* pInfo, uint32_t regionIndex) const override;

    uint32_t GetChannelCount() const {
        return m_pInfoBlockBody->GetChannelInfoTable()->GetChannelCount();
    }

    uint32_t GetPrefetchDataCount() const {
        return m_pPrefetchDataBlockBody->GetPrefetchDataCount();
    }

    uint32_t GetRegionDataOffset() const {
        uint32_t result{0};

        if (IsAvailable() && m_pHeader->HasRegionBlock()) {
            result = m_pHeader->GetRegionBlockOffset() + sizeof(BinaryBlockHeader) +
                     m_pInfoBlockBody->GetStreamSoundInfo()->regionDataOffset.offset;
        }

        return result;
    }

    uint16_t GetRegionInfoBytes() const {
        return m_pInfoBlockBody->GetStreamSoundInfo()->regionInfoBytes;
    }

private:
    const StreamSoundPrefetchFile::FileHeader* m_pHeader{};
    const StreamSoundFile::InfoBlockBody* m_pInfoBlockBody{};
    const StreamSoundPrefetchFile::PrefetchDataBlockBody* m_pPrefetchDataBlockBody{};
    uint32_t m_RegionDataOffset{0};
    uint16_t m_RegionInfoBytes{0};
};
static_assert(sizeof(StreamSoundPrefetchFileReader) == 0x28);

}  // namespace nn::atk::detail
