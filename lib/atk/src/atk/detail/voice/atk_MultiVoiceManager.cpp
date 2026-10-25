#include <nn/atk/atk_MultiVoiceManager.h>

namespace nn::atk::detail::driver {

MultiVoiceManager& MultiVoiceManager::GetInstance() {
    static MultiVoiceManager instance;
    return instance;
}

MultiVoiceManager::MultiVoiceManager() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount) {
    return sizeof(MultiVoice) * voiceCount;
}
#else
size_t MultiVoiceManager::GetObjectSize(const SoundInstanceConfig& config) {
    size_t result{sizeof(MultiVoice)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += sizeof(OutputAdditionalParam);
        result += AdditionalParamBufferSize;
    }

    return result;
}

size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount,
                                             const SoundInstanceConfig& config) {
    return GetObjectSize(config) * voiceCount;
}
#endif

void MultiVoiceManager::Initialize(void* mem, size_t memSize, const SoundInstanceConfig& config) {
    if (m_Initialized)
        return;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    const size_t TotalObjectSize{sizeof(MultiVoice)};
#else
    const size_t TotalObjectSize{GetObjectSize(config)};
#endif

    const int VoiceCount{static_cast<int>(memSize / TotalObjectSize)};

    uint8_t* ptr{static_cast<uint8_t*>(mem)};

    for (int i{0}; i < VoiceCount; ++i) {
        uint8_t* voicePtr{ptr};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        new (voicePtr) MultiVoice();
#else
        ptr += sizeof(MultiVoice);
        OutputAdditionalParam* additionalParamPtr{};
        const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};

        if (AdditionalParamBufferSize != 0) {
            additionalParamPtr = new (ptr) OutputAdditionalParam;
            ptr += sizeof(OutputAdditionalParam);

            void* additionalParamBufferPtr{ptr};

            additionalParamPtr->Initialize(additionalParamBufferPtr, AdditionalParamBufferSize,
                                           config);
            ptr += AdditionalParamBufferSize;
        }

        new (voicePtr) MultiVoice(additionalParamPtr);
#endif

        m_FreeVoiceList.push_back(*reinterpret_cast<MultiVoice*>(voicePtr));
    }

    m_Initialized = true;
}

}  // namespace nn::atk::detail::driver
