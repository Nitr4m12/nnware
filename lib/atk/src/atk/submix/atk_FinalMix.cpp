#include <nn/util.h>
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
#include <nn/atk/atk_FinalMix.h>

#include <nn/audio.h>

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
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    OutputMixer::Initialize(FinalMixBusCount, isEffectEnabled, buffer, bufferSize);
#else
    detail::fnd::WorkBufferAllocator allocator{buffer, bufferSize};
    size_t requiredSize{GetRequiredMemorySize(isEffectEnabled)};
    void* mixerBuffer{allocator.Allocate(requiredSize, alignof(OutputMixer))};

    OutputMixer::Initialize(FinalMixBusCount, isEffectEnabled, mixerBuffer,
                            GetRequiredMemorySize(isEffectEnabled));
#endif

    m_ChannelCount = channelCount;
    m_ReferenceCount = 0;

    return audio::AcquireFinalMix(pConfig, &m_FinalMix, GetBusCount() * GetChannelCount());
}

void FinalMix::Finalize(audio::AudioRendererConfig* pConfig) {
    audio::ReleaseFinalMix(pConfig, &m_FinalMix);
    OutputMixer::Finalize();
}

bool FinalMix::AppendEffect(EffectBase* pEffect, void* buffer, size_t bufferSize) {
    return OutputMixer::AppendEffect(pEffect, FinalMixBusIndex, buffer, bufferSize);
}

bool FinalMix::AppendEffect(EffectAux* pEffect, void* buffer, size_t bufferSize) {
    return OutputMixer::AppendEffect(pEffect, FinalMixBusIndex, buffer, bufferSize);
}

bool FinalMix::RemoveEffect(EffectBase* pEffect) {
    return OutputMixer::RemoveEffect(pEffect, FinalMixBusIndex);
}

bool FinalMix::RemoveEffect(EffectAux* pEffect) {
    return OutputMixer::RemoveEffect(pEffect, FinalMixBusIndex);
}

bool FinalMix::ClearEffect() {
    return OutputMixer::ClearEffect(FinalMixBusIndex);
}

bool FinalMix::IsEffectEnabled() const {
    return OutputMixer::IsEffectEnabled();
}

OutputReceiver::ReceiverType FinalMix::GetReceiverType() const {
    return ReceiverType::ReceiverType_FinalMix;
}

int32_t FinalMix::GetChannelCount() const {
    return m_ChannelCount;
}

int32_t FinalMix::GetBusCount() const {
    return FinalMixBusCount;
}

void FinalMix::AddReferenceCount(int32_t value) {
    m_ReferenceCount += value;
}

}  // namespace nn::atk

#endif
