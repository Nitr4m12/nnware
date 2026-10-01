#include <nn/atk/atk_TaskProfileReader.h>

namespace nn::atk {

TimeSpan TaskProfile::LoadStreamBlock::GetTotalTime() const {
    return os::Tick(m_EndTick - m_BeginTick).ToTimeSpan();
}

}  // namespace nn::atk
