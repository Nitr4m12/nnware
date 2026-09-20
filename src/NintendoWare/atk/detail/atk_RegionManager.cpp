#include <nn/atk/detail/atk_RegionManager.h>

#include <nn/atk/atk_StreamSoundLoader.h>

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

bool RegionManager::InitializeRegion(IRegionInfoReadable* pRegionReader,
                                     StreamDataInfoDetail* pStreamDataInfo) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    if (m_IsRegionIndexCheckEnabled)
        return true;
#else
    if (m_IsRegionInitialized)
        return true;
#endif

    m_CurrentRegionNo = 0;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_pCurrentRegionName = nullptr;
    m_IsCurrentRegionNameEnabled = false;
#endif

    StreamSoundFile::RegionInfo regionInfo;
    m_IsRegionInfoEnabled = pRegionReader->ReadRegionInfo(&regionInfo, 0);
    m_IsRegionIndexCheckEnabled = pStreamDataInfo->isRegionIndexCheckEnabled;

    if (IsPreparedForRegionJump()) {
        if (!ChangeRegion(0, pRegionReader, pStreamDataInfo))
            return false;
    } else {
        m_CurrentRegion.begin = 0;
        m_CurrentRegion.end = static_cast<position_t>(pStreamDataInfo->sampleCount);
    }
    m_CurrentRegion.current = m_CurrentRegion.begin;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_IsRegionInitialized = true;
#endif

    return true;
}

bool RegionManager::IsPreparedForRegionJump() const {
    if (!m_IsRegionInfoEnabled)
        return false;

    return m_StreamRegionCallbackFunc != nullptr;
}

}  // namespace nn::atk::detail
