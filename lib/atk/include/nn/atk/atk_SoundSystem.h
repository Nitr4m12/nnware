#pragma once

#include <cstddef>
#include <cstdint>
#include <nn/audio/audio_MemoryPoolTypes.h>
#include <nn/time.h>
#include <vapours/results/results_common.hpp>

#include <nn/atk/atk_AudioRendererPerformanceReader.h>
#include <nn/atk/atk_EffectAux.h>
#include <nn/atk/atk_EffectBase.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/atk/atk_ThreadInfoReader.h>

namespace nn::atk {

struct SoundSystem {
    static bool g_IsInitialized;
    static bool g_IsStreamLoadWait;
    static bool g_IsEnterSleep;
    static bool g_IsInitializedDriverCommandManager;

    static uintptr_t g_LoadThreadStackPtr;
    static size_t g_LoadThreadStackSize;

    static uintptr_t g_SoundThreadStackPtr;
    static size_t g_SoundThreadStackSize;

    static uintptr_t g_PerformanceFrameBuffer;
    static size_t g_PerformanceFrameBufferSize;

    static size_t g_SoundThreadCommandBufferSize;
    static size_t g_TaskThreadCommandBufferSize;
    static size_t g_VoiceCommandBufferSize;

    static uintptr_t g_MemoryPoolForSoundSystem;

    static int32_t g_RendererSampleRate;
    static int32_t g_CustomSubMixSubMixCount;
    static int32_t g_CustomSubMixMixBufferCount;

    static bool g_IsProfilerEnabled;
    static bool g_IsDetailSoundThreadProfilerEnabled;
    static bool g_IsAdditionalEffectBusEnabled;
    static bool g_IsAdditionalSubMixEnabled;
    static bool g_IsEffectEnabled;
    static bool g_IsRecordingEnabled;
    static bool g_IsCircularBufferSinkEnabled;
    static bool g_IsCircularBufferSinkWarningDisplayed;
    static bool g_IsVoiceDropEnabled;
    static bool g_IsPreviousSdkVersionLowPassFilterCompatible;
    static bool g_IsUnusedEffectChannelMutingEnabled;
    static bool g_IsUserThreadRenderingEnabled;
    static bool g_IsCustomSubMixEnabled;
    static bool g_IsMemoryPoolAttachCheckEnabled;
    static bool g_IsBusMixVolumeEnabled;
    static bool g_IsVolumeThroughModeEnabled;

    constexpr static uint32_t VoiceCountMax = 96;
    constexpr static uint32_t WorkMemoryAlignSize = 4;
    constexpr static uint32_t VoiceCommandManagerCountMax = 2;
    constexpr static uint32_t SoundThreadIntervalUsec = 5000;

    constexpr static int32_t g_TaskThreadFsPriority = 1;
    static bool g_IsStreamOpenFailureHalt;
    constexpr static bool g_IsTaskThreadEnabled = true;
    constexpr static bool g_IsManagingMemoryPool = true;
    constexpr static uint32_t g_UserEffectCount = 10;
    constexpr static bool g_IsSubMixEnabled = true;
    constexpr static bool g_IsPresetSubMixEnabled = true;
    constexpr static bool g_IsStereoModeEnabled = true;
    constexpr static bool g_IsSoundThreadEnabled = true;
    constexpr static int32_t g_BusCountMax = 4;

    struct SoundSystemParam {
        constexpr static uint32_t DefaultSoundThreadPriority = 0;
        constexpr static uint32_t DefaultTaskThreadPriority = 0;
        constexpr static uint32_t DefaultSoundThreadStackSize = 0x4000;
        constexpr static uint32_t DefaultTaskThreadStackSize = 0x4000;
        constexpr static uint32_t DefaultSoundThreadCommandBufferSize = 0x20000;
        constexpr static uint32_t DefaultTaskThreadCommandBufferSize = 0x2000;
        constexpr static uint32_t DefaultUserEffectCount = 10;
        constexpr static uint32_t DefaultVoiceCountMax = 96;
        constexpr static uint32_t DefaultSoundThreadCoreNumber = 0;
        constexpr static uint32_t DefaultTaskThreadCoreNumber = 0;
        constexpr static uint32_t DefaultRecordingAudioFrameCount = 8;

