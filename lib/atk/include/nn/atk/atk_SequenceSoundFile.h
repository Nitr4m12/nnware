#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct SequenceSoundFile {
    struct DataBlock;
    struct LabelBlock;
    struct FileHeader : Util::SoundFileHeader {
        const DataBlock* GetDataBlock() const;
        const LabelBlock* GetLabelBlock() const;
    };

    struct DataBlockBody {
        uint8_t sequenceData[1];

        const void* GetSequenceData() const { return &sequenceData; };
    };

    struct DataBlock {
        BinaryBlockHeader header;
        DataBlockBody body;
    };

    struct LabelInfo;
    struct LabelBlockBody {
        Util::ReferenceTable labelInfoReferenceTable;

        int GetLabelCount() const { return static_cast<int>(labelInfoReferenceTable.count); }

        const LabelInfo* GetLabelInfo(int index) const;
        const char* GetLabel(int index) const;
        const char* GetLabelByOffset(uint32_t offset) const;

        bool GetOffset(int index, uint32_t* offsetPtr) const;
        bool GetOffsetByLabel(const char* label, uint32_t* offsetPtr) const;
    };

    struct LabelBlock {
        BinaryBlockHeader header;
        LabelBlockBody body;
    };

    struct LabelInfo {
        Util::Reference referToSequenceData;
        uint32_t labelStringLength;
        char label[1];
    };
    static_assert(sizeof(LabelInfo) == 0x10);
};

}  // namespace nn::atk::detail
