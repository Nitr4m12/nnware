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

}  // namespace nn::atk
