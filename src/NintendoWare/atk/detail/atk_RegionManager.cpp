#include <nn/atk/detail/atk_RegionManager.h>

namespace nn::atk::detail {

void RegionManager::Initialize() {
    m_IsRegionInfoEnabled = false;
    m_IsRegionIndexCheckEnabled = false;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_IsRegionInitialized = false;
#endif
    m_StreamRegionCallbackFunc = nullptr;
    m_StreamRegionCallbackArg = nullptr;
    m_AdpcmContextForStartOffsetFrame = 0xffffffff;
}

bool RegionManager::IsPreparedForRegionJump() const {
    if (!m_IsRegionInfoEnabled)
        return false;

    return m_StreamRegionCallbackFunc != nullptr;
}

}  // namespace nn::atk::detail
