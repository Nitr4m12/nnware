#pragma once

#include <nn/atk/atk_HardwareManager.h>
#include <nn/atk/atk_MmlParser.h>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/atk_TaskManager.h>

namespace nn::atk {

struct SoundSystem {
    struct SoundSystemParam {
        static const int32_t DefaultRendererSampleRate{48000};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        static const int32_t DefaultSoundThreadPriority{0};
        static const int32_t DefaultTaskThreadPriority{0};
#else
        static const int32_t DefaultSoundThreadPriority{4};
        static const int32_t DefaultTaskThreadPriority{3};
#endif
        static const int32_t DefaultUserEffectCount{10};
        static const int32_t DefaultVoiceCountMax{96};
        static const uint32_t DefaultSoundThreadStackSize{16384};
        static const uint32_t DefaultTaskThreadStackSize{16384};
        static const uint32_t DefaultSoundThreadCommandBufferSize{131072};
        static const uint32_t DefaultTaskThreadCommandBufferSize{8192};
        static const int32_t DefaultRecordingAudioFrameCount{8};
        static const int32_t DefaultTaskThreadCoreNumber{0};
        static const int32_t DefaultSoundThreadCoreNumber{0};
        static const int32_t DefaultBusCountMax{4};
        static const int32_t InvalidMixBufferCount{-1};
        static const FsPriority DefaultTaskThreadFsPriority{FsPriority_Normal};

        SoundSystemParam();

        int32_t soundThreadPriority{DefaultSoundThreadPriority};
        size_t soundThreadStackSize{DefaultSoundThreadStackSize};
        size_t soundThreadCommandBufferSize{DefaultSoundThreadCommandBufferSize};
        size_t voiceCommandBufferSize;
        int32_t taskThreadPriority{DefaultTaskThreadPriority};
        size_t taskThreadStackSize{DefaultTaskThreadStackSize};
        size_t taskThreadCommandBufferSize{DefaultTaskThreadCommandBufferSize};
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        FsPriority taskThreadFsPriority{FsPriority_Normal};
#endif
        bool enableNwRenderer;
        uint32_t nwVoiceSynthesizeBufferCount;
        int32_t rendererSampleRate{DefaultRendererSampleRate};
        int32_t effectCount{DefaultUserEffectCount};
        int32_t voiceCountMax{DefaultVoiceCountMax};
        int32_t voiceCommandWaveBufferPacketCount{512};
        bool enableProfiler{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        bool enableDetailSoundThreadProfile{false};
#endif
        bool enableRecordingFinalOutputs{false};
        bool enableCircularBufferSink{false};
        int32_t recordingAudioFrameCount{DefaultRecordingAudioFrameCount};
        int32_t soundThreadCoreNumber{DefaultSoundThreadCoreNumber};
        int32_t taskThreadCoreNumber{DefaultTaskThreadCoreNumber};
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
        bool enableCompatibleLowPassFilter{false};
        bool enableUnusedEffectChannelMuting{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        bool enableCompatibleBusVolume{false};
#endif
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        bool enableMemoryPoolAttachCheck{false};
#endif
        bool enableUserThreadRendering{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        bool enableCustomSubMix{false};
        int32_t subMixCount{0};
        int32_t subMixTotalChannelCount{0};
#endif
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        int32_t mixBufferCount{InvalidMixBufferCount};
        int32_t busCountMax{DefaultBusCountMax};
#endif
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        bool enableMemoryPoolAttachCheck{false};
        bool enableBusMixVolume{false};
        bool enableVolumeThroughMode{false};
#endif
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0) && NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        int32_t busCountMax{DefaultBusCountMax};
#endif
    };
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static_assert(sizeof(SoundSystemParam) == 0x70);
#elif NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    static_assert(sizeof(SoundSystemParam) == 0x80);
#else
    static_assert(sizeof(SoundSystemParam) == 0x88);
#endif

    struct InitializeBufferSet {
        uintptr_t workMem;
        size_t workMemSize;
        uintptr_t memoryPoolMem;
        size_t memoryPoolMemSize;
        uintptr_t circularBufferSinkMem;
        size_t circularBufferSinkMemSize;
    };
    static_assert(sizeof(InitializeBufferSet) == 0x30);

    static const int32_t VoiceCountMax{detail::driver::HardwareManager::AtkVoiceCountMax};
    static const uint32_t WorkMemoryAlignSize{4};
    static const uint32_t VoiceCommandManagerCountMax{2};

    static bool Initialize(const SoundSystemParam& param, uintptr_t workMem, size_t workMemSize);

    static bool Initialize(Result* pOutResult, const SoundSystemParam& param, uintptr_t workMem,
                           size_t workMemSize);

    static bool Initialize(const SoundSystemParam& param, uintptr_t workMem, size_t workMemSize,
                           uintptr_t memoryPoolMem, size_t memoryPoolMemSize);

    static bool Initialize(Result* pOutResult, const SoundSystemParam& param, uintptr_t workMem,
                           size_t workMemSize, uintptr_t memoryPoolMem, size_t memoryPoolMemSize);

    static bool Initialize(const SoundSystemParam& param, InitializeBufferSet& bufferSet);

    static bool Initialize(Result* pOutResult, const SoundSystemParam& param,
                           InitializeBufferSet& bufferSet);

    static size_t GetRequiredMemSize(const SoundSystemParam& param);

    static size_t GetRequiredMemSizeForMemoryPool(const SoundSystemParam& param);

    static size_t GetRequiredMemSizeForCircularBufferSink(const SoundSystemParam& param);

    static void Finalize();

    static void SetSoundFrameUserCallback(SoundFrameUserCallback callback, uintptr_t arg) {
        detail::driver::SoundThread::GetInstance().RegisterSoundFrameUserCallback(callback, arg);
    }

    static void ClearSoundFrameUserCallback() {
        detail::driver::SoundThread::GetInstance().ClearSoundFrameUserCallback();
    }

    static void SetSoundThreadBeginUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    static void ClearSoundThreadBeginUserCallback();

    static void SetSoundThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg);
    static void ClearSoundThreadEndUserCallback();

