#include <nn/atk/detail/atk_RegionManager.h>

#include <nn/util/util_StringUtil.h>

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

// NON_MATCHING
bool RegionManager::ChangeRegion(int currentRegionNo, IRegionInfoReadable* pRegionReader,
                                 StreamDataInfoDetail* pStreamDataInfo) {
    StreamRegionCallbackParam param;
    param.regionNo = currentRegionNo;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_pCurrentRegionName != nullptr)
        util::Strlcpy(const_cast<char*>(m_pCurrentRegionName), param.regionName,
                      sizeof(param.regionName));
    else
        std::memset(param.regionName, 0, sizeof(param.regionName));
    param.isRegionNameEnabled = m_IsCurrentRegionNameEnabled;
#endif

    param.regionCount = pStreamDataInfo->regionCount;

    if (m_StreamRegionCallbackFunc(&param, m_StreamRegionCallbackArg) ==
        StreamRegionCallbackResult_Finish)
        return false;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (param.isRegionNameEnabled) {
        // TODO
    }
#endif

    m_CurrentRegionNo = param.regionNo;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_pCurrentRegionName = nullptr;
#endif

    return true;
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void RegionManager::SetRegionInfo(int regionNo, IRegionInfoReadable* pRegionReader,
                                  StreamDataInfoDetail* pStreamDataInfo) {
    StreamSoundFile::RegionInfo regionInfo;
    if (!pRegionReader->ReadRegionInfo(&regionInfo, regionNo)) {
        m_CurrentRegion.begin = 0;
        m_CurrentRegion.end = static_cast<position_t>(pStreamDataInfo->sampleCount);
        return;
    }

    m_CurrentRegion.begin = regionInfo.start;
    m_CurrentRegion.end = regionInfo.end;

    if (pStreamDataInfo->sampleFormat == SampleFormat_DspAdpcm) {
        for (int ch{0}; ch < pStreamDataInfo->channelCount; ++ch) {
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.predScale =
                regionInfo.adpcmContext[ch].loopPredScale;
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.history[0] =
                static_cast<s16>(regionInfo.adpcmContext[ch].loopYn1);
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.history[1] =
                static_cast<s16>(regionInfo.adpcmContext[ch].loopYn2);
        }
        m_AdpcmContextForStartOffsetFrame = regionInfo.start;
    }
}
#endif

// UNCHECKED
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void RegionManager::SetRegionInfo(const StreamSoundFile::RegionInfo* pRegionInfo,
                                  const StreamDataInfoDetail* pStreamDataInfo) {
    if (pRegionInfo == nullptr) {
        m_CurrentRegion.begin = 0;
        m_CurrentRegion.end = static_cast<position_t>(pStreamDataInfo->sampleCount);
        m_CurrentRegion.isEnabled = true;
        return;
    }

    m_CurrentRegion.begin = pRegionInfo->start;
    m_CurrentRegion.end = pRegionInfo->end;

    m_CurrentRegion.isEnabled = false;
    if (m_IsRegionIndexCheckEnabled)
        m_CurrentRegion.isEnabled = pRegionInfo->isEnabled;

    if (pStreamDataInfo->sampleFormat == SampleFormat_DspAdpcm) {
        for (int ch{0}; ch < pStreamDataInfo->channelCount; ++ch) {
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.predScale =
                pRegionInfo->adpcmContext[ch].loopPredScale;
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.history[0] =
                static_cast<s16>(pRegionInfo->adpcmContext[ch].loopYn1);
            m_AdpcmContextForStartOffset[ch].audioAdpcmContext.history[1] =
                static_cast<s16>(pRegionInfo->adpcmContext[ch].loopYn2);
        }
        m_AdpcmContextForStartOffsetFrame = pRegionInfo->start;
    }
}
#endif

}  // namespace nn::atk::detail
