#include <nn/atk/atk_VolumeThroughModePacket.h>

#include <nn/util/util_BitUtil.h>

namespace nn::atk::detail {

size_t VolumeThroughModePacket::GetRequiredMemSize(int busCount) {
    size_t result{VolumeThroughModeArray::GetRequiredMemSize(busCount)};
    result = util::align_up(result, 8);

    return result;
}

}  // namespace nn::atk::detail
