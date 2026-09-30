/**
 * @brief Resource file for material animations.
 */

#pragma once

#include <cstdint>

#include <nn/g3d/g3d_TextureRef.h>

namespace nn::g3d {

class ResMaterialAnim {
public:
    void ReleaseTexture();
    int32_t BindTexture(TextureBindCallback, void*);
    void Reset();

    uint8_t _0[0x78];
};

}  // namespace nn::g3d