    static bool IsInitialized();

    static void LockSoundThread() { detail::driver::SoundThread::GetInstance().Lock(); }

    static void UnlockSoundThread() { detail::driver::SoundThread::GetInstance().Unlock(); }

    static void SuspendAudioRenderer(TimeSpan fadeTimes);
    static void ResumeAudioRenderer(TimeSpan fadeTimes);
    static void ExecuteRendering();

    static audio::AudioRendererConfig& GetAudioRendererConfig() {
        return detail::driver::HardwareManager::GetInstance().GetAudioRendererConfig();
    }

    static const audio::AudioRendererParameter& GetAudioRendererParameter() {
        return detail::driver::HardwareManager::GetInstance().GetAudioRendererParameter();
    }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static audio::FinalMixType& GetFinalMix() {
        return detail::driver::HardwareManager::GetInstance().GetFinalMix();
    }
#else
    static FinalMix& GetFinalMix() {
        return detail::driver::HardwareManager::GetInstance().GetFinalMix();
    }
#endif

    static void AttachMemoryPool(audio::MemoryPoolType* pMemoryPool, void* address, size_t size);
    static void DetachMemoryPool(audio::MemoryPoolType* pMemoryPool);

    static void EnterSleep() { g_IsEnterSleep = true; }

    static void LeaveSleep() { g_IsEnterSleep = false; }

    static int32_t GetAudioRendererRenderingTimeLimit() {
        return detail::driver::HardwareManager::GetInstance().GetAudioRendererRenderingTimeLimit();
    }

    static void SetAudioRendererRenderingTimeLimit(int32_t timeLimit) {
        detail::driver::HardwareManager::GetInstance().SetAudioRendererRenderingTimeLimit(
            timeLimit);
    }

    static void DumpMemory();

    static size_t GetAudioRendererBufferSize();
    static size_t GetRecorderBufferSize();
    static size_t GetUserCircularBufferSinkBufferSize();
    static size_t GetLowLevelVoiceAllocatorBufferSize();
    static size_t GetMultiVoiceManagerBufferSize();
    static size_t GetChannelManagerBufferSize();
    static size_t GetSynthesizeBufferSize();  // TODO
    static size_t GetDriverCommandBufferSize();
    static size_t GetAllocatableDriverCommandSize();
    static size_t GetAllocatedDriverCommandBufferSize();
    static size_t GetAllocatedDriverCommandCount();

    static bool detail_IsStreamLoadWait() { return g_IsStreamLoadWait; }

