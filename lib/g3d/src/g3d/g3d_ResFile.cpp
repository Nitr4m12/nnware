#include <nn/g3d/g3d_ResFile.h>

#include <nn/gfx/gfx_MemoryPool.h>

#include <nn/g3d/g3d_ResMaterialAnim.h>
#include <nn/g3d/g3d_ResModel.h>
#include <nn/g3d/g3d_ResSceneAnim.h>
#include <nn/g3d/g3d_ResShapeAnim.h>
#include <nn/g3d/g3d_ResSkeletalAnim.h>

namespace nn::g3d {

NN_MIDDLEWARE(g_MiddlewareInfo, "Nintendo", "NintendoWare_G3d" NN_SDK_BUILD_STR);

bool ResFile::IsValid(const void* modelSrc) {
    return static_cast<const util::BinaryFileHeader*>(modelSrc)->IsValid(Signature, 8, 0, 0);
}

void ResFile::Relocate() {
    if (!fileHeader.IsRelocated())
        fileHeader.GetRelocationTable()->Relocate();
}

void ResFile::Unrelocate() {
    if (fileHeader.IsRelocated())
        fileHeader.GetRelocationTable()->Unrelocate();
}

ResFile* ResFile::ResCast(void* ptr) {
    auto* file = static_cast<ResFile*>(ptr);

    if (ptr != nullptr) {
        file->Relocate();
        file->fileHeader.IsEndianReverse();
    }

    return file;
}

int32_t ResFile::BindTexture(TextureBindCallback callback, void* callbackArg) {
    int32_t result = 0;

    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        result |= pModels.Get()[i].BindTexture(callback, callbackArg);

    int32_t matAnimCount = materialAnimCount;
    for (int32_t i{0}; i < matAnimCount; ++i)
        result |= pMaterialAnims.Get()[i].BindTexture(callback, callbackArg);

    return result;
}

void ResFile::ReleaseTexture() {
    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        pModels.Get()[i].ReleaseTexture();

    int32_t matAnimCount = materialAnimCount;
    for (int32_t i{0}; i < matAnimCount; ++i)
        pMaterialAnims.Get()[i].ReleaseTexture();
}

void ResFile::Setup(gfx::Device* device) {
    nn::util::ReferSymbol(g_MiddlewareInfo);

    if (pMemoryPool.Get() != nullptr && pMemoryPoolInfo.Get() != nullptr)
        pMemoryPool.Get()->Initialize(device, *pMemoryPoolInfo.Get(), "g3d");

    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        pModels.Get()[i].Setup(device);
}

void ResFile::Setup(gfx::Device* device, gfx::MemoryPool* memPool, int64_t bufferOffset,
                    [[maybe_unused]] uint64_t) {
    if (pMemoryPool.Get() != nullptr && pMemoryPoolInfo.Get() != nullptr)
        bufferOffset = reinterpret_cast<ptrdiff_t>(pMemoryPoolInfo.Get()->GetPoolMemory()) +
                       (bufferOffset - reinterpret_cast<ptrdiff_t>(this));

    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        pModels.Get()[i].Setup(device, memPool, bufferOffset);
}

void ResFile::Cleanup(gfx::Device* device) {
    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        pModels.Get()[i].Cleanup(device);

    if (pMemoryPoolInfo.Get() != nullptr &&
        pMemoryPool.Get()->ToData()->state != gfx::MemoryPool::DataType::State_NotInitialized)
        pMemoryPool.Get()->Finalize(device);
}

void ResFile::Reset() {
    int32_t mdlCount = modelCount;
    for (int32_t i{0}; i < mdlCount; ++i)
        pModels.Get()[i].Reset();

    int32_t sklAnimCount = skeletalAnimCount;
    for (int32_t i{0}; i < sklAnimCount; ++i)
        pSkeletalAnims.Get()[i].Reset();

    int32_t matAnimCount = materialAnimCount;
    for (int32_t i{0}; i < matAnimCount; ++i)
        pMaterialAnims.Get()[i].Reset();

    int32_t shpAnimCount = shapeAnimCount;
    for (int32_t i{0}; i < shpAnimCount; ++i)
        pShapeAnims.Get()[i].Reset();

    int32_t scnAnimCount = sceneAnimCount;
    for (int32_t i{0}; i < scnAnimCount; ++i)
        pSceneAnims.Get()[i].Reset();

    padding = 0;
}

}  // namespace nn::g3d
