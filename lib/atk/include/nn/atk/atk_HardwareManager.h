#pragma once

#include <atomic>

#include <nn/os.h>

#include <nn/audio/audio_MemoryPoolTypes.h>
#include <nn/audio/audio_SinkTypes.h>

#include <nn/atk/atk_BiquadFilterCallback.h>
#include <nn/atk/atk_DeviceOutRecorder.h>
#include <nn/atk/atk_FinalMix.h>
#include <nn/atk/atk_LowLevelVoice.h>
#include <nn/atk/atk_SubMix.h>
#include <nn/atk/atk_Util.h>

namespace nn::atk::detail::driver {

class HardwareManager : public Util::Singleton<HardwareManager> {
public:
    using SubMixList =
        util::IntrusiveList<SubMix, util::IntrusiveListMemberNodeTraits<SubMix, &SubMix::m_Link>>;

    constexpr static uint32_t SoundFrameIntervalMsec = 5;
    constexpr static uint32_t SoundFrameIntervalUsec = 5000;

    constexpr static uint32_t DefaultRendererSampleRate = 48000;
    constexpr static uint32_t DefaultRendererUserEffectCount = 10;
    constexpr static uint32_t DefaultRendererVoiceCountMax = 96;

    constexpr static uint32_t DefaultRecordingAudioFrameCount = 8;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    constexpr static uint32_t AtkVoiceCountMax = 96;
#else
    constexpr static uint32_t AtkVoiceCountMax = 192;
#endif
    constexpr static uint32_t MixerCount = 3;
    constexpr static uint32_t ChannelCountMax = 6;
    constexpr static uint32_t BusCount = 4;

    constexpr static uint32_t DefaultRendererSampleCount = 140;
    constexpr static uint32_t DefaultRendererMixBufferCount = 30;
    constexpr static uint32_t DefaultRendererSubMixCount = 1;
    constexpr static uint32_t DefaultRendererSinkCount = 1;
    constexpr static uint32_t DefaultRendererPerformanceFrameCount = 0;
    constexpr static uint32_t DefaultRendererSystemEffectCount = 4;

    constexpr static uint32_t SubMixCountMax = 2;

    constexpr static uint32_t SubMixCountForAdditionalEffect = 1;
    constexpr static uint32_t ChannelCountForAdditionalEffect = 2;
    constexpr static uint32_t AuxBusCountForAdditionalEffect = 2;
    constexpr static uint32_t MixBufferCountForAdditionalEffect = 6;

    constexpr static bool DefaultRendererIsVoiceDropEnabled = false;

    class EffectAuxListScopedLock {
    public:
        EffectAuxListScopedLock();
        ~EffectAuxListScopedLock();
    };
    void LockEffectAuxList();
    void UnlockEffectAuxList();

    class EffectAuxListForFinalMixScopedLock {
    public:
        EffectAuxListForFinalMixScopedLock();
        ~EffectAuxListForFinalMixScopedLock();
    };
    void LockEffectAuxListForFinalMix();
    void UnlockEffectAuxListForFinalMix();

    class EffectAuxListForAdditionalSubMixScopedLock {
    public:
        EffectAuxListForAdditionalSubMixScopedLock();
        ~EffectAuxListForAdditionalSubMixScopedLock();
    };
    void LockEffectAuxListForAdditionalSubMix();
    void UnlockEffectAuxListForAdditionalSubMix();

    class SubMixListScopedLock {
    public:
        SubMixListScopedLock();
        ~SubMixListScopedLock();
    };
    void LockSubMixList();
    void UnlockSubMixList();

    class UpdateAudioRendererScopedLock {
    public:
        UpdateAudioRendererScopedLock() {
            HardwareManager::GetInstance().LockUpdateAudioRenderer();
        }

        ~UpdateAudioRendererScopedLock() {
            HardwareManager::GetInstance().UnlockUpdateAudioRenderer();
        }
    };

    class HardwareManagerParameter {
    public:
        void SetSubMixParameter(bool isStereoModeEnabled, bool isEffectEnabled,
                                bool isSubMixEnabled, bool isAdditionalEffectBusEnabled,
                                bool isAdditionalSubMixEnabled, bool isCustomSubMixEnabled,
                                int32_t customSubMixCount, int32_t customMixTotalChannelCount);

    private:
        int32_t m_RendererSampleRate;
        int32_t m_UserEffectCount;
        int32_t m_VoiceCount;
        int32_t m_RecordingAudioFrameCount;
        int32_t m_SubMixCount;
        int32_t m_MixBufferCount;
        bool m_IsProfilerEnabled;
        bool m_IsAdditionalEffectBusEnabled;
        bool m_IsAdditionalSubMixEnabled;
        bool m_IsEffectEnabled;
        bool m_IsRecordingEnabled;
        bool m_IsUserCircularBufferSinkEnabled;
        bool m_IsPresetSubMixEnabled;
        bool m_IsStereoModeEnabled;
        bool m_IsSoundThreadEnabled;
        bool m_IsVoiceDropEnabled;
        bool m_IsCompatibleDownMixSettingEnabled;
        bool m_IsPreviousSdkVersionLowPassFilterCompatible;
        bool m_IsUnusedEffectChannelMutingEnabled;
        bool m_IsCompatibleBusVolumeEnabled;
        bool m_IsUserThreadRenderingEnabled;
        bool m_IsCustomSubMixEnabled;
        bool m_IsMemoryPoolAttachCheckEnabled;
    };
    static_assert(sizeof(HardwareManagerParameter) == 0x2c);

