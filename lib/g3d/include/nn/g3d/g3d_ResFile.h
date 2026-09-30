/**
 * @brief Resource file for models.
 */

#pragma once

#include <cstdint>

#include <nn/util.h>
#include <nn/util/util_AccessorBase.h>
#include <nn/util/util_BinaryFormat.h>
#include <nn/util/util_ResDic.h>

#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/gfx/gfx_Types.h>

#include <nn/g3d/g3d_TextureRef.h>

namespace nn {

namespace gfx {
template <typename T>
class TDevice;
}

namespace g3d {

class ResModel;
class ResMaterialAnim;
class ResShapeAnim;
class ResSceneAnim;
class ResSkeletalAnim;
class ResBoneVisibilityAnim;

struct ResFileData {
    util::BinaryFileHeader fileHeader;
    util::BinTPtr<util::BinString> pFileName;
    util::BinTPtr<ResModel> pModels;
    util::BinTPtr<util::ResDic> pModelDict;
    util::BinTPtr<ResSkeletalAnim> pSkeletalAnims;
    util::BinTPtr<util::ResDic> pSkeletalAnimDict;
    util::BinTPtr<ResMaterialAnim> pMaterialAnims;
    util::BinTPtr<util::ResDic> pMaterialAnimsDict;
    util::BinTPtr<ResBoneVisibilityAnim> pBoneVisibilityAnims;
    util::BinTPtr<util::ResDic> pBoneVisiDict;
    util::BinTPtr<ResShapeAnim> pShapeAnims;
    util::BinTPtr<util::ResDic> pShapeAnimDict;
    util::BinTPtr<ResSceneAnim> pSceneAnims;
    util::BinTPtr<util::ResDic> pSceneAnimDict;
    util::BinTPtr<gfx::MemoryPool> pMemoryPool;
    util::BinTPtr<gfx::MemoryPoolInfo> pMemoryPoolInfo;
    uint64_t embeddedFilesOffset;
    util::BinTPtr<util::ResDic> pEmbeddedFilesDict;
    uint64_t padding;
    uint64_t strTableOffset;
    uint32_t strTableSize;
    uint16_t modelCount;
    uint16_t skeletalAnimCount;
    uint16_t materialAnimCount;
    uint16_t boneAnimCount;
    uint16_t shapeAnimCount;
    uint16_t sceneAnimCount;
    uint16_t externalFileCount;
};

class ResFile : public nn::util::AccessorBase<ResFileData> {
public:
    static const int64_t Signature = 0x2020202053455246;  // "FRES    "

    static bool IsValid(void const* modelSrc);

    void Relocate();
    void Unrelocate();

    static nn::g3d::ResFile* ResCast(void*);

    int32_t BindTexture(TextureBindCallback callback, void* callbackArg);
    void ReleaseTexture();

    void Setup(gfx::Device*);
    void Setup(gfx::Device*, gfx::MemoryPool*, int64_t, uint64_t);

    void Cleanup(gfx::Device*);
    void Reset();
};
static_assert(sizeof(ResFile) == 0xd0);

}  // namespace g3d
}  // namespace nn
