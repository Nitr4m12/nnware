#include <nn/atk/atk_BusMixVolumePacket.h>
#include <nn/util/util_BitUtil.h>

namespace nn::atk::detail {

BusMixVolumePacket::BusMixVolumePacket() = default;

size_t BusMixVolumePacket::GetRequiredMemSize(int busCount) {
    size_t result{util::align_up<uint64_t>(busCount, 8)};
    return result;
}

}  // namespace nn::atk::detail
