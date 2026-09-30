/**
 * @brief Resource file for scene animations.
 */

#pragma once

#include <cstdint>

#include <nn/util/util_BinaryFormat.h>

namespace nn::g3d {

class ResLightAnim;
class ResFogAnim;
class BindFuncTable;

class ResSceneAnim : public util::BinaryBlockHeader {
public:
    int32_t Bind(nn::g3d::BindFuncTable const&);
    void Release();
    void Reset();

    uint64_t mNameOffset;                // _10
    uint64_t mPathOffset;                // _18
    uint64_t mCameraAnimOffset;          // _20
    uint64_t mCameraAnimDictOffset;      // _28
    nn::g3d::ResLightAnim* mLightAnims;  // _30
    uint64_t mLightAnimDictOffset;       // _38
    nn::g3d::ResFogAnim* mFogAnims;      // _40
    uint64_t mFogAnimDictOffset;         // _48
    uint64_t mUserDataOffset;            // _50
    uint64_t mUserDataDictOffset;        // _58
    uint16_t mUserDataCount;             // _60
    uint16_t mCameraAnimCount;           // _62
    uint16_t mLightAnimCount;            // _64
    uint16_t mFogAnimCount;              // _66
};

}  // namespace nn::g3d
