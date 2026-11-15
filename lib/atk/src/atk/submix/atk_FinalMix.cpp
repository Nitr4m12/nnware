#include <nn/util.h>
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
#include <nn/atk/atk_FinalMix.h>

namespace {

const int32_t FinalMixBusCount{1};
const int32_t FinalMixBusIndex{0};

}  // namespace

namespace nn::atk {

size_t FinalMix::GetRequiredMemorySize(bool isEffectEnabled) {
    return OutputMixer::GetRequiredMemorySize(FinalMixBusCount, isEffectEnabled);
}

}  // namespace nn::atk

#endif