        SoundSystemParam();

        int32_t soundThreadPriority{4};
        size_t soundThreadStackSize{0x4000};
        size_t soundThreadCommandBufferSize{0x20000};
        size_t voiceCommandBufferSize;
        int32_t taskThreadPriority{3};
        size_t taskThreadStackSize{0x4000};
        size_t taskThreadCommandBufferSize{0x2000};
        FsPriority taskThreadFsPriority{FsPriority_Normal};
        bool enableNwRenderer{};
        uint32_t nwVoiceSynthesizeBufferCount;
        int32_t rendererSampleRate{48000};
        int32_t effectCount{10};
        int32_t voiceCountMax{96};
        int32_t voiceCommandWaveBufferPacketCount{0x200};
        bool enableProfiler{false};
        bool enableDetailSoundThreadProfile{false};
        bool enableRecordingFinalOutputs{false};
        bool enableCircularBufferSink{false};
        int32_t recordingAudioFrameCount{8};
        int32_t soundThreadCoreNumber;
        int32_t taskThreadCoreNumber;
        bool enableAdditionalEffectBus{false};
        bool enableAdditionalSubMix{false};
        bool enableTaskThread{true};
        bool enableSoundThread{true};
        bool enableMemoryPoolManagement{true};
        bool enableCircularBufferSinkBufferManagement{true};
        bool enableEffect{true};
        bool enableSubMix{true};
        bool enableStereoMode{false};
        bool enableVoiceDrop{true};
        bool enableCompatibleDownMixSetting{false};
        bool enableCompatibleLowPassFilter;
        bool enableUnusedEffectChannelMuting;
        bool enableCompatibleBusVolume;
        bool enableUserThreadRendering;
        bool enableCustomSubMix;
        int32_t subMixCount{0};
        int32_t subMixTotalChannelCount{0};
        int32_t mixBufferCount{-1};
        int32_t busCountMax{4};
        bool enableMemoryPoolAttachCheck{false};
        bool enableBusMixVolume{false};
        bool enableVolumeThroughMode{false};
    };
    static_assert(sizeof(SoundSystemParam) == 0x88);

    struct InitializeBufferSet {
        uintptr_t workMem;
        size_t workMemSize;
        uintptr_t memoryPoolMem;
        size_t memoryPoolMemSize;
        uintptr_t circularBufferSinkMem;
        size_t circularBufferSinkMemSize;
    };
    static_assert(sizeof(InitializeBufferSet) == 0x30);

    static size_t GetRequiredMemSize(const SoundSystemParam& param);
    static size_t GetRequiredMemSizeForCircularBufferSink(const SoundSystemParam& param);
    static size_t GetRequiredMemSizeForMemoryPool(const SoundSystemParam& param);

    static void SetupHardwareManagerParameter(
        detail::driver::HardwareManager::HardwareManagerParameter* pOutValue,
        SoundSystemParam* parameter);

    static bool detail_InitializeSoundSystem(Result* pOutResult, const SoundSystemParam& param,
                                             const InitializeBufferSet& bufferSet);
    static void detail_InitializeDriverCommandManager(const SoundSystemParam& param, uint64_t,
                                                      uint64_t, uint64_t, uint64_t);

    static bool Initialize(SoundSystemParam* param, uintptr_t workMem, size_t workMemSize);
    static bool Initialize(Result* pOutResult, const SoundSystemParam& param, uintptr_t workMem,
                           size_t workMemSize);