    HardwareManager();

    void ResetParameters();

    audio::MemoryPoolState GetMemoryPoolState(audio::MemoryPoolType* pPool);

    void
    SetupAudioRendererParameter(audio::AudioRendererParameter* audioRendererParameter,
                                const HardwareManagerParameter& hardwareManagerParameter) const;

    size_t GetRequiredMemSize(const HardwareManagerParameter& hardwareManagerParameter) const;
    size_t GetRequiredMemSizeForMemoryPool(int32_t voiceCount) const;
    size_t GetRequiredRecorderWorkBufferSize(
        const HardwareManagerParameter& hardwareManagerParameter) const;
    size_t GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(
        const HardwareManagerParameter& hardwareManagerParameter) const;
    size_t GetRequiredCircularBufferSinkBufferSize(
        const HardwareManagerParameter& hardwareManagerParameter) const;

    int32_t GetChannelCountMax() const;

    bool RegisterRecorder(DeviceOutRecorder* pRecorder);
    bool UnregisterRecorder(DeviceOutRecorder* pRecorder);
    void UpdateRecorder();

    uint64_t ReadRecordingCircularBufferSink(void* buffer, size_t bufferSize);
    audio::CircularBufferSinkType* AllocateRecordingCircularBufferSink();
    void FreeRecordingCircularBufferSink(audio::CircularBufferSinkType* circularBufferSink);
    void StartRecordingCircularBufferSink();

    void StopUserCircularBufferSink();
    void StartUserCircularBufferSink(bool isForceStartMode);
    uint64_t ReadUserCircularBufferSink(void* buffer, size_t bufferSize);

    void AttachMemoryPool(audio::MemoryPoolType* pPool, void* buffer, size_t bufferSize,
                          bool isSoundThreadEnabled);

    Result RequestUpdateAudioRenderer();

    void DetachMemoryPool(audio::MemoryPoolType* pPool, bool isSoundThreadEnabled);

    void LockUpdateAudioRenderer() { m_UpdateAudioRendererLock.Lock(); }

    void UnlockUpdateAudioRenderer() { m_UpdateAudioRendererLock.Unlock(); }

    void ExecuteAudioRendererRendering();

    void WaitAudioRendererEvent();

    int32_t* GetDroppedLowLevelVoiceCount() const;

    size_t
    GetRequiredPerformanceFramesBufferSize(HardwareManagerParameter* hardwareManagerParameter);

    Result Initialize(void* buffer, size_t bufferSize, void* memoryPoolBuffer,
                      size_t memoryPoolBufferSize, void* circularBufferSinkBuffer,
                      size_t circularBufferSinkBufferSize, HardwareManagerParameter* parameter);

    void SetBiquadFilterCallback(int32_t, const BiquadFilterCallback* callback);
    void SetOutputMode(OutputMode mode, OutputDevice device);

    void UpdateEndUserOutputMode();

    void Finalize();

    void Update();
    void UpdateEffect();

    void SuspendAudioRenderer();
    void ResumeAudioRenderer();

    bool TimedWaitAudioRendererEvent(TimeSpan timeout);

    void SetAudioRendererRenderingTimeLimit(int32_t timeLimit);
    int32_t GetAudioRendererRenderingTimeLimit();

    void PrepareReset();

    bool IsResetReady() const;

    audio::AudioRendererConfig& GetAudioRendererConfig() { return m_Config; }

    void AddSubMix(SubMix* pSubMix);
    void RemoveSubMix(SubMix* pSubMix);

    SubMix* GetSubMix(int32_t subMixNumber);
    SubMix* GetSubMix(int32_t subMixNumber) const;

    int32_t GetSubMixCount() const;
    int32_t GetChannelCount() const;
    float GetOutputVolume() const;

    void SetOutputDeviceFlag(int32_t, uint8_t);

    void SetMasterVolume(float volume, int32_t fadeFrames);
    void SetSrcType(SampleRateConverterType sampleRateConverter);

    size_t GetRequiredEffectAuxBufferSize(const EffectAux* pEffect) const;

    void SetAuxBusVolume(AuxBus bus, float volume, int32_t fadeFrames, int32_t subMixIndex);
    float GetAuxBusVolume(AuxBus bus, int32_t subMixIndex) const;

