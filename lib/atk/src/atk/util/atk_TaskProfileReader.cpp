#include <nn/atk/atk_TaskProfileReader.h>

namespace nn::atk {

TimeSpan TaskProfile::LoadStreamBlock::GetTotalTime() const {
    return os::Tick(m_EndTick - m_BeginTick).ToTimeSpan();
}

os::Tick TaskProfile::LoadStreamBlock::GetBeginTick() const {
    return m_BeginTick;
}

os::Tick TaskProfile::LoadStreamBlock::GetEndTick() const {
    return m_EndTick;
}

#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
float TaskProfile::LoadStreamBlock::GetRemainingCachePercentage() const {
    if (m_CachedLength == 0)
        return 0.0f;

    if (m_CacheCurrentPosition < m_CacheStartPosition)
        return 0.0f;

    return ((static_cast<float>(m_CacheStartPosition) + m_CachedLength - m_CacheCurrentPosition) /
            m_CachedLength) *
           100.0f;
}

size_t TaskProfile::LoadStreamBlock::GetCachedLength() const {
    return m_CachedLength;
}

detail::driver::StreamSoundPlayer* TaskProfile::LoadStreamBlock::GetStreamSoundPlayer() const {
    return m_pPlayer;
}
#endif

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
void TaskProfile::LoadStreamBlock::SetTick(const os::Tick& beginTick, const os::Tick& endTick)
#else
void TaskProfile::LoadStreamBlock::SetData(
    const os::Tick& beginTick, const os::Tick& endTick,
    const detail::IStreamDataDecoder::CacheProfile& cacheProfile)
#endif
{
    m_BeginTick = beginTick.GetInt64Value();
    m_EndTick = endTick.GetInt64Value();
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    m_CacheStartPosition = cacheProfile.cacheStartPosition;
    m_CachedLength = cacheProfile.cachedLength;
    m_CacheCurrentPosition = cacheProfile.cacheCurrentPosition;
    m_pPlayer = cacheProfile.player;
#endif
}

TimeSpan TaskProfile::LoadOpusStreamBlock::GetTotalTime() const {
    return os::Tick(m_EndTick - m_BeginTick).ToTimeSpan();
}

os::Tick TaskProfile::LoadOpusStreamBlock::GetBeginTick() const {
    return m_BeginTick;
}

os::Tick TaskProfile::LoadOpusStreamBlock::GetEndTick() const {
    return m_EndTick;
}

#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
float TaskProfile::LoadOpusStreamBlock::GetRemainingCachePercentage() const {
    if (m_CachedLength == 0)
        return 0.0f;

    if (m_CacheCurrentPosition < m_CacheStartPosition)
        return 0.0f;

    return ((static_cast<float>(m_CacheStartPosition) + m_CachedLength - m_CacheCurrentPosition) /
            m_CachedLength) *
           100.0f;
}

size_t TaskProfile::LoadOpusStreamBlock::GetCachedLength() const {
    return m_CachedLength;
}
#endif

}  // namespace nn::atk
