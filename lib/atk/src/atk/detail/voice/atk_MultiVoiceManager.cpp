#include <nn/atk/atk_MultiVoiceManager.h>

namespace nn::atk::detail::driver {

MultiVoiceManager& MultiVoiceManager::GetInstance() {
    static MultiVoiceManager instance;
    return instance;
}

MultiVoiceManager::MultiVoiceManager() = default;

size_t MultiVoiceManager::GetObjectSize(const SoundInstanceConfig& config) {
    size_t result{sizeof(MultiVoice)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += sizeof(OutputAdditionalParam);
        result += AdditionalParamBufferSize;
    }

    return result;
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount) {
    return sizeof(MultiVoice) * voiceCount;
}
#else
size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount,
                                             const SoundInstanceConfig& config) {
    return GetObjectSize(config) * voiceCount;
}
#endif

}  // namespace nn::atk::detail::driver
