#pragma once

#include <nn/atk/atk_StreamSoundFile.h>

namespace nn::atk::detail {

class StreamSoundFileReader {
public:
    struct TrackInfo {
        uint8_t volume;
        uint8_t pan;
        uint8_t span;
        uint8_t flags;
        uint8_t channelCount;
        uint8_t globalChannelIndex[2];

        TrackInfo();
    };

    StreamSoundFileReader();

    void Initialize(const void* streamSoundFile);
    void Finalize();

    bool IsAvailable() const { return m_pHeader != nullptr; }

    bool IsTrackInfoAvailable() const;
    bool IsOriginalLoopAvailable() const;
    bool IsCrc32CheckAvailable() const;
    bool IsRegionIndexCheckAvailable() const;

    static bool IsValidFileHeader(const void* streamSoundFile);

    bool ReadStreamSoundInfo(StreamSoundFile::StreamSoundInfo* strmInfo) const;
    bool ReadStreamTrackInfo(TrackInfo* pTrackInfo, int trackIndex) const;
    bool ReadDspAdpcmChannelInfo(DspAdpcmParam* pParam, DspAdpcmLoopParam* pLoopParam,
                                 int channelIndex) const;

    uint32_t GetChannelCount() const {
        return m_pInfoBlockBody->GetChannelInfoTable()->GetChannelCount();
    }

    uint32_t GetTrackCount() const {
        return m_pInfoBlockBody->GetTrackInfoTable()->GetTrackCount();
    }

    uint32_t GetSeekBlockOffset() const {
        if (m_pHeader != nullptr && m_pHeader->HasSeekBlock())
            return m_pHeader->GetSeekBlockOffset();

        return 0;
    }

    uint32_t GetSampleDataOffset() const {
        uint32_t result{0};

        if (m_pHeader != nullptr)
            result = m_pHeader->GetDataBlockOffset() +
                     m_pInfoBlockBody->GetStreamSoundInfo()->sampleDataOffset.offset +
                     sizeof(BinaryBlockHeader);

        return result;
    }

    uint32_t GetRegionDataOffset() const {
        uint32_t result{0};

        if (m_pHeader != nullptr && m_pHeader->HasRegionBlock()) {
            result = m_pHeader->GetRegionBlockOffset() +
                     m_pInfoBlockBody->GetStreamSoundInfo()->regionDataOffset.offset +
                     sizeof(BinaryBlockHeader);
        }

        return result;
    }

    uint32_t GetRegionInfoBytes() const {
        return m_pInfoBlockBody->GetStreamSoundInfo()->regionInfoBytes;
    }

    static bool IsOriginalLoopAvailableImpl(const StreamSoundFile::FileHeader* pHeader);

private:
    const StreamSoundFile::FileHeader* m_pHeader{};
    const StreamSoundFile::InfoBlockBody* m_pInfoBlockBody{};
};
static_assert(sizeof(StreamSoundFileReader) == 0x10);

}  // namespace nn::atk::detail
