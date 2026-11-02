#include <nn/atk/atk_SoundSystem.h>

#include <nn/atk/atk_ChannelManager.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

NN_MIDDLEWARE(g_MiddlewareInfo, "Nintendo", "NintendoWare_Atk" NN_SDK_BUILD_STR);

namespace {

size_t GetRequiredDriverCommandManagerMemSize(const nn::atk::SoundSystem::SoundSystemParam& param) {
    size_t result{0};
    result += param.soundThreadCommandBufferSize;
    result += param.taskThreadCommandBufferSize;
    result += 0x1000;

    return result;
}

static void SetupHardwareManagerParameter(
    nn::atk::detail::driver::HardwareManager::HardwareManagerParameter* pOutValue,
    const nn::atk::SoundSystem::SoundSystemParam& parameter) {
    pOutValue->SetRendererSampleRate(parameter.rendererSampleRate);
    pOutValue->SetUserEffectCount(parameter.effectCount);
    pOutValue->SetProfilerEnabled(parameter.enableProfiler);
    pOutValue->SetVoiceCount(parameter.voiceCountMax);
    pOutValue->SetEffectEnabled(parameter.enableEffect);
    pOutValue->SetStereoModeEnabled(parameter.enableStereoMode);
    pOutValue->SetRecordingEnabled(parameter.enableRecordingFinalOutputs);
    pOutValue->SetRecordingAudioFrameCount(parameter.recordingAudioFrameCount);
    pOutValue->SetUserCircularBufferSinkEnabled(parameter.enableCircularBufferSink);
    pOutValue->SetSoundThreadEnabled(parameter.enableSoundThread);
    pOutValue->SetVoiceDropEnabled(parameter.enableVoiceDrop);
    pOutValue->SetCompatibleDownMixSettingEnabled(parameter.enableCompatibleDownMixSetting);
    pOutValue->SetPreviousSdkVersionLowPassFilterCompatible(
        parameter.enableCompatibleLowPassFilter);
    pOutValue->SetUnusedEffectChannelMutingEnabled(parameter.enableUnusedEffectChannelMuting);
    pOutValue->SetCompatibleBusVolumeEnabled(parameter.enableCompatibleBusVolume);
    pOutValue->SetUserThreadRenderingEnabled(parameter.enableUserThreadRendering);
    pOutValue->SetMemoryPoolAttachCheckEnabled(parameter.enableMemoryPoolAttachCheck);

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    const int32_t subMixTotalChannelCount{parameter.subMixTotalChannelCount};
#else
    const int32_t subMixTotalChannelCount{
        parameter.mixBufferCount != nn::atk::SoundSystem::SoundSystemParam::InvalidMixBufferCount ?
            parameter.mixBufferCount :
            parameter.subMixTotalChannelCount,
    };
#endif

    pOutValue->SetSubMixParameter(parameter.enableStereoMode, parameter.enableEffect,
                                  parameter.enableSubMix, parameter.enableAdditionalEffectBus,
                                  parameter.enableAdditionalSubMix, parameter.enableCustomSubMix,
                                  parameter.subMixCount, subMixTotalChannelCount);
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t GetRequiredSoundSystemMemSize(const nn::atk::SoundSystem::SoundSystemParam& param)
#else
size_t GetRequiredSoundSystemMemSize(const nn::atk::SoundSystem::SoundSystemParam& param,
                                     const nn::atk::detail::SoundInstanceConfig& config)
#endif
{
    size_t result{0};
    result += param.taskThreadStackSize;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    result += nn::atk::detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(
        param.voiceCountMax);
#else
    result += nn::atk::detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(
        param.voiceCountMax, config);
#endif

    result += param.soundThreadStackSize;

    nn::atk::detail::driver::HardwareManager::HardwareManagerParameter hardwareManagerParameter;
    SetupHardwareManagerParameter(&hardwareManagerParameter, param);

    if (param.enableProfiler) {
        result += nn::atk::detail::driver::HardwareManager::GetInstance()
                      .GetRequiredPerformanceFramesBufferSize(&hardwareManagerParameter) *
                  3;
        result += 0x1040;
    } else {
        result += 0x1000;
    }

    if (param.enableRecordingFinalOutputs)
        result += nn::atk::detail::driver::HardwareManager::GetInstance()
                      .GetRequiredRecorderWorkBufferSize(hardwareManagerParameter);

    result += nn::atk::detail::driver::HardwareManager::GetInstance().GetRequiredMemSize(
        hardwareManagerParameter);

    return result;
}

int32_t GetChannelCount(int32_t voiceCount) {
    return voiceCount * nn::atk::WaveChannelMax;
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t
GetRequiredSoundSystemMemSizeForMemoryPool(const nn::atk::SoundSystem::SoundSystemParam& param)
#else
size_t
GetRequiredSoundSystemMemSizeForMemoryPool(const nn::atk::SoundSystem::SoundSystemParam& param,
                                           const nn::atk::detail::SoundInstanceConfig& config)
#endif
{
    size_t result{0};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    result += nn::atk::detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        GetChannelCount(param.voiceCountMax));
#else
    result += nn::atk::detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        GetChannelCount(param.voiceCountMax), config);
#endif

    result +=
        nn::atk::detail::driver::HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(
            param.voiceCountMax);

    return result;
}

const int32_t AdditionalSubMixBusIndex{0};

int32_t ClampBusCountMax(int32_t busCountMax) {
    return nn::atk::detail::fnd::Clamp(busCountMax, 1, nn::atk::OutputMix::ChannelCountMax);
}

int32_t ConvertAuxBusIndexToSubMixBusIndex(nn::atk::AuxBus bus) {
    // TODO
}

}  // anonymous namespace

