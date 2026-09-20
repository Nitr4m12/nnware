#include <nn/atk/detail/atk_RegionManager.h>

namespace nn::atk::detail {

void RegionManager::Initialize() {
    m_IsRegionInfoEnabled = false;
    m_IsRegionIndexCheckEnabled = false;
    m_IsRegionInitialized = false;
    m_StreamRegionCallbackFunc = nullptr;
    m_StreamRegionCallbackArg = nullptr;
    m_AdpcmContextForStartOffsetFrame = 0xffffffff;
}

}  // namespace nn::atk::detail
