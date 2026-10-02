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

float TaskProfile::LoadStreamBlock::GetRemainingCachePercentage() const {
    if (m_CachedLength == 0)
        return 0.0f;

    if (m_CacheCurrentPosition < m_CacheStartPosition)
        return 0.0f;

    return ((static_cast<float>(m_CacheStartPosition) + m_CachedLength - m_CacheCurrentPosition) /
            m_CachedLength) *
           100.0f;
}

}  // namespace nn::atk