    static void SetupInitializeBufferSet(InitializeBufferSet* pOutValue, SoundSystemParam* param,
                                         InitializeBufferSet* bufferSet);

    static bool Initialize(SoundSystemParam* param, uintptr_t workMem, size_t workMemSize,
                           uintptr_t memoryPoolMem, size_t memoryPoolMemSize);
    static bool Initialize(Result* pOutResult, const SoundSystemParam& param, uintptr_t workMem,
                           size_t workMemSize, uintptr_t memoryPoolMem, size_t memoryPoolMemSize);

    static bool Initialize(const SoundSystemParam& param, const InitializeBufferSet& bufferSet);
    static bool Initialize(Result* pOutResult, const SoundSystemParam& param,
                           const InitializeBufferSet& bufferSet);

    static void Finalize();

    static void SetSoundThreadBeginUserCallback(void (*threadBeginUserCallback)(uint64_t),
                                                uintptr_t threadBeginUserCallbackArg);
    static void ClearSoundThreadBeginUserCallback();

    static void SetSoundThreadEndUserCallback(void (*threadEndUserCallback)(uint64_t),
                                              uintptr_t threadEndUserCallbackArg);
    static void ClearSoundThreadEndUserCallback();

    static bool IsInitialized();

    static void SuspendAudioRenderer(TimeSpan timeSpan);
    static void ResumeAudioRenderer(TimeSpan timeSpan);

    static void ExecuteRendering();

    static void AttachMemoryPool(audio::MemoryPoolType* pMemoryPool, void* address, size_t size);
    static void DetachMemoryPool(audio::MemoryPoolType* pMemoryPool);

    static void DumpMemory();

    static size_t GetAudioRendererBufferSize();

    static void SetupHardwareManagerParameterFromCurrentSetting(
        detail::driver::HardwareManager::HardwareManagerParameter* pHardwareManagerParameter);

    static size_t GetRecorderBufferSize();
    static size_t GetUserCircularBufferSinkBufferSize();
    static size_t GetLowLevelVoiceAllocatorBufferSize();
    static size_t GetMultiVoiceManagerBufferSize();
    static size_t GetChannelManagerBufferSize();
    static size_t GetDriverCommandBufferSize();
    static size_t GetAllocatableDriverCommandSize();
    static size_t GetAllocatedDriverCommandBufferSize();
    static size_t GetAllocatedDriverCommandCount();

    static void RegisterAudioRendererPerformanceReader(
        AudioRendererPerformanceReader& audioRendererPerformanceReader);

    static bool AppendEffect(AuxBus auxBus, EffectBase* pEffectBase, void* buffer,
                             size_t bufferSize);
    static bool AppendEffect(AuxBus auxBus, EffectBase* pEffectBase, void* buffer,
                             size_t bufferSize, OutputDevice device);
    static bool AppendEffect(AuxBus auxBus, EffectBase* pEffectBase, void* buffer,
                             size_t bufferSize, OutputDevice device, int32_t subMixNumber);

    static bool AppendEffect(AuxBus auxBus, EffectAux* pEffectAux, void* buffer, size_t bufferSize);
    static bool AppendEffect(AuxBus auxBus, EffectAux* pEffectAux, void* buffer, size_t bufferSize,
                             OutputDevice device);
    static bool AppendEffect(AuxBus auxBus, EffectAux* pEffectAux, void* buffer, size_t bufferSize,
                             OutputDevice device, int32_t subMixNumber);

    static bool AppendEffectToFinalMix(EffectAux* pEffectAux, void* buffer, size_t bufferSize);
    static bool AppendEffectToAdditionalSubMix(EffectAux* pEffectAux, void* buffer,
                                               size_t bufferSize);

    static size_t GetRequiredEffectAuxBufferSize(const EffectAux* pEffectAux);

