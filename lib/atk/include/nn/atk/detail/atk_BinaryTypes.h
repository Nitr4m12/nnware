#pragma once

#include <cstdint>

#include <nn/util.h>

namespace nn::atk::detail {

struct BinaryTypes {
    static const uint32_t InvalidOffset{0xffffffff};
    static const uint32_t InvalidSize{0xffffffff};

    struct Reference {
        uint32_t offset;

        static const uint32_t InvalidOffset{BinaryTypes::InvalidOffset};

        bool IsValidOffset() const { return offset != InvalidOffset; }
    };
    static_assert(sizeof(Reference) == 0x4);

    template <typename ItemType, typename CountType = int>
    struct Table {
        CountType count;
        ItemType item[1];
    };

    struct ReferenceTable : Table<Reference> {
        const void* GetReferedItem(int index) const;
    };
};

};  // namespace nn::atk::detail
