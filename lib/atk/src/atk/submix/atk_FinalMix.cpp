#include <nn/util.h>
#include "nn/audio.h"
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
#include <nn/atk/atk_FinalMix.h>

#include <nn/atk/fnd/basis/atkfnd_WorkBufferAllocator.h>

namespace {

const int32_t FinalMixBusCount{1};
const int32_t FinalMixBusIndex{0};

}  // namespace

namespace nn::atk {

size_t FinalMix::GetRequiredMemorySize(bool isEffectEnabled) {
    return OutputMixer::GetRequiredMemorySize(FinalMixBusCount, isEffectEnabled);
}

bool FinalMix::Initialize(audio::AudioRendererConfig* pConfig, int32_t channelCount,
                          bool isEffectEnabled, void* buffer, size_t bufferSize) {
    detail::fnd::WorkBufferAllocator allocator{buffer, bufferSize};
    size_t requiredSize{GetRequiredMemorySize(isEffectEnabled)};
    void* mixerBuffer{allocator.Allocate(requiredSize, alignof(OutputMixer))};

    OutputMixer::Initialize(FinalMixBusCount, isEffectEnabled, mixerBuffer,
                            GetRequiredMemorySize(isEffectEnabled));

    m_ChannelCount = channelCount;
    m_ReferenceCount = 0;

    return audio::AcquireFinalMix(pConfig, &m_FinalMix, GetBusCount() * GetChannelCount());
}

}  // namespace nn::atk

#endif