    static bool detail_IsInitializedNwRenderer();  // TODO

    static void detail_SetStreamOpenFailureHaltEnabled(bool isStreamOpenFailureHalt) {
        g_IsStreamOpenFailureHalt = isStreamOpenFailureHalt;
    }

    static bool detail_IsStreamOpenFailureHaltEnabled() {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        return true;
#else
        return g_IsStreamOpenFailureHalt;
#endif
    }

    static bool detail_IsTaskThreadEnabled() { return g_IsTaskThreadEnabled; }

    static bool detail_IsSoundThreadEnabled() { return g_IsSoundThreadEnabled; }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0) && NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    static detail::SoundInstanceConfig GetSoundInstanceConfig() {
        detail::SoundInstanceConfig config;
        config.isBusMixVolumeEnabled = g_IsBusMixVolumeEnabled;
        config.isVolumeThroughModeEnabled = g_IsVolumeThroughModeEnabled;
        config.busCount = g_BusCountMax;

        return config;
    }

    static detail::SoundInstanceConfig GetSoundInstanceConfig(const SoundSystemParam& param) {
        detail::SoundInstanceConfig config;
        config.isBusMixVolumeEnabled = param.enableBusMixVolume;
        config.isVolumeThroughModeEnabled = param.enableVolumeThroughMode;
        config.busCount = param.busCountMax;

        return config;
    }
#elif NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    static detail::SoundInstanceConfig GetSoundInstanceConfig();
    static detail::SoundInstanceConfig GetSoundInstanceConfig(const SoundSystemParam& param);
#endif

    static void SetOutputMode(OutputMode mode, OutputDevice device) {
        detail::driver::HardwareManager::GetInstance().SetOutputMode(mode, device);
    }

    static OutputMode GetOutputMode(OutputDevice device) {
        return detail::driver::HardwareManager::GetInstance().GetOutputMode(device);
    }

    static void detail_UpdateEndUserOutputMode() {
        detail::driver::HardwareManager::GetInstance().UpdateEndUserOutputMode();
    }

    static void SetOutputDeviceFlag(OutputLineIndex outputLineIndex, uint8_t outputDeviceFlag) {
        detail::driver::HardwareManager::GetInstance().SetOutputDeviceFlag(outputLineIndex,
                                                                           outputDeviceFlag);
    }

    static uint8_t GetOutputDeviceFlag(OutputLineIndex outputLineIndex) {
        return detail::driver::HardwareManager::GetInstance().GetOutputDeviceFlag(outputLineIndex);
    }

    static void SetMasterVolume(float volume, int32_t fadeFrames) {
        detail::driver::HardwareManager::GetInstance().SetMasterVolume(volume, fadeFrames);
    }

