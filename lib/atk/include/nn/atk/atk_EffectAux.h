#pragma once

#include <atomic>

#include <nn/util.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/audio/audio_AudioRendererTypes.h>
#include <nn/audio/audio_EffectTypes.h>
#include <nn/audio/audio_FinalMixTypes.h>
#include <nn/audio/audio_SubMixTypes.h>

#include <nn/atk/atk_Global.h>

namespace nn::atk {

class OutputMixer;

class EffectAux {
public:
    constexpr static int32_t ChannelCountMax = 6;

    struct BufferSet {
        void* sendBuffer;
        void* returnBuffer;
        void* readBuffer;
    };
    static_assert(sizeof(BufferSet) == 0x18);

    struct UpdateSamplesArg {
        int32_t sampleCountPerAudioFrame;
        int32_t sampleRate;
        int32_t audioFrameCount;
        int32_t channelCount;
        int32_t readSampleCount;
        OutputMode outputMode;
    };
    static_assert(sizeof(UpdateSamplesArg) == 0x18);

    EffectAux();
    virtual ~EffectAux();
    virtual void Initialize();
    virtual void Finalize();
    virtual void OnChangeOutputMode();
    virtual void UpdateSamples(int32_t, const UpdateSamplesArg&);

    void ResetChannelIndex();

    size_t GetRequiredMemSize(const audio::AudioRendererParameter& parameter) const;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter,
                   audio::FinalMixType* pFinalMixType);
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter, audio::SubMixType* pSubMixType);
#else
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter, OutputMixer* pOutputMixer);
#endif

    void SplitEffectBuffer(BufferSet* pBufferSet, void* effectBuffer, size_t effectBufferSize);

    void SetEffectInputOutput(const int8_t* input, const int8_t* output, int32_t inputCount,
                              int32_t outputCount);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void RemoveEffect(audio::AudioRendererConfig* pConfig, audio::FinalMixType* pFinalMixType);
    void RemoveEffect(audio::AudioRendererConfig* pConfig, audio::SubMixType* pSubMixType);
#else
    void RemoveEffect(audio::AudioRendererConfig* pConfig, OutputMixer* pOutputMixer);
#endif

    bool SetChannelCount(int32_t channelCount);
    bool SetChannelIndex(const ChannelIndex* pChannel, int32_t channelCount);

    int32_t GetChannelCount() const;
    void GetChannelIndex(ChannelIndex* pChannel, int32_t channelCount) const;

    bool SetAudioFrameCount(int32_t audioFrameCount);
    int32_t GetAudioFrameCount() const;

    bool IsRemovable() const;
    bool IsClearable();
    bool IsEnabled() const;

    void SetEnabled(bool isEnabled);
    void SetEffectBuffer(void* effectBuffer, size_t effectBufferSize);

    void Update();

private:
    friend OutputMixer;

    util::IntrusiveListNode m_AuxLinkNode;
    audio::AuxType m_AuxType;
    std::atomic_uint64_t m_AudioRendererUpdateCountWhenAddedAux;
    int32_t m_AudioFrameCount;
    int32_t m_ChannelCount;
    bool m_IsActive;
    bool m_IsEnabled;
    void* m_EffectBuffer;
    size_t m_EffectBufferSize;
    int32_t* m_AuxReadBuffer;
    ChannelIndex m_ChannelSetting[ChannelCountMax];
};
static_assert(sizeof(EffectAux) == 0x50 + sizeof(ChannelIndex) * EffectAux::ChannelCountMax);

}  // namespace nn::atk
