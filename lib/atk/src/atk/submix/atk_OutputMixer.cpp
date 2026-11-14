#include <nn/atk/atk_OutputMixer.h>

namespace nn::atk {

OutputMixer::OutputMixer() = default;

size_t OutputMixer::GetRequiredMemorySize(int32_t bus, bool isEffectEnabled) {
    size_t result{0};
    if (!isEffectEnabled)
        return 0;

    result = bus;
    result *= 32;

    return result;
}

}  // namespace nn::atk
