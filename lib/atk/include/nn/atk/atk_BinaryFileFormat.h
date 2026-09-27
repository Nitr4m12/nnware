#pragma once

#include <cstdint>

namespace nn::atk::detail {

struct BinaryFileHeader {
    int32_t signature;
    uint16_t byteOrder;
    uint16_t headerSize;
    uint32_t version;
    uint32_t fileSize;
    uint16_t dataBlocks;
    uint16_t reserved;

    static const uint16_t ValidByteOrderMark{0xFEFF};
};
static_assert(sizeof(BinaryFileHeader) == 0x14);

struct BinaryBlockHeader {
    // Named "kind", but functionally the same as BinaryFileHeader.signature
    int32_t kind;
    uint32_t size;
};
static_assert(sizeof(BinaryBlockHeader) == 0x8);

}  // namespace nn::atk::detail