    static void RemoveEffect(AuxBus auxBus, EffectBase* pEffectBase);
    static void RemoveEffect(AuxBus auxBus, EffectBase* pEffectBase, OutputDevice outputDevice);
    static void RemoveEffect(AuxBus auxBus, EffectBase* pEffectBase, OutputDevice outputDevice,
                             int32_t subMixNumber);

    static void RemoveEffect(AuxBus auxBus, EffectAux* pEffectAux);
    static void RemoveEffect(AuxBus auxBus, EffectAux* pEffectAux, OutputDevice outputDevice);
    static void RemoveEffect(AuxBus auxBus, EffectAux* pEffectAux, OutputDevice outputDevice,
                             int32_t subMixNumber);

    static void RemoveEffectFromFinalMix(EffectAux* pEffectAux);
    static void RemoveEffectFromAdditionalSubMix(EffectAux* pEffectAux);

    static void ClearEffect(AuxBus auxBus);
    static void ClearEffect(AuxBus auxBus, OutputDevice outputDevice);
    static void ClearEffect(AuxBus auxBus, OutputDevice outputDevice, int32_t subMixNumber);

    static void ClearEffectFromFinalMix();
    static void ClearEffectFromAdditionalSubMix();

    static bool IsClearEffectFinished(AuxBus auxBus);
    static bool IsClearEffectFinished(AuxBus auxBus, OutputDevice outputDevice);
    static bool IsClearEffectFinished(AuxBus auxBus, OutputDevice outputDevice,
                                      int32_t subMixNumber);

    static bool IsClearEffectFromFinalMixFinished();
    static bool IsClearEffectFromAdditionalSubMixFinished();

    static void SetAuxBusVolume(AuxBus auxBus, float volume, TimeSpan timeSpan);
    static void SetAuxBusVolume(AuxBus auxBus, float volume, TimeSpan timeSpan, int32_t);

    static float GetAuxBusVolume(AuxBus auxBus);
    static float GetAuxBusVolume(AuxBus auxBus, int32_t subMixIndex);

    static void SetMainBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannel,
                                                           int32_t dstChannel);
    static float GetMainBusChannelVolumeForAdditionalEffect(int32_t srcChannel, int32_t dstChannel);

    static void SetAuxBusChannelVolumeForAdditionalEffect(AuxBus auxBus, float volume,
                                                          int32_t srcChannel, int32_t dstChannel);
    static float GetAuxBusChannelVolumeForAdditionalEffect(AuxBus auxBus, int32_t srcChannel,
                                                           int32_t dstChannel);

    static void SetAllAuxBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannel,
                                                             int32_t dstChannel);
    static void SetAllBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannel,
                                                          int32_t dstChannel);

    static void VoiceCommandProcess(UpdateType updateType, uint32_t);
    static void VoiceCommandProcess(uint32_t);

    static void VoiceCommandUpdate();

    static size_t GetPerformanceFrameBufferSize();

    static int32_t GetDroppedLowLevelVoiceCount();

    static void RegisterSoundThreadUpdateProfileReader(AtkProfileReader<SoundThreadUpdateProfile>&);
    static void
    UnregisterSoundThreadUpdateProfileReader(AtkProfileReader<SoundThreadUpdateProfile>&);

    static void RegisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder&);
    static void UnregisterSoundThreadInfoRecorder(detail::ThreadInfoRecorder&);

    static bool ReadCircularBufferSink(void* buffer, size_t bufferSize);

    static size_t GetCircularBufferSinkBufferSize();

    static uint32_t GetRendererSampleCount();
    static uint32_t GetRendererChannelCountMax();

    static void StopCircularBufferSink();
    static void StartCircularBufferSink();

    static CircularBufferSinkState GetCircularBufferSinkState();
    static detail::SoundInstanceConfig GetSoundInstanceConfig();

    static bool detail_IsStreamOpenFailureHaltEnabled() {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        return true;
#else
        return g_IsStreamOpenFailureHalt;
#endif
    }
};

}  // namespace nn::atk
