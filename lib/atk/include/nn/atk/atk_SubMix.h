#pragma once

#include <nn/audio/audio_SubMixTypes.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_MoveValue.h>
#include <nn/atk/atk_OutputMixer.h>

namespace nn::atk {
namespace detail::driver {

class HardwareManager;

};

class SubMix : public OutputMixer {
    static const int32_t SubMixAlignment{8};

public:
    class SubMixParam {
    public:
        SubMixParam();

        void SetSrcBusCount(int32_t count);
        int32_t GetSrcBusCount() const;

        void SetSrcChannelCount(int32_t count);
        int32_t GetSrcChannelCount() const;

        void SetDstBusCount(int32_t count);
        int32_t GetDstBusCount() const;

        void SetDstChannelCount(int32_t count);
        int32_t GetDstChannelCount() const;

        void SetOutputReceiver(const OutputReceiver* pOutputReceiver);
        const OutputReceiver* GetOutputReceiver() const;

        void SetEffectEnabled(bool isEnabled);
        bool IsEffectEnabled() const;

        void SetSoundSendClampEnabled(int32_t bus, bool isEnabled);
        bool IsSoundSendClampEnabled(int32_t bus) const;

    private:
        int32_t m_SrcBusCount{1};
        int32_t m_SrcChannelCount{1};
        int32_t m_DstBusCount{1};
        int32_t m_DstChannelCount{1};
        const OutputReceiver* m_pOutputReceiver{};
        bool m_IsEffectEnabled{true};
        bool m_IsSoundSendClampEnabledArray[BusCountMax];
    };
    static_assert(sizeof(SubMixParam) == 0x38);

    class VolumeData {
    public:
        VolumeData();

        bool Update();

        void SetMute(bool isMute) {
            m_IsDirtyFlag = true;
            m_IsMute = isMute;
        }
        bool IsMute() const { return m_IsMute; }

        void SetVolume(float volume, int32_t fadeFrame) { m_Volume.SetTarget(volume, fadeFrame); }
        float GetVolume() const { return m_Volume.GetValue(); }

    private:
        detail::MoveValue<float, int32_t> m_Volume;
        bool m_IsMute{false};
        bool m_IsPrevMute{false};
        bool m_IsDirtyFlag{false};
    };
    static_assert(sizeof(VolumeData) == 0x14);

    SubMix();

    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount,
                                        bool isEffectEnabled);
#else
    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        int32_t dstBusCount, int32_t dstChannelCount,
                                        bool isEffectEnabled, bool isInternalCall);
#endif

    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        const OutputReceiver* pDstReceiver);
    static size_t GetRequiredMemorySize(int32_t srcBusCount, int32_t srcChannelCount,
                                        const OutputReceiver* pDstReceiver, bool isEffectEnabled);

    static size_t GetRequiredMemorySize(const SubMixParam& param);

    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, void* buffer, size_t bufferSize);
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, bool isEffectEnabled, void* buffer, size_t bufferSize);
#else
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount, int32_t dstBusCount,
                    int32_t dstChannelCount, bool isEffectEnabled, bool isInternalCall,
                    void* buffer, size_t bufferSize);
#endif
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount,
                    const OutputReceiver* pDstReceiver, void* buffer, size_t bufferSize);
    bool Initialize(int32_t srcBusCount, int32_t srcChannelCount,
                    const OutputReceiver* pDstReceiver, bool isEffectEnabled, void* buffer,
                    size_t bufferSize);

    bool Initialize(const SubMixParam& param, void* buffer, size_t bufferSize);

    void Finalize();

    bool IsRemovable() const;

    bool IsFinalizable() const;

    void Update();
    void UpdateBusMixVolume(int32_t bus);
    void UpdateChannelMixVolume(int32_t channel);
    void UpdateMixVolume(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel);

    void SetDestination(OutputReceiver* pReceiver);
    void ApplyDestination();

    float GetSend(int32_t srcBus, int32_t dstBus) const;
    void SetSend(int32_t srcBus, int32_t dstBus, float send);

    void SetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel,
                     float send);
    float GetSendImpl(int32_t srcBus, int32_t srcChannel, int32_t dstBus, int32_t dstChannel) const;

    void SetBusVolume(int32_t bus, float volume, int32_t fadeFrame);
    float GetBusVolume(int32_t bus) const;

    void SetBusMute(int32_t bus, bool isMute);
    bool IsBusMuted(int32_t bus) const;

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

    audio::SubMixType* GetAudioSubMixInstance() { return &m_SubMix; }
    const audio::SubMixType* GetAudioSubMixInstance() const { return &m_SubMix; }

    void AddReferenceCount(int32_t value) override;

    bool IsSoundSendClampEnabled(int32_t bus) const override;

    util::IntrusiveListNode m_Link;

private:
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    static size_t GetRequiredMemorySizeImpl(const SubMixParam& param);
#else
    static size_t GetRequiredMemorySizeImpl(const SubMixParam& param, bool isInternalCall);
#endif

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    bool InitializeImpl(const SubMixParam& param, void* buffer, size_t bufferSize);
#else
    bool InitializeImpl(const SubMixParam& param, void* buffer, size_t bufferSize,
                        bool isInternalCall);
#endif

    void MuteUnusedEffectChannel(ChannelIndex* effectChannelIndex, int32_t effectChannelCount,
                                 int32_t bus);

    bool IsMixBufferLinked(int32_t srcBus, int32_t srcChannel, int32_t dstBus,
                           int32_t dstChannel) const;

    audio::SubMixType m_SubMix;
    std::atomic_uint32_t m_ReferenceCount{0};
    OutputReceiver* m_pReceiver{};
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
    bool m_IsInitialized{false};
    bool m_IsUnusedEffectChannelMuted{false};
    bool m_IsAppliedOutputReceiver{false};
};
static_assert(sizeof(SubMix) == 0xf8);

}  // namespace nn::atk
