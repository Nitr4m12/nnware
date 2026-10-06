#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct StreamSoundFile {
    struct InfoBlock;

    struct FileHeader : BinaryFileHeader {
    private:
        static const int BlockCount = 4;

    public:
        Util::ReferenceWithSize toBlocks[BlockCount];

        bool HasSeekBlock() const;
        bool HasRegionBlock() const;

        uint32_t GetInfoBlockSize() const;
        uint32_t GetSeekBlockSize() const;
        uint32_t GetDataBlockSize() const;
        uint32_t GetRegionBlockSize() const;

        uint32_t GetInfoBlockOffset() const;
        uint32_t GetSeekBlockOffset() const;
        uint32_t GetDataBlockOffset() const;
        uint32_t GetRegionBlockOffset() const;

        const InfoBlock* GetInfoBlock() const {
            return util::ConstBytePtr(this).Advance(GetInfoBlockOffset()).Get<InfoBlock>();
        }

    private:
        const Util::ReferenceWithSize* GetReferenceBy(uint16_t typeId) const;
    };
    static_assert(sizeof(FileHeader) == 0x44);

    struct StreamSoundInfo;
    struct TrackInfoTable;
    struct ChannelInfoTable;
    struct InfoBlockBody {
        Util::Reference toStreamSoundInfo;
        Util::Reference toTrackInfoTable;
        Util::Reference toChannelInfoTable;

        const StreamSoundInfo* GetStreamSoundInfo() const;
        const TrackInfoTable* GetTrackInfoTable() const;
        const ChannelInfoTable* GetChannelInfoTable() const;
    };
    static_assert(sizeof(InfoBlockBody) == 0x18);

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };
    static_assert(sizeof(InfoBlock) == 0x20);

    struct StreamSoundInfo {
        uint8_t encodeMethod;
        bool isLoop;
        uint8_t channelCount;
        uint8_t regionCount;
        uint32_t sampleRate;
        uint32_t loopStart;
        uint32_t frameCount;
        uint32_t blockCount;
        uint32_t oneBlockBytes;
        uint32_t oneBlockSamples;
        uint32_t lastBlockBytes;
        uint32_t lastBlockSamples;
        uint32_t lastBlockPaddedBytes;
        uint32_t sizeofSeekInfoAtom;
        uint32_t seekInfoIntervalSamples;
        Util::Reference sampleDataOffset;
        uint16_t regionInfoBytes;
        uint8_t padding[2];
        Util::Reference regionDataOffset;
        uint32_t originalLoopStart;
        uint32_t originalLoopEnd;
        uint32_t crc32Value;
    };
    static_assert(sizeof(StreamSoundInfo) == 0x50);

    struct TrackInfo;
    struct TrackInfoTable {
        Util::ReferenceTable table;

        uint32_t GetTrackCount() const { return table.count; }

        const TrackInfo* GetTrackInfo(uint32_t index) const;
    };

    struct GlobalChannelIndexTable;
    struct TrackInfo {
        uint8_t volume;
        uint8_t pan;
        uint8_t span;
        uint8_t flags;

        Util::Reference toGlobalChannelIndexTable;

        uint32_t GetTrackChannelCount() const { return GetGlobalChannelIndexTable().GetCount(); }

        uint8_t GetGlobalChannelIndex(uint32_t index) const {
            return GetGlobalChannelIndexTable().GetGlobalIndex(index);
        }

    private:
        const GlobalChannelIndexTable& GetGlobalChannelIndexTable() const {
            return *util::ConstBytePtr(this)
                        .Advance(toGlobalChannelIndexTable.offset)
                        .Get<GlobalChannelIndexTable>();
        }
    };
    static_assert(sizeof(TrackInfo) == 0xc);

    struct GlobalChannelIndexTable {
        Util::Table<uint8_t> table;

        uint32_t GetCount() const { return table.count; }
        uint8_t GetGlobalIndex(uint32_t index) const { return table.item[index]; }
    };

    struct ChannelInfo;
    struct ChannelInfoTable {
        Util::ReferenceTable table;

        uint32_t GetChannelCount() const { return table.count; }
        const ChannelInfo* GetChannelInfo(uint32_t index) const;
    };

    struct DspAdpcmChannelInfo;
    struct ChannelInfo {
        Util::Reference toDetailChannelInfo;

        const DspAdpcmChannelInfo* GetDspAdpcmChannelInfo() const;
    };
    static_assert(sizeof(ChannelInfo) == 0x8);

    struct DspAdpcmChannelInfo {
        DspAdpcmParam param;
        DspAdpcmLoopParam loopParam;
    };
    static_assert(sizeof(DspAdpcmChannelInfo) == 0x2c);

    struct RegionInfo {
        uint32_t start;
        uint32_t end;
        DspAdpcmLoopParam adpcmContext[16];
        bool isEnabled;
        uint8_t padding[87];
        char regionName[64];
    };
    static_assert(sizeof(RegionInfo) == 0x100);

    struct RegionBlock {
        BinaryBlockHeader header;
        RegionInfo info;
    };
    static_assert(sizeof(RegionBlock) == 0x108);
};

}  // namespace nn::atk::detail