namespace nn::atk {

bool SoundSystem::g_IsStreamOpenFailureHalt{true};
bool SoundSystem::g_IsTaskThreadEnabled{true};
bool SoundSystem::g_IsManagingMemoryPool{true};

int32_t SoundSystem::g_VoiceCountMax{96};
int32_t SoundSystem::g_UserEffectCount{10};

bool SoundSystem::g_IsSubMixEnabled{true};
bool SoundSystem::g_IsPresetSubMixEnabled{true};

bool SoundSystem::g_IsStereoModeEnabled{true};
bool SoundSystem::g_IsSoundThreadEnabled{true};

int32_t SoundSystem::g_BusCountMax{detail::DefaultBusCount};

SoundSystem::SoundSystemParam::SoundSystemParam() = default;

size_t SoundSystem::GetRequiredMemSize(const SoundSystemParam& param) {
    size_t result{0};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    result += GetRequiredSoundSystemMemSize(param);
#else
    result += GetRequiredSoundSystemMemSize(param, GetSoundInstanceConfig(param));
#endif

    result += GetRequiredDriverCommandManagerMemSize(param);

    if (param.enableCircularBufferSink && param.enableCircularBufferSinkBufferManagement)
        result += GetRequiredMemSizeForCircularBufferSink(param);

    if (param.enableMemoryPoolManagement) {
        const size_t memoryPoolMemSize{GetRequiredMemSizeForMemoryPool(param)};

        result += util::align_up(memoryPoolMemSize, nn::audio::MemoryPoolType::AddressAlignment);
        result += 0x1000;
    }

    return result;
}

size_t SoundSystem::GetRequiredMemSizeForMemoryPool(const SoundSystemParam& param) {
    size_t result{0};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    result += GetRequiredSoundSystemMemSizeForMemoryPool(param);
#else
    result += GetRequiredSoundSystemMemSizeForMemoryPool(param, GetSoundInstanceConfig(param));
#endif

    return result;
}

size_t SoundSystem::GetRequiredMemSizeForCircularBufferSink(const SoundSystemParam& param) {
    nn::atk::detail::driver::HardwareManager::HardwareManagerParameter hardwareManagerParameter;

    SetupHardwareManagerParameter(&hardwareManagerParameter, param);

    return nn::atk::detail::driver::HardwareManager::GetInstance()
        .GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(hardwareManagerParameter);
}

void SoundSystem::detail_InitializeDriverCommandManager(const SoundSystemParam& param,
                                                        uintptr_t workMem,
                                                        [[maybe_unused]] size_t workMemSize,
                                                        [[maybe_unused]] uintptr_t memoryPoolMem,
                                                        [[maybe_unused]] size_t memoryPoolMemSize) {
    if (g_IsInitializedDriverCommandManager)
        return;

    uintptr_t ptr{workMem};
    uintptr_t soundThreadCommandBufferPtr{ptr};
    ptr += param.soundThreadCommandBufferSize;

    detail::DriverCommand::GetInstance().Initialize(
        reinterpret_cast<void*>(soundThreadCommandBufferPtr), param.soundThreadCommandBufferSize);

    uintptr_t taskThreadCommandBufferPtr{ptr};
    detail::DriverCommand::GetInstanceForTaskThread().Initialize(
        reinterpret_cast<void*>(taskThreadCommandBufferPtr), param.taskThreadCommandBufferSize);

    g_SoundThreadCommandBufferSize = param.soundThreadCommandBufferSize;
    g_TaskThreadCommandBufferSize = param.taskThreadCommandBufferSize;
    g_IsInitializedDriverCommandManager = true;
}

#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
detail::SoundInstanceConfig SoundSystem::GetSoundInstanceConfig() {
    detail::SoundInstanceConfig config;
    config.isBusMixVolumeEnabled = g_IsBusMixVolumeEnabled;
    config.isVolumeThroughModeEnabled = g_IsVolumeThroughModeEnabled;
    config.busCount = g_BusCountMax;

    return config;
}

detail::SoundInstanceConfig SoundSystem::GetSoundInstanceConfig(const SoundSystemParam& param) {
    detail::SoundInstanceConfig config;
    config.isBusMixVolumeEnabled = param.enableBusMixVolume;
    config.isVolumeThroughModeEnabled = param.enableVolumeThroughMode;
    config.busCount = ClampBusCountMax(param.busCountMax);

    return config;
}
#endif

}  // namespace nn::atk
