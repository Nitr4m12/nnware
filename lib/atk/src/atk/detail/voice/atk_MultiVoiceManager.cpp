#include <nn/atk/atk_MultiVoiceManager.h>

namespace nn::atk::detail::driver {

MultiVoiceManager& MultiVoiceManager::GetInstance() {
    static MultiVoiceManager instance;
    return instance;
}

MultiVoiceManager::MultiVoiceManager() = default;

size_t MultiVoiceManager::GetObjectSize(const SoundInstanceConfig &config) {
    size_t result{sizeof(MultiVoice)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += sizeof(OutputAdditionalParam);
        result += AdditionalParamBufferSize;
    }

    return result;
}

}  // namespace nn::atk::detail::driver
