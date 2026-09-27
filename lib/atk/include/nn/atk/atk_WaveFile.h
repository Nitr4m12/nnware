#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct WaveFile {
    struct InfoBlock;
    struct DataBlock;
    struct FileHeader : Util::SoundFileHeader {
        const InfoBlock* GetInfoBlock() const;
        const DataBlock* GetDataBlock() const;
    };

    struct ChannelInfo;
    struct InfoBlockBody {
        uint8_t encoding;
        uint8_t isLoop;
        uint8_t padding[2];
        uint32_t sampleRate;
        uint32_t loopStartFrame;
        uint32_t loopEndFrame;
        uint32_t originalLoopStartFrame;
        Util::ReferenceTable channelInfoReferenceTable;

        int GetChannelCount() const { return static_cast<int>(channelInfoReferenceTable.count); };
        const ChannelInfo& GetChannelInfo(int channelIndex) const;
    };

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };

    struct DspAdpcmInfo;
    struct ChannelInfo {
        Util::Reference referToSamples;
        Util::Reference referToAdpcmInfo;
        uint32_t reserved;

        const void* GetSamplesAddress(const void* dataBlockBodyAddress) const;
        const DspAdpcmInfo& GetDspAdpcmInfo() const;
    };
    static_assert(sizeof(ChannelInfo) == 0x14);

    struct DspAdpcmInfo {
        DspAdpcmParam adpcmParam;
        DspAdpcmLoopParam adpcmLoopParam;
    };
    static_assert(sizeof(DspAdpcmInfo) == 0x2c);

    struct DataBlock {
        BinaryBlockHeader header;
        union {
            int8_t pcm8[1];
            int16_t pcm16[1];
            uint8_t byte[1];
        };
    };
};

}  // namespace nn::atk::detail
