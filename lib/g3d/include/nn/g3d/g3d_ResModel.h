/**
 * @file ResModel.h
 * @brief Resource model.
 */

#pragma once

#include <cstdint>

#include <nn/gfx/gfx_Types.h>

#include <nn/g3d/g3d_TextureRef.h>

namespace nn::g3d {
class ResMaterial;

class ResModel {
public:
    int32_t BindTexture(TextureBindCallback, void*);
    void ForceBindTexture(nn::g3d::TextureRef const&, char const*);
    void ReleaseTexture();
    void Setup(gfx::Device*);
    void Setup(gfx::Device*, gfx::MemoryPool*, int64_t);
    void Cleanup(gfx::Device*);
    void Reset();
    void Reset(uint32_t);
    nn::g3d::ResMaterial* FindMaterial(char const* materialName) const;

    uint8_t _0[0x78];
};

}  // namespace nn::g3d
