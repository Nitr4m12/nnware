#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct WaveArchiveFile {
    static const int BlockCount{2};

    struct InfoBlock;
    struct FileBlock;
    struct FileHeader : BinaryFileHeader {
    public:
        Util::ReferenceWithSize toBlocks[BlockCount];

        const InfoBlock* GetInfoBlock() const;
        const FileBlock* GetFileBlock() const;

        uint32_t GetInfoBlockSize() const;
        uint32_t GetFileBlockSize() const;

        uint32_t GetInfoBlockOffset() const;
        uint32_t GetFileBlockOffset() const;

    private:
        const Util::ReferenceWithSize* GetReferenceBy(uint16_t typeId) const;
    };
    static_assert(sizeof(FileHeader) == 0x2c);

    struct InfoBlockBody {
        Util::Table<Util::ReferenceWithSize> table;

        uint32_t GetWaveFileCount() const { return table.count; }

        uint32_t GetSize(uint32_t index) const { return table.item[index].size; }

        uint32_t GetOffsetFromFileBlockBody(uint32_t index) const {
            return table.item[index].offset;
        }

        static const uint32_t InvalidOffset = 0xffffffff;
    };

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };

    struct FileBlockBody { /* empty structure */
    };

    struct FileBlock {
        BinaryBlockHeader header;
        FileBlockBody body;
    };
    static_assert(sizeof(FileBlock) == 0xc);
};

}  // namespace nn::atk::detail
