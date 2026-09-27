#pragma once

#include <atomic>
#include <nn/audio/audio_AudioRendererTypes.h>
#include <nn/audio/audio_FinalMixTypes.h>

#include <nn/atk/atk_OutputMixer.h>

namespace nn::atk {

class FinalMix : OutputMixer {
public:
    static size_t GetRequiredMemorySize(bool isEffectEnabled);

    bool Initialize(audio::AudioRendererConfig* pConfig, int32_t channelCount, bool isEffectEnabled,
                    void* buffer, size_t bufferSize);

    void Finalize(audio::AudioRendererConfig* pConfig);

    bool AppendEffect(EffectBase* pEffect, void* buffer, size_t bufferSize);
    bool AppendEffect(EffectAux* pEffect, void* buffer, size_t bufferSize);

    bool RemoveEffect(EffectBase* pEffect);
    bool RemoveEffect(EffectAux* pEffect);

    void ClearEffect();

    bool IsEffectEnabled() const;

    ReceiverType GetReceiverType() const override;
    int32_t GetChannelCount() const override;
    int32_t GetBusCount() const override;

    void AddReferenceCount(int32_t value) override;

    bool IsSoundSendClampEnabled(int32_t bus) const override;

private:
    audio::FinalMixType m_FinalMix;
    std::atomic_uint m_ReferenceCount;
    int32_t m_ChannelCount;
};
static_assert(sizeof(FinalMix) == 0x50);

}  // namespace nn::atk