    static float GetMasterVolume() {
        return detail::driver::HardwareManager::GetInstance().GetMasterVolume();
    }

    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* effectBuffer,
                             size_t effectBufferSize);
    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* effectBuffer,
                             size_t effectBufferSize, OutputDevice device);
    static bool AppendEffect(AuxBus bus, EffectBase* pEffect, void* effectBuffer,
                             size_t effectBufferSize, OutputDevice device, int32_t subMixIndex);

    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* effectBuffer,
                             size_t effectBufferSize);
    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* effectBuffer,
                             size_t effectBufferSize, OutputDevice device);
    static bool AppendEffect(AuxBus bus, EffectAux* pEffect, void* effectBuffer,
                             size_t effectBufferSize, OutputDevice device, int32_t subMixIndex);
    static bool AppendEffectToFinalMix(EffectAux* pEffect, void* effectBuffer,
                                       size_t effectBufferSize);
    static bool AppendEffectToAdditionalSubMix(EffectAux* pEffect, void* effectBuffer,
                                               size_t effectBufferSize);

    static size_t GetRequiredEffectAuxBufferSize(const EffectAux* pEffect);

    static void RemoveEffect(AuxBus bus, EffectBase* pEffect);
    static void RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device);
    static void RemoveEffect(AuxBus bus, EffectBase* pEffect, OutputDevice device,
                             int32_t subMixIndex);

    static void RemoveEffect(AuxBus bus, EffectAux* pEffect);
    static void RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device);
    static void RemoveEffect(AuxBus bus, EffectAux* pEffect, OutputDevice device,
                             int32_t subMixIndex);
    static void RemoveEffectFromFinalMix(EffectAux* pEffect);
    static void RemoveEffectFromAdditionalSubMix(EffectAux* pEffect);

    static void ClearEffect(AuxBus bus);
    static void ClearEffect(AuxBus bus, OutputDevice device);
    static void ClearEffect(AuxBus bus, OutputDevice device, int32_t subMixIndex);
    static void ClearEffectFromFinalMix();
    static void ClearEffectFromAdditionalSubMix();

    static bool IsClearEffectFinished(AuxBus bus);
    static bool IsClearEffectFinished(AuxBus bus, OutputDevice device);
    static bool IsClearEffectFinished(AuxBus bus, OutputDevice device, int32_t subMixIndex);
    static bool IsClearEffectFromFinalMixFinished();
    static bool IsClearEffectFromAdditionalSubMixFinished();

    static void SetAuxBusVolume(AuxBus bus, float volume, TimeSpan fadeTimes);
    static void SetAuxBusVolume(AuxBus bus, float volume, TimeSpan fadeTimes, int32_t subMixIndex);

    static float GetAuxBusVolume(AuxBus bus);
    static float GetAuxBusVolume(AuxBus bus, int32_t subMixIndex);

    static void SetMainBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannelIndex,
                                                           int32_t destChannelIndex);
    static float GetMainBusChannelVolumeForAdditionalEffect(int32_t srcChannelIndex,
                                                            int32_t destChannelIndex);

    static void SetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, float volume,
                                                          int32_t srcChannelIndex,
                                                          int32_t destChannelIndex);
    static float GetAuxBusChannelVolumeForAdditionalEffect(AuxBus bus, int32_t srcChannelIndex,
                                                           int32_t destChannelIndex);

    static void SetAllAuxBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannelIndex,
                                                             int32_t destChannelIndex);
    static void SetAllBusChannelVolumeForAdditionalEffect(float volume, int32_t srcChannelIndex,
                                                          int32_t destChannelIndex);

    static void SetSampleRateConverterType(SampleRateConverterType sampleRateConverter) {
        detail::driver::HardwareManager::GetInstance().SetSrcType(sampleRateConverter);
    }

    static SampleRateConverterType GetSampleRateConverterType() {
        return detail::driver::HardwareManager::GetInstance().GetSrcType();
    }

    static void PrepareReset() { detail::driver::HardwareManager::GetInstance().PrepareReset(); }

    static void WaitForResetReady();  // TODO

    static void SetNwVoiceSampleRate(uint32_t sampleRate);  // TODO

    static const uint32_t SoundThreadIntervalUsec = 5000;

    static void EnableSequencePrintVar() { detail::driver::MmlParser::EnablePrintVar(); }

    static void DisableSequencePrintVar();  // TODO

    static bool IsEnabledSequencePrintVar() {
        return detail::driver::MmlParser::IsEnabledPrintVar();
    }

    static int32_t GetVoiceCount() {
        return detail::driver::MultiVoiceManager::GetInstance().GetVoiceCount();
    }

    static int32_t GetVoiceCountMax() { return g_VoiceCountMax; }

    static void SetVoiceCountMax(int32_t voiceCount) { g_VoiceCountMax = voiceCount; }

    static int32_t GetRendererSampleRate() { return g_RendererSampleRate; }

    static void SetBiquadFilterCallback(int32_t type, const BiquadFilterCallback* callback) {
        detail::driver::HardwareManager::GetInstance().SetBiquadFilterCallback(type, callback);
    }

    static void RegisterProfileReader(ProfileReader& profileReader) {
        detail::driver::SoundThread::GetInstance().RegisterProfileReader(profileReader);
    }

    static void UnregisterProfileReader(ProfileReader& profileReader) {
        detail::driver::SoundThread::GetInstance().UnregisterProfileReader(profileReader);
    }

    static void RegisterTaskProfileReader(TaskProfileReader& profileReader) {
        detail::TaskManager::GetInstance().RegisterTaskProfileReader(profileReader);
    }

    static void UnregisterTaskProfileReader(const TaskProfileReader& profileReader) {
        detail::TaskManager::GetInstance().UnregisterTaskProfileReader(profileReader);
    }

    static void
    RegisterSoundThreadUpdateProfileReader(SoundThreadUpdateProfileReader profileReader);
    static void
    UnregisterSoundThreadUpdateProfileReader(SoundThreadUpdateProfileReader& profileReader);

    static void RegisterSoundThreadInfoRecorder(SoundThreadInfoRecorder& recorder);
    static void UnregisterSoundThreadInfoRecorder(SoundThreadInfoRecorder& recorder);

    static size_t ReadCircularBufferSink(void* pOutBuffer, size_t bufferSize);

    static size_t GetCircularBufferSinkBufferSize();

    static int32_t GetRendererSampleCount();
    static int32_t GetRendererChannelCountMax();

    static void StopCircularBufferSink();
    static void StartCircularBufferSink();

    static CircularBufferSinkState GetCircularBufferSinkState();

    static void
    RegisterAudioRendererPerformanceReader(AudioRendererPerformanceReader& performanceReader);

    static void UnregisterAudioRendererPerformanceReader();  // TODO

    static void VoiceCommandProcess(UpdateType updateType, uint32_t frameCount);
    static void VoiceCommandProcess(uint32_t frameCount);

    static void VoiceCommandUpdate();

    static size_t GetPerformanceFrameBufferSize();

    static int32_t GetDroppedLowLevelVoiceCount();

