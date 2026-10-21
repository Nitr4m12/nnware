#include <nn/atk/atk_VolumeThroughModePacket.h>

#include <nn/util/util_BitUtil.h>

namespace nn::atk::detail {

size_t VolumeThroughModePacket::GetRequiredMemSize(int busCount) {
    size_t result{VolumeThroughModeArray::GetRequiredMemSize(busCount)};
    result = util::align_up(result, 8);

    return result;
}

bool VolumeThroughModePacket::Initialize(void *buffer, size_t size, int busCount) {
    return m_VolumeThroughMode.Initialize(buffer, size, busCount);
}

}  // namespace nn::atk::detail
