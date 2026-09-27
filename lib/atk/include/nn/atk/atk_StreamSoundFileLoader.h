#pragma once

#include <nn/atk/atk_StreamSoundFileReader.h>
#include <nn/atk/detail/atk_IRegionInfoReadable.h>
#include <nn/atk/fnd/io/atkfnd_FileStream.h>

namespace nn::atk::detail {

class StreamSoundFileLoader : public IRegionInfoReadable {
public:
    StreamSoundFileLoader() = default;
    explicit StreamSoundFileLoader(fnd::FileStream* stream) { Initialize(stream); }

    void Initialize(fnd::FileStream* stream) {
        m_pStream = stream;
        m_SeekBlockOffset = 0;
        m_RegionDataOffset = 0;
        m_RegionInfoBytes = 0;
    }

    void Finalize() {
        m_pStream = nullptr;
        m_SeekBlockOffset = 0;
        m_RegionDataOffset = 0;
        m_RegionInfoBytes = 0;
    }

    bool LoadFileHeader(StreamSoundFileReader* reader, void* buffer, uint64_t size);

    bool ReadSeekBlockData(uint16_t* yn1, uint16_t* yn2, int blockIndex, int channelCount);
    bool ReadRegionInfo(StreamSoundFile::RegionInfo* pInfo, uint32_t regionIndex) const override;

    ~StreamSoundFileLoader() override = default;

private:
    fnd::FileStream* m_pStream{};
    uint32_t m_SeekBlockOffset{0};
    uint32_t m_RegionDataOffset{0};
    uint16_t m_RegionInfoBytes{0};
};
static_assert(sizeof(StreamSoundFileLoader) == 0x20);

}  // namespace nn::atk::detail