private:
    static bool detail_InitializeSoundSystem(Result* pOutResult, const SoundSystemParam& param,
                                             InitializeBufferSet& bufferSet);
    static void detail_InitializeDriverCommandManager(const SoundSystemParam& param,
                                                      uintptr_t workMem, size_t workMemSize,
                                                      uintptr_t memoryPoolMem,
                                                      size_t memoryPoolMemSize);

    static void SetupInitializeBufferSet(InitializeBufferSet* pOutValue, const SoundSystemParam& param,
                                         const InitializeBufferSet& bufferSet);

    static void SetupHardwareManagerParameterFromCurrentSetting(
        detail::driver::HardwareManager::HardwareManagerParameter* pOutValue);

    static void* g_LoadThreadStackPtr;
    static size_t g_LoadThreadStackSize;

#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    static FsPriority g_TaskThreadFsPriority;
#endif

    static void* g_SoundThreadStackPtr;
    static size_t g_SoundThreadStackSize;

    static void* g_PerformanceFrameBuffer;
    static size_t g_PerformanceFrameBufferSize;

    static bool g_IsInitialized;
    static bool g_IsEnterSleep;
    static bool g_IsInitializedDriverCommandManager;
    static bool g_IsStreamLoadWait;
    static bool g_IsStreamOpenFailureHalt;
    static bool g_IsTaskThreadEnabled;
    static bool g_IsManagingMemoryPool;

    static size_t g_SoundThreadCommandBufferSize;
    static size_t g_TaskThreadCommandBufferSize;
    static size_t g_VoiceCommandBufferSize;

    static audio::MemoryPoolType g_MemoryPoolForSoundSystem;

    static int32_t g_RendererSampleRate;
    static int32_t g_VoiceCountMax;
    static int32_t g_UserEffectCount;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    static int32_t g_CustomSubMixSubMixCount;
    static int32_t g_CustomSubMixMixBufferCount;
#endif

    static bool g_IsProfilerEnabled;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    static bool g_IsDetailSoundThreadProfilerEnabled;
#endif

    static bool g_IsAdditionalEffectBusEnabled;
    static bool g_IsAdditionalSubMixEnabled;
    static bool g_IsEffectEnabled;
    static bool g_IsRecordingEnabled;
    static bool g_IsCircularBufferSinkEnabled;
    static bool g_IsCircularBufferSinkWarningDisplayed;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    static bool g_IsSubMixEnabled;
    static bool g_IsPresetSubMixEnabled;
    static bool g_IsStereoModeEnabled;
#endif

    static bool g_IsSoundThreadEnabled;
    static bool g_IsVoiceDropEnabled;
    static bool g_IsPreviousSdkVersionLowPassFilterCompatible;
    static bool g_IsUnusedEffectChannelMutingEnabled;
    static bool g_IsUserThreadRenderingEnabled;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    static bool g_IsCustomSubMixEnabled;
    static bool g_IsMemoryPoolAttachCheckEnabled;
    static bool g_IsBusMixVolumeEnabled;
    static bool g_IsVolumeThroughModeEnabled;

    static int32_t g_BusCountMax;
#endif
};

}  // namespace nn::atk
