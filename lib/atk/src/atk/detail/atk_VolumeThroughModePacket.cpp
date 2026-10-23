#include <nn/atk/atk_VolumeThroughModePacket.h>

#include <nn/util/util_BitUtil.h>

namespace nn::atk::detail {

size_t VolumeThroughModePacket::GetRequiredMemSize(int busCount) {
    size_t result{VolumeThroughModeArray::GetRequiredMemSize(busCount)};
    result = util::align_up(result, 8);

    return result;
}

bool VolumeThroughModePacket::Initialize(void* buffer, size_t size, int busCount) {
    return m_VolumeThroughMode.Initialize(buffer, size, busCount);
}

void VolumeThroughModePacket::Finalize() {
    m_VolumeThroughMode.Finalize();
    m_IsVolumeThroughModeUsed = false;
    m_BinaryVolume = 1.0f;
}

void VolumeThroughModePacket::Reset() {
    m_VolumeThroughMode.Reset();
    m_IsVolumeThroughModeUsed = false;
    m_BinaryVolume = 1.0f;
}

VolumeThroughModePacket& VolumeThroughModePacket::operator=(const VolumeThroughModePacket& rhs) {
    m_VolumeThroughMode = rhs.m_VolumeThroughMode;
    m_IsVolumeThroughModeUsed = rhs.m_IsVolumeThroughModeUsed;
    m_BinaryVolume = rhs.m_BinaryVolume;
    return *this;
}

}  // namespace nn::atk::detail
