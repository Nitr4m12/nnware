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
    static const int32_t ChannelCountMax{ChannelIndex_Count};
    static const int32_t EffectAuxAlignment{8};

    struct UpdateSamplesArg {
        int32_t sampleCountPerAudioFrame;
        int32_t sampleRate;
        int32_t audioFrameCount;
        int32_t channelCount;
        int32_t readSampleCount;
        OutputMode outputMode;
    };
    static_assert(sizeof(UpdateSamplesArg) == 0x18);

    struct BufferSet {
        void* sendBuffer;
        void* returnBuffer;
        void* readBuffer;
    };
    static_assert(sizeof(BufferSet) == 0x18);

    EffectAux();
    virtual ~EffectAux();

    virtual bool Initialize();
    virtual void Finalize();

    virtual void OnChangeOutputMode();

    bool SetChannelCount(int32_t channelCount);
    int32_t GetChannelCount() const;

    bool SetChannelIndex(const ChannelIndex* pChannel, int32_t channelCount);
    void GetChannelIndex(ChannelIndex* pChannel, int32_t channelCount) const;

    bool SetAudioFrameCount(int32_t audioFrameCount);
    int32_t GetAudioFrameCount() const;

    bool IsRemovable() const;
    bool IsClearable();

    bool IsEnabled() const;
    void SetEnabled(bool isEnabled);

    int32_t GetChannelSettingCountMax() const { return ChannelCountMax; }

    size_t GetRequiredMemSize(const audio::AudioRendererParameter& parameter) const;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter, audio::FinalMixType* pFinalMix);
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter, audio::SubMixType* pSubMix);
#else
    bool AddEffect(audio::AudioRendererConfig* pConfig,
                   const audio::AudioRendererParameter& parameter, OutputMixer* pOutputMixer);
#endif

    void SetEffectInputOutput(const int8_t* input, const int8_t* output, int32_t inputCount,
                              int32_t outputCount);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    void RemoveEffect(audio::AudioRendererConfig* pConfig, audio::FinalMixType* pFinalMix);
    void RemoveEffect(audio::AudioRendererConfig* pConfig, audio::SubMixType* pSubMix);
#else
    void RemoveEffect(audio::AudioRendererConfig* pConfig, OutputMixer* pOutputMixer);
#endif

    void SetEffectBuffer(void* effectBuffer, size_t effectBufferSize);

    void Update();

protected:
    virtual void UpdateSamples(int32_t* readBuffer, const UpdateSamplesArg& arg) = 0;

private:
    void SplitEffectBuffer(BufferSet* pOutBufferSet, void* effectBuffer,
                           size_t sendReturnBufferSize);

    void ResetChannelIndex();

public:
    util::IntrusiveListNode m_AuxLinkNode;

private:
    audio::AuxType m_AuxType;
    std::atomic_uint64_t m_AudioRendererUpdateCountWhenAddedAux{0};
    int32_t m_AudioFrameCount{4};
    int32_t m_ChannelCount{4};
    bool m_IsActive{false};
    bool m_IsEnabled{false};
    void* m_EffectBuffer{};
    size_t m_EffectBufferSize{0};
    int32_t* m_AuxReadBuffer{};
    ChannelIndex m_ChannelSetting[ChannelCountMax];
};
static_assert(sizeof(EffectAux) == 0x68);

}  // namespace nn::atk
