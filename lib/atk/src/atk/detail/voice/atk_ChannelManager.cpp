#include <nn/atk/atk_ChannelManager.h>

namespace nn::atk::detail::driver {

ChannelManager& ChannelManager::GetInstance() {
    static ChannelManager instance;
    return instance;
}

ChannelManager::ChannelManager() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t ChannelManager::GetRequiredMemSize(int32_t channelCount) {
    return (static_cast<size_t>(channelCount * 2) / 2) * (sizeof(Channel) * 2) + 0x40;
}
#else
size_t ChannelManager::GetObjectSize(const SoundInstanceConfig& config) {
    size_t result{sizeof(Channel)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += AdditionalParamBufferSize;
        result += sizeof(OutputAdditionalParam);
        result = (result + (ChannelPool::DefaultAlignment - 1)) &  // align down
                 -ChannelPool::DefaultAlignment;
    }

    return result;
}

size_t ChannelManager::GetRequiredMemSize(int32_t channelCount, const SoundInstanceConfig& config) {
    return GetObjectSize(config) * channelCount + 0x70;
}
#endif

}  // namespace nn::atk::detail::driver
