/**
 * @brief Resource file for shape animations.
 */

#pragma once

#include <nn/util/util_BinaryFormat.h>

namespace nn::g3d {

class ResShapeAnim : public util::BinaryBlockHeader {
public:
    void Reset();

    uint8_t _0[0x50];
};

}  // namespace nn::g3d
