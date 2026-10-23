#include <nn/atk/atk_OutputAdditionalParam.h>

namespace {

int32_t GetAdditionalBusCount(int32_t busCount) {
    return busCount - 4;
}

}  // anonymous namespace

namespace nn::atk::detail {

size_t OutputAdditionalParam::GetRequiredMemSize(const SoundInstanceConfig& config) {
    size_t result{0};

    if (config.busCount > 4)
        result = SendArray::GetRequiredMemSize(GetAdditionalBusCount(config.busCount)) + 16;

    if (config.isBusMixVolumeEnabled) {
        result += BusMixVolumePacket::GetRequiredMemSize(config.busCount);
        result += sizeof(BusMixVolumePacket);
    }

    if (config.isVolumeThroughModeEnabled) {
        result += VolumeThroughModePacket::GetRequiredMemSize(config.busCount);
        result += sizeof(VolumeThroughModePacket);
    }

    return result;
}

}  // namespace nn::atk::detail
