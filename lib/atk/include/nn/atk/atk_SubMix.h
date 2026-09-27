#pragma once

#include <nn/audio/audio_SubMixTypes.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_MoveValue.h>
#include <nn/atk/atk_OutputMixer.h>

namespace nn::atk {
namespace detail::driver {

class HardwareManager;

};

class SubMix : OutputMixer {
public:
    class SubMixParam {
    public:
        SubMixParam();

        void SetSrcBusCount(int32_t srcBusCount);
        int32_t GetSrcBusCount() const;

        void SetSrcChannelCount(int32_t srcChannelCount);
        int32_t GetSrcChannelCount() const;

        void SetDstBusCount(int32_t dstBusCount);
        int32_t GetDstBusCount() const;

        void SetDstChannelCount(int32_t dstChannelCount);
        int32_t GetDstChannelCount() const;

        void SetOutputReceiver(const OutputReceiver* pOutputReceiver);
        OutputReceiver* GetOutputReceiver() const;

        void SetEffectEnabled(bool isEffectEnabled);
        bool IsEffectEnabled() const;

        void SetSoundSendClampEnabled(int32_t index, bool IsSoundSendClampEnabled);
        bool GetSoundSendClampEnabled(int32_t index) const;

    private:
        int32_t m_SrcBusCount;
        int32_t m_SrcChannelCount;
        int32_t m_DstBusCount;
        int32_t m_DstChannelCount;
        OutputReceiver* m_pOutputReceiver;
        bool m_IsEffectEnabled;
        bool m_IsSoundSendClampEnabledArray[24];
    };
    static_assert(sizeof(SubMixParam) == 0x38);

    class VolumeData {
    public:
        VolumeData();

        bool Update();

    private:
        detail::MoveValue<float, int32_t> m_Volume;
        bool m_IsMute;
        bool m_IsPrevMute;
        bool m_IsDirtyFlag;
    };
    static_assert(sizeof(VolumeData) == 0x14);

    SubMix();

    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount);
    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount,
                                        bool isEffectEnabled);
    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount,
                                        bool isEffectEnabled, bool isInternalCall);

    static size_t GetRequiredMemorySizeImpl(const SubMixParam& param);

    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t dstBusCount,
                                        const OutputReceiver* pReceiver, bool isEffectEnabled);

    static size_t GetRequiredMemorySize(const SubMixParam& param);

    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, void* buffer, size_t bufferSize);
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, bool isEffectEnabled, void* buffer, size_t bufferSize);
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, bool isEffectEnabled, bool isInternalCall,
                    void* buffer, size_t bufferSize);

    bool InitializeImpl(const SubMixParam& param, void* buffer, size_t bufferSize);
    bool InitializeImpl(const SubMixParam& param, void* buffer, size_t bufferSize,
                        bool isInternalCall);

    bool Initialize(int32_t srcBusCount, int32_t dstBusCount, const OutputReceiver* pReceiver,
                    void* buffer, size_t bufferSize);
    bool Initialize(int32_t srcBusCount, int32_t dstBusCount, const OutputReceiver* pReceiver,
                    bool isEffectEnabled, void* buffer, size_t bufferSize);
    bool Initialize(const SubMixParam& param, void* buffer, size_t bufferSize);

    void Finalize();

    bool IsRemovable() const;

    void Update();

    void UpdateBusMixVolume(int32_t bus);
    void UpdateChannelMixVolume(int32_t bus);
    void UpdateMixVolume(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel);

    void SetDestination(OutputReceiver* pReceiver);
    void ApplyDestination();

    float GetSend(int32_t srcBus, int32_t dstBus) const;
    float GetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel) const;

    void SetSend(int32_t srcBus, int32_t dstBus, float send);
    void SetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel,
                     float send);

    void SetBusVolume(int32_t bus, float volume, int32_t fadeFrame);
    float GetBusVolume(int32_t bus) const;

    void SetBusMute(int32_t bus, bool isMute);
    bool IsBusMuted(int32_t bus) const;

    bool IsSoundSendClampEnabled(int32_t bus) const override;

    void SetChannelVolume(int32_t channel, float volume, int32_t fadeFrame);
    float GetChannelVolume(int32_t channel) const;

    void SetChannelMute(int32_t channel, bool isMute);
    bool IsChannelMuted(int32_t channel) const;

    void SetSubMixVolume(float volume, int32_t fadeFrame);
    float GetSubMixVolume() const;

    void SetSubMixMute(bool isMute);
    bool IsSubMixMuted() const;

    void SetMuteUnusedEffectChannel(bool isUnusedEffectChannelMuted);
    void IsUnusedEffectChannelMuted() const;

    void MuteUnusedEffectChannel(ChannelIndex* effectChannelIndex, int32_t effectChannelCount,
                                 int32_t bus);

    bool AppendEffect(EffectBase* pEffect, int32_t bus, void* buffer, size_t bufferSize);
    bool AppendEffect(EffectAux* pEffect, int32_t bus, void* buffer, size_t bufferSize);

    bool RemoveEffect(EffectBase* pEffect, int32_t bus);
    bool RemoveEffect(EffectAux* pEffect, int32_t bus);

    bool ClearEffect(int32_t bus);

    bool IsEffectEnabled() const;

    void AppendEffectImpl(EffectBase* pEffect, int32_t bus, void* buffer,
                          size_t bufferSize) override;
    void AppendEffectImpl(EffectAux* pEffect, int32_t bus, void* buffer,
                          size_t bufferSize) override;

    ReceiverType GetReceiverType() const override;
    int32_t GetChannelCount() const override;
    int32_t GetBusCount() const override;

    void AddReferenceCount(int32_t value) override;

private:
    friend detail::driver::HardwareManager;

    util::IntrusiveListNode m_Link;
    audio::SubMixType m_SubMix;
    std::atomic_uint m_ReferenceCount;
    OutputReceiver* m_pReceiver;
    VolumeData* m_pBusVolume;
    VolumeData* m_pChannelVolume;
    VolumeData m_SubMixVolume;
    bool* m_pIsSoundSendClampEnabledArray;
    float** m_ppSendVolume;
    int32_t m_ChannelCount;
    int32_t m_BusCount;
    int32_t m_ReceiverChannelCountMax;
    detail::fnd::CriticalSection m_VolumeLock;
    detail::fnd::CriticalSection m_DestinationLock;
    bool m_IsInitialized;
    bool m_IsUnusedEffectChannelMuted;
    bool m_IsAppliedOutputReceiver;
};
static_assert(sizeof(SubMix) == 0xf8);

}  // namespace nn::atk