    void SetMainBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannel,
                                                    int32_t dstChannel);
    float GetMainBusChannelVolumeForAdditionalEffect(int32_t srcChannel, int32_t dstChannel) const;

    void SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, float volume, int32_t srcChannel,
                                                   int32_t dstChannel);
    float GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int32_t srcChannel,
                                                    int32_t dstChannel) const;

    static void FlushDataCache(void* address, size_t length);

    SampleRateConverterType GetSrcType() const { return m_SrcType; }

    bool IsEffectInitialized() const { return m_IsInitializedEffect; }
    bool IsPresetSubMixEnabled() const { return m_IsPresetSubMixEnabled; }
    bool IsAdditionalEffectEnabled() const { return m_IsAdditionalEffectEnabled; }

    OutputMode GetOutputMode(OutputDevice device) const { return m_OutputMode[device]; }

    LowLevelVoiceAllocator& GetLowLevelVoiceAllocator() { return m_LowLevelVoiceAllocator; }

private:
    bool m_IsInitialized;
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    audio::AudioRendererHandle* m_RendererHandle;
#else
    audio::AudioRendererHandle m_RendererHandle;
#endif
    audio::AudioRendererConfig m_Config;
    os::SystemEvent m_SystemEvent;
    int32_t m_AudioRendererSuspendCount;
    std::atomic_ulong m_AudioRendererUpdateCount;
    void* m_pAudioRendererWorkBuffer;
    void* m_pAudioRendererConfigWorkBuffer;
    OutputMode m_OutputMode[OutputDevice_Count];
    OutputMode m_EndUserOutputMode[OutputDevice_Count];
    SampleRateConverterType m_SrcType;
    MoveValue<float, int32_t> m_MasterVolume;
    MoveValue<float, int32_t> m_VolumeForReset;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    SubMixList _1[14];  // There's some kind of intrusive list array here, but it's not SubMix
                        // (because SubMix does not exist on versions prior to 4.0.0)
#endif
    BiquadFilterCallback* m_BiquadFilterCallbackTable[BiquadFilterType_Max +
                                                      1];  // SMO: 0x1e8 = 488, BTD5: 0x108 = 264
    uint8_t m_OutputDeviceFlag[OutputLineIndex_Max];
    LowLevelVoiceAllocator m_LowLevelVoiceAllocator;  // SMO: 0x608 = 1544, BTD5: 0x528 = 1320
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    FinalMix m_FinalMix;
    SubMix m_SubMix[SubMixCountMax];
    SubMix m_AdditionalSubMix;
    SubMixList m_SubMixList;
    fnd::CriticalSection m_SubMixListLock;
#endif
    audio::AudioRendererParameter m_AudioRendererParameter;
    audio::DeviceSinkType m_Sink;
    MoveValue<float, int32_t> m_AuxUserVolume[MixerCount];
    MoveValue<float, int32_t> m_AuxUserVolumeForAdditionalEffect[AuxBusCountForAdditionalEffect];
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    uint8_t _0[298];
#endif
    bool m_IsInitializedEffect;  // SMO: 0x7f0 = 2040, BTD5: 0x948 = 2376
    bool m_IsPresetSubMixEnabled;
    bool m_IsAdditionalEffectEnabled;
    bool m_IsAdditionalSubMixEnabled;
    bool m_IsStereoModeEnabled;
    bool m_IsInitializedSoundThread;
    bool m_IsPreviousSdkVersionLowPassFilterCompatible;
    bool m_IsMemoryPoolAttachCheckEnabled;
    fnd::CriticalSection m_UpdateAudioRendererLock;  // SMO: 0x7f8 = 2048, BTD5: 0x950 = 2384
    fnd::CriticalSection m_UpdateHardwareManagerLock;
    fnd::CriticalSection m_EffectAuxListLock;
    fnd::CriticalSection m_EffectAuxListForFinalMixLock;
    fnd::CriticalSection m_EffectAuxListForAdditionalSubMixLock;
    audio::CircularBufferSinkType m_RecordingCircularBufferSink;
    CircularBufferSinkState m_RecordingCircularBufferSinkState;
    audio::MemoryPoolType m_RecordingCircularBufferSinkMemoryPool;
    void* m_RecordingCircularBufferSinkBuffer;
    bool m_IsRecordingCircularBufferSinkAllocated;
    void* m_RecordingBuffer;
    size_t m_RecordingBufferSize;
    DeviceOutRecorder* m_pRecorder;
    audio::CircularBufferSinkType m_UserCircularBufferSink;
    audio::MemoryPoolType m_UserCircularBufferSinkMemoryPool;
    void* m_UserCircularBufferSinkBuffer;
    size_t m_UserCircularBufferSinkBufferSize;
    CircularBufferSinkState m_UserCircularBufferSinkState;
    bool m_IsCompatibleBusVolumeEnabled;
    bool m_IsUserThreadRenderingEnabled;
};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(HardwareManager) == 0xa58);
#endif

}  // namespace nn::atk::detail::driver
