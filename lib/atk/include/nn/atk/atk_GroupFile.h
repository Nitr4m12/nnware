#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct GroupFile {
    struct InfoBlock;
    struct FileBlock;
    struct InfoExBlock;
    struct FileHeader : Util::SoundFileHeader {
        const InfoBlock* GetInfoBlock() const;
        const FileBlock* GetFileBlock() const;
        const InfoExBlock* GetInfoExBlock() const;
    };

    struct GroupItemInfo;
    struct InfoBlockBody {
        Util::ReferenceTable referenceTableOfGroupItemInfo;

        uint32_t GetGroupItemInfoCount() const { return referenceTableOfGroupItemInfo.count; }

        const GroupItemInfo* GetGroupItemInfo(uint32_t index) const {
            if (GetGroupItemInfoCount() <= index)
                return nullptr;

            return util::ConstBytePtr(this, referenceTableOfGroupItemInfo.item[index].offset)
                .Get<GroupItemInfo>();
        }
    };

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };

    struct FileBlockBody;
    struct GroupItemInfo {
        uint32_t fileId;
        Util::ReferenceWithSize embeddedItemInfo;

        static const uint32_t OffsetForLink{0xffffffff};
        static const uint32_t SizeForLink{0xffffffff};

        const void* GetFileLocation(const FileBlockBody* fileBlockBody) const {
            if (static_cast<uint32_t>(embeddedItemInfo.offset) == OffsetForLink)
                return nullptr;

            return util::ConstBytePtr(fileBlockBody, embeddedItemInfo.offset).Get();
        }
    };
    static_assert(sizeof(GroupItemInfo) == 0x10);

    struct FileBlockBody {
        /* empty structure */
    };

    struct FileBlock {
        BinaryBlockHeader header;
        FileBlockBody body;
    };

    struct GroupItemInfoEx;
    struct InfoExBlockBody {
        Util::ReferenceTable referenceTableOfGroupItemInfoEx;

        uint32_t GetGroupItemInfoExCount() const { return referenceTableOfGroupItemInfoEx.count; }

        const GroupItemInfoEx* GetGroupItemInfoEx(uint32_t index) const {
            if (GetGroupItemInfoExCount() <= index)
                return nullptr;

            return util::ConstBytePtr(this, referenceTableOfGroupItemInfoEx.item[index].offset)
                .Get<GroupItemInfoEx>();
        }
    };

    struct InfoExBlock {
        BinaryBlockHeader header;
        InfoExBlockBody body;
    };

    struct GroupItemInfoEx {
        uint32_t itemId;
        uint32_t loadFlag;
    };
    static_assert(sizeof(GroupItemInfoEx) == 0x8);
};

}  // namespace nn::atk::detail
