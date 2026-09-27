#pragma once

#include <nn/atk/atk_StreamSoundFile.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

class StreamSoundPrefetchFile {
public:
    struct PrefetchDataBlock;
    struct FileHeader : Util::SoundFileHeader {
        const StreamSoundFile::InfoBlock* GetInfoBlock() const;
        const StreamSoundFile::RegionBlock* GetRegionBlock() const;
        const PrefetchDataBlock* GetPrefetchDataBlock() const;

        uint32_t GetPrefetchDataBlockSize() const;

        bool HasRegionBlock() const;
        uint32_t GetRegionBlockSize() const;
        uint32_t GetRegionBlockOffset() const;
    };

    struct PrefetchSample;
    struct PrefetchData {
        uint32_t startFrame;
        uint32_t prefetchSize;
        uint32_t reserved[1];
        Util::Reference toPrefetchSample;

        const PrefetchSample* GetPrefetchSample() const;
    };
    static_assert(sizeof(PrefetchData) == 0x14);

    struct PrefetchDataBlockBody {
        Util::Table<PrefetchData> prefetchDataTable;

        uint32_t GetPrefetchDataCount() const { return prefetchDataTable.count; }
        const PrefetchData* GetPrefetchData(uint32_t index) const {
            return &prefetchDataTable.item[index];
        }
    };

    struct PrefetchDataBlock {
        BinaryBlockHeader header;
        PrefetchDataBlockBody body;
    };

    struct PrefetchSample {
        uint8_t data[1];

        const void* GetSampleAddress() const;
    };
};

}  // namespace nn::atk::detail
