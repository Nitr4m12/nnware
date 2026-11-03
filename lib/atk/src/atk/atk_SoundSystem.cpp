#include <nn/atk/atk_SoundSystem.h>

#include <nn/diag.h>

#include <nn/atk/atk_ChannelManager.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_TaskThread.h>
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
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pOutValue->SetAdditionalSubMixEnabled(parameter.enableAdditionalEffectBus);
    pOutValue->SetAdditionalEffectBusEnabled(parameter.enableAdditionalSubMix);
#endif
    pOutValue->SetEffectEnabled(parameter.enableEffect);
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pOutValue->SetPresetSubMixEnabled(parameter.enableSubMix);
#endif
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
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pOutValue->SetMemoryPoolAttachCheckEnabled(parameter.enableMemoryPoolAttachCheck);
#else
    pOutValue->SetCompatibleBusVolumeEnabled(parameter.enableCompatibleBusVolume);
#endif
    pOutValue->SetUserThreadRenderingEnabled(parameter.enableUserThreadRendering);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    pOutValue->SetMemoryPoolAttachCheckEnabled(parameter.enableMemoryPoolAttachCheck);
#endif

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0) && NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
    const int32_t subMixTotalChannelCount{parameter.subMixTotalChannelCount};
#elif NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    const int32_t subMixTotalChannelCount{
        parameter.mixBufferCount != nn::atk::SoundSystem::SoundSystemParam::InvalidMixBufferCount ?
            parameter.mixBufferCount :
            parameter.subMixTotalChannelCount,
    };
#endif

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    pOutValue->SetSubMixParameter(parameter.enableStereoMode, parameter.enableEffect,
                                  parameter.enableSubMix, parameter.enableAdditionalEffectBus,
                                  parameter.enableAdditionalSubMix, parameter.enableCustomSubMix,
                                  parameter.subMixCount, subMixTotalChannelCount);
#endif
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

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
bool SoundSystem::g_IsSubMixEnabled{true};
bool SoundSystem::g_IsPresetSubMixEnabled{true};
bool SoundSystem::g_IsStereoModeEnabled{true};
#endif

bool SoundSystem::g_IsSoundThreadEnabled{true};

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
int32_t SoundSystem::g_BusCountMax{detail::DefaultBusCount};
#endif

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

bool SoundSystem::detail_InitializeSoundSystem(Result* pOutResult, const SoundSystemParam& param,
                                               InitializeBufferSet& bufferSet) {
    nn::util::ReferSymbol(g_MiddlewareInfo);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    detail::SoundInstanceConfig config{GetSoundInstanceConfig(param)};
#endif

    if (g_IsInitialized) {
        if (pOutResult != nullptr)
            *pOutResult = nn::ResultSuccess();

        return true;
    }

    bool result{false};

    uintptr_t ptr{bufferSet.workMem};
    uintptr_t ptrMemoryPool{bufferSet.memoryPoolMem};
    uintptr_t ptrCircularBufferSink{bufferSet.circularBufferSinkMem};

    if (!g_IsInitializedDriverCommandManager) {
        size_t workMemSizeForDriverCommandManager{
            param.soundThreadCommandBufferSize + param.taskThreadCommandBufferSize,
        };
        size_t workMemSizeForDriverCommandManagerForMemoryPool{0};

        detail_InitializeDriverCommandManager(param, ptr, workMemSizeForDriverCommandManager,
                                              ptrMemoryPool,
                                              workMemSizeForDriverCommandManagerForMemoryPool);
        ptr += workMemSizeForDriverCommandManager;
    }

    ptr = util::align_up(ptr, 4096);

    {
        detail::driver::HardwareManager::HardwareManagerParameter hardwareManagerParameter;
        SetupHardwareManagerParameter(&hardwareManagerParameter, param);
        g_VoiceCountMax = param.voiceCountMax;

        {
            void* workBuffer{reinterpret_cast<void*>(ptr)};
            void* memoryPoolBuffer{reinterpret_cast<void*>(ptrMemoryPool)};
            void* circularBufferSinkBuffer{};
            size_t workBufferSize{
                detail::driver::HardwareManager::GetInstance().GetRequiredMemSize(
                    hardwareManagerParameter),
            };
            size_t memoryPoolBufferSize{
                detail::driver::HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(
                    g_VoiceCountMax),
            };
            size_t circularBufferSinkBufferSize{0};

            if (param.enableRecordingFinalOutputs)
                workBufferSize += detail::driver::HardwareManager::GetInstance()
                                      .GetRequiredRecorderWorkBufferSize(hardwareManagerParameter);

            if (param.enableCircularBufferSink) {
                circularBufferSinkBuffer = reinterpret_cast<void*>(ptrCircularBufferSink);
                circularBufferSinkBufferSize =
                    detail::driver::HardwareManager::GetInstance()
                        .GetRequiredCircularBufferSinkBufferSize(hardwareManagerParameter);
            }

            nn::Result hardwareManagerResult{
                detail::driver::HardwareManager::GetInstance().Initialize(
                    workBuffer, workBufferSize, memoryPoolBuffer, memoryPoolBufferSize,
                    circularBufferSinkBuffer, circularBufferSinkBufferSize,
                    &hardwareManagerParameter),
            };

            if (pOutResult != nullptr)
                *pOutResult = hardwareManagerResult;

            if (hardwareManagerResult.IsFailure()) {
                if (g_IsInitializedDriverCommandManager) {
                    detail::DriverCommand::GetInstance().Finalize();
                    detail::DriverCommand::GetInstanceForTaskThread().Finalize();
                    g_SoundThreadCommandBufferSize = 0;
                    g_TaskThreadCommandBufferSize = 0;
                }

                return false;
            }

            ptr += workBufferSize;
            ptrMemoryPool += memoryPoolBufferSize;
        }

        {
            uintptr_t performanceFrameBuffer{0};
            size_t performanceFrameBufferSize{0};
            if (param.enableProfiler) {
                ptr = util::align_up(ptr, 64);
                performanceFrameBuffer = ptr;
                performanceFrameBufferSize =
                    detail::driver::HardwareManager::GetInstance()
                        .GetRequiredPerformanceFramesBufferSize(&hardwareManagerParameter);
                ptr += performanceFrameBufferSize * 3;
            }

            g_PerformanceFrameBuffer = reinterpret_cast<void*>(performanceFrameBuffer);
            g_PerformanceFrameBufferSize = performanceFrameBufferSize * 3;
        }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0) && NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        const int32_t subMixTotalChannelCount{param.subMixTotalChannelCount};
#elif NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        const int32_t subMixTotalChannelCount{
            param.mixBufferCount == SoundSystemParam::InvalidMixBufferCount ?
                param.subMixTotalChannelCount :
                param.mixBufferCount,
        };
#endif

        g_RendererSampleRate = param.rendererSampleRate;
        g_UserEffectCount = param.effectCount;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        g_CustomSubMixSubMixCount = param.subMixCount;
        g_CustomSubMixMixBufferCount = subMixTotalChannelCount;
#endif
        g_IsProfilerEnabled = param.enableProfiler;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        g_IsDetailSoundThreadProfilerEnabled = param.enableDetailSoundThreadProfile;
#endif
        g_IsAdditionalEffectBusEnabled = param.enableAdditionalEffectBus;
        g_IsAdditionalSubMixEnabled = param.enableAdditionalSubMix;
        g_IsEffectEnabled = param.enableEffect;
        g_IsRecordingEnabled = param.enableRecordingFinalOutputs;
        g_IsCircularBufferSinkEnabled = param.enableCircularBufferSink;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        g_IsSubMixEnabled = param.enableSubMix;
        g_IsPresetSubMixEnabled = hardwareManagerParameter.IsPresetSubMixEnabled();
        g_IsStereoModeEnabled = param.enableStereoMode;
#endif
        g_IsSoundThreadEnabled = param.enableSoundThread;
        g_IsVoiceDropEnabled = param.enableVoiceDrop;
        g_IsPreviousSdkVersionLowPassFilterCompatible = param.enableCompatibleLowPassFilter;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        g_IsUnusedEffectChannelMutingEnabled = param.enableUnusedEffectChannelMuting;
        g_IsUserThreadRenderingEnabled = param.enableUserThreadRendering;
#else
        g_IsUserThreadRenderingEnabled = param.enableUserThreadRendering;
        g_IsUnusedEffectChannelMutingEnabled = param.enableUnusedEffectChannelMuting;
        g_IsCustomSubMixEnabled = param.enableCustomSubMix;
        g_IsMemoryPoolAttachCheckEnabled = param.enableMemoryPoolAttachCheck;
        g_IsBusMixVolumeEnabled = param.enableBusMixVolume;
        g_IsVolumeThroughModeEnabled = param.enableVolumeThroughMode;
        g_BusCountMax = ClampBusCountMax(param.busCountMax);
#endif
    }

    ptr = util::align_up(ptr, 4096);
    {
        uintptr_t soundThreadStackPtr{ptr};
        ptr += param.soundThreadStackSize;

        g_SoundThreadStackPtr = reinterpret_cast<void*>(soundThreadStackPtr);
        g_SoundThreadStackSize = param.soundThreadStackSize;
    }

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    detail::driver::SoundThread::GetInstance().Initialize(
        g_PerformanceFrameBuffer, g_PerformanceFrameBufferSize, g_IsProfilerEnabled);
#elif NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    detail::driver::SoundThread::GetInstance().Initialize(
        g_PerformanceFrameBuffer, g_PerformanceFrameBufferSize, g_IsProfilerEnabled,
        g_IsUserThreadRenderingEnabled);
#else
    detail::driver::SoundThread::GetInstance().Initialize(
        g_PerformanceFrameBuffer, g_PerformanceFrameBufferSize, g_IsProfilerEnabled,
        g_IsDetailSoundThreadProfilerEnabled, g_IsUserThreadRenderingEnabled);
#endif

    {
        uintptr_t loadThreadStackPtr{ptr};
        ptr += param.taskThreadStackSize;

        detail::TaskManager::GetInstance().Initialize(param.enableProfiler);

        g_LoadThreadStackPtr = reinterpret_cast<void*>(loadThreadStackPtr);
        g_LoadThreadStackSize = param.taskThreadStackSize;
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        g_TaskThreadFsPriority = param.taskThreadFsPriority;
#endif
    }

    {
        uintptr_t voiceWork{ptr};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax);
        detail::driver::MultiVoiceManager::GetInstance().Initialize(
            reinterpret_cast<void*>(voiceWork),
            detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax));
#else
        detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax,
                                                                            config);
        detail::driver::MultiVoiceManager::GetInstance().Initialize(
            reinterpret_cast<void*>(voiceWork),
            detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax,
                                                                                config),
            config);
#endif
    }

    {
        void* mem{reinterpret_cast<void*>(ptrMemoryPool)};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        const size_t memSize{
            detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax),
        };
        detail::driver::ChannelManager::GetInstance().Initialize(mem, memSize);
#else
        const size_t memSize{
            detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
                GetChannelCount(g_VoiceCountMax), config),
        };
        detail::driver::ChannelManager::GetInstance().Initialize(
            mem, memSize, GetChannelCount(g_VoiceCountMax), config);
#endif
    }

    uint16_t attribute{0};
    detail::driver::SequenceSoundPlayer::InitSequenceSoundPlayer();
    if (param.enableTaskThread) {
#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
        detail::TaskThread::GetInstance().Create(param.taskThreadPriority, g_LoadThreadStackPtr,
                                                 g_LoadThreadStackSize, param.taskThreadCoreNumber,
                                                 attribute);
#else
        detail::TaskThread::GetInstance().Create(param.taskThreadPriority, g_LoadThreadStackPtr,
                                                 g_LoadThreadStackSize, param.taskThreadCoreNumber,
                                                 attribute, g_TaskThreadFsPriority);
#endif
        g_IsTaskThreadEnabled = true;
    } else {
        g_IsTaskThreadEnabled = false;
    }

#if NN_WARE_VER > +NN_MAKE_VER(4, 0, 0)
    detail::DriverCommand::GetInstance().FlushCommand(false);
#endif

    if (g_IsSoundThreadEnabled) {
        detail::driver::SoundThread::GetInstance().CreateSoundThread(
            param.soundThreadPriority, g_SoundThreadStackPtr, g_SoundThreadStackSize,
            param.soundThreadCoreNumber, attribute);

        detail::CurveLfo::InitializeCurveTable();
        result = true;
        g_IsInitialized = true;

        return result;
    }

    diag::detail::AbortImpl("", "", "", 0);
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

bool SoundSystem::Initialize(const SoundSystemParam& param, uintptr_t workMem, size_t workMemSize) {
    return Initialize(nullptr, param, workMem, workMemSize);
}

bool SoundSystem::Initialize(nn::Result* pOutResult, const SoundSystemParam& param,
                             uintptr_t workMem, size_t workMemSize) {
    InitializeBufferSet bufferSet;
    bufferSet.workMem = workMem;
    bufferSet.workMemSize = workMemSize;
    bufferSet.memoryPoolMem = 0;
    bufferSet.memoryPoolMemSize = 0;
    bufferSet.circularBufferSinkMem = 0;
    bufferSet.circularBufferSinkMemSize = 0;

    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, param, bufferSet);

    const bool result{detail_InitializeSoundSystem(pOutResult, param, setupBufferSet)};
    if (result) {
        detail::driver::HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return result;
}

bool SoundSystem::Initialize(const SoundSystemParam& param, uintptr_t workMem, size_t workMemSize,
                             uintptr_t memoryPoolMem, size_t memoryPoolMemSize) {
    return Initialize(nullptr, param, workMem, workMemSize, memoryPoolMem, memoryPoolMemSize);
}

bool SoundSystem::Initialize(Result* pOutResult, const SoundSystemParam& param, uintptr_t workMem,
                             size_t workMemSize, uintptr_t memoryPoolMem,
                             size_t memoryPoolMemSize) {
    InitializeBufferSet bufferSet;
    bufferSet.workMem = workMem;
    bufferSet.workMemSize = workMemSize;
    bufferSet.memoryPoolMem = memoryPoolMem;
    bufferSet.memoryPoolMemSize = memoryPoolMemSize;
    bufferSet.circularBufferSinkMemSize = 0;
    bufferSet.circularBufferSinkMem = 0;

    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, param, bufferSet);

    const bool result{detail_InitializeSoundSystem(pOutResult, param, setupBufferSet)};

    return result;
}

bool SoundSystem::Initialize(const SoundSystemParam& param, InitializeBufferSet& bufferSet) {
    return Initialize(nullptr, param, bufferSet);
}

bool SoundSystem::Initialize(Result* pOutResult, const SoundSystemParam& param,
                             InitializeBufferSet& bufferSet) {
    InitializeBufferSet setupBufferSet;
    SetupInitializeBufferSet(&setupBufferSet, param, bufferSet);

    const bool result{detail_InitializeSoundSystem(pOutResult, param, setupBufferSet)};
    if (result && param.enableMemoryPoolManagement) {
        detail::driver::HardwareManager::GetInstance().AttachMemoryPool(
            &g_MemoryPoolForSoundSystem, reinterpret_cast<void*>(setupBufferSet.memoryPoolMem),
            setupBufferSet.memoryPoolMemSize, true);
    }

    return result;
}

void SoundSystem::Finalize() {
    if (!g_IsInitialized)
        return;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (g_IsPresetSubMixEnabled) {
        const uint32_t tag{detail::DriverCommand::GetInstance().FlushCommand(true)};
        if (g_IsUserThreadRenderingEnabled)
            detail::driver::SoundThread::GetInstance().FrameProcess(UpdateType_AudioFrame);
        detail::DriverCommand::GetInstance().WaitCommandReply(tag);
    }
#endif

    if (g_IsTaskThreadEnabled) {
        detail::TaskManager::GetInstance().CancelAllTask();
        detail::TaskThread::GetInstance().Destroy();
        detail::TaskManager::GetInstance().Finalize();
    } else {
        g_IsTaskThreadEnabled = true;
    }

    if (g_IsSoundThreadEnabled)
        detail::driver::SoundThread::GetInstance().Destroy();
    else
        g_IsSoundThreadEnabled = true;

    detail::driver::ChannelManager::GetInstance().Finalize();

    if (g_IsManagingMemoryPool)
        detail::driver::HardwareManager::GetInstance().DetachMemoryPool(&g_MemoryPoolForSoundSystem,
                                                                        false);
    else
        g_IsManagingMemoryPool = true;

    detail::driver::MultiVoiceManager::GetInstance().Finalize();
    detail::driver::HardwareManager::GetInstance().Finalize();
    detail::driver::SoundThread::GetInstance().Finalize();

    if (g_IsInitializedDriverCommandManager) {
        detail::DriverCommand::GetInstance().Finalize();
        detail::DriverCommand::GetInstanceForTaskThread().Finalize();
        g_SoundThreadCommandBufferSize = 0;
        g_TaskThreadCommandBufferSize = 0;
        g_IsInitializedDriverCommandManager = false;
    }

    g_IsCircularBufferSinkEnabled = false;
    g_IsCircularBufferSinkWarningDisplayed = false;
    g_IsInitialized = false;
}

void SoundSystem::SetSoundThreadBeginUserCallback(SoundThreadUserCallback callback, uintptr_t arg) {
    detail::driver::SoundThread::GetInstance().RegisterThreadBeginUserCallback(callback, arg);
}

void SoundSystem::ClearSoundThreadBeginUserCallback() {
    detail::driver::SoundThread::GetInstance().ClearThreadBeginUserCallback();
}

void SoundSystem::SetSoundThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg) {
    detail::driver::SoundThread::GetInstance().RegisterThreadEndUserCallback(callback, arg);
}

void SoundSystem::ClearSoundThreadEndUserCallback() {
    detail::driver::SoundThread::GetInstance().ClearThreadEndUserCallback();
}

bool SoundSystem::IsInitialized() {
    return g_IsInitialized;
}

void SoundSystem::SuspendAudioRenderer([[maybe_unused]] TimeSpan fadeTimes) {
    detail::driver::HardwareManager::GetInstance().SuspendAudioRenderer();
}

void SoundSystem::ResumeAudioRenderer([[maybe_unused]] TimeSpan fadeTimes) {
    detail::driver::HardwareManager::GetInstance().ResumeAudioRenderer();
}

void SoundSystem::ExecuteRendering() {
    detail::driver::HardwareManager::GetInstance().ExecuteAudioRendererRendering();
}

int32_t SoundSystem::GetAudioRendererRenderingTimeLimit() {
    return detail::driver::HardwareManager::GetInstance().GetAudioRendererRenderingTimeLimit();
}

void SoundSystem::SetAudioRendererRenderingTimeLimit(int32_t limitPercent) {
    detail::driver::HardwareManager::GetInstance().SetAudioRendererRenderingTimeLimit(limitPercent);
    // TODO: figure out this variable and scope
    // {
    //     const nn::Result& nnDetailAssertArgValue_arg1;
    // }
}

void SoundSystem::AttachMemoryPool(audio::MemoryPoolType* pMemoryPool, void* address, size_t size) {
    detail::driver::HardwareManager::GetInstance().AttachMemoryPool(pMemoryPool, address, size,
                                                                    true);
}

void SoundSystem::DetachMemoryPool(audio::MemoryPoolType* pMemoryPool) {
    detail::driver::HardwareManager::GetInstance().DetachMemoryPool(pMemoryPool, true);
}

void SoundSystem::DumpMemory() {}

size_t SoundSystem::GetAudioRendererBufferSize() {
    detail::driver::HardwareManager::HardwareManagerParameter parameter;
    SetupHardwareManagerParameterFromCurrentSetting(&parameter);

    if (!IsInitialized())
        return 0;

    return detail::driver::HardwareManager::GetInstance().GetRequiredMemSize(parameter);
}

size_t SoundSystem::GetRecorderBufferSize() {
    detail::driver::HardwareManager::HardwareManagerParameter parameter;
    SetupHardwareManagerParameterFromCurrentSetting(&parameter);

    size_t bufferSize{0};
    if (IsInitialized() && g_IsRecordingEnabled) {
        bufferSize =
            detail::driver::HardwareManager::GetInstance().GetRequiredRecorderWorkBufferSize(
                parameter);
    }

    return bufferSize;
}

size_t SoundSystem::GetUserCircularBufferSinkBufferSize() {
    detail::driver::HardwareManager::HardwareManagerParameter parameter;
    SetupHardwareManagerParameterFromCurrentSetting(&parameter);

    size_t bufferSize{0};
    if (IsInitialized() && g_IsCircularBufferSinkEnabled) {
        bufferSize = detail::driver::HardwareManager::GetInstance()
                         .GetRequiredCircularBufferSinkWithMemoryPoolBufferSize(parameter);
    }

    return bufferSize;
}

size_t SoundSystem::GetLowLevelVoiceAllocatorBufferSize() {
    if (!IsInitialized())
        return 0;

    return detail::driver::HardwareManager::GetInstance().GetRequiredMemSizeForMemoryPool(
        g_VoiceCountMax);
}

size_t SoundSystem::GetMultiVoiceManagerBufferSize() {
    if (!IsInitialized())
        return 0;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    return detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax);
#else
    return detail::driver::MultiVoiceManager::GetInstance().GetRequiredMemSize(
        g_VoiceCountMax, GetSoundInstanceConfig());
#endif
}

size_t SoundSystem::GetChannelManagerBufferSize() {
    if (!IsInitialized())
        return 0;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    return detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(g_VoiceCountMax);
#else
    return detail::driver::ChannelManager::GetInstance().GetRequiredMemSize(
        GetChannelCount(g_VoiceCountMax), GetSoundInstanceConfig());
#endif
}

size_t SoundSystem::GetDriverCommandBufferSize() {
    return detail::DriverCommand::GetInstance().GetCommandBufferSize();
}

size_t SoundSystem::GetAllocatableDriverCommandSize() {
    return detail::DriverCommand::GetInstance().GetAllocatableCommandSize();
}

size_t SoundSystem::GetAllocatedDriverCommandBufferSize() {
    return detail::DriverCommand::GetInstance().GetAllocatedCommandBufferSize();
}

size_t SoundSystem::GetAllocatedDriverCommandCount() {
    return detail::DriverCommand::GetInstance().GetAllocatedCommandCount();
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

void SoundSystem::SetupHardwareManagerParameterFromCurrentSetting(
    detail::driver::HardwareManager::HardwareManagerParameter* pOutValue) {  // 1570
    pOutValue->SetRendererSampleRate(g_RendererSampleRate);
    pOutValue->SetUserEffectCount(g_UserEffectCount);
    pOutValue->SetProfilerEnabled(g_IsProfilerEnabled);
    pOutValue->SetVoiceCount(g_VoiceCountMax);
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pOutValue->SetAdditionalSubMixEnabled(g_IsAdditionalSubMixEnabled);
#endif
    pOutValue->SetEffectEnabled(g_IsEffectEnabled);
    pOutValue->SetRecordingEnabled(g_IsRecordingEnabled);
    pOutValue->SetUserCircularBufferSinkEnabled(g_IsCircularBufferSinkEnabled);
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    pOutValue->SetStereoModeEnabled(g_IsStereoModeEnabled);
#endif
    pOutValue->SetSoundThreadEnabled(g_IsSoundThreadEnabled);
    pOutValue->SetVoiceDropEnabled(g_IsVoiceDropEnabled);
    pOutValue->SetPreviousSdkVersionLowPassFilterCompatible(
        g_IsPreviousSdkVersionLowPassFilterCompatible);
    pOutValue->SetUnusedEffectChannelMutingEnabled(g_IsUnusedEffectChannelMutingEnabled);
    pOutValue->SetUserThreadRenderingEnabled(g_IsUserThreadRenderingEnabled);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    pOutValue->SetSubMixParameter(g_IsStereoModeEnabled, g_IsEffectEnabled, g_IsSubMixEnabled,
                                  g_IsAdditionalEffectBusEnabled, g_IsAdditionalSubMixEnabled,
                                  g_IsCustomSubMixEnabled, g_CustomSubMixSubMixCount,
                                  g_CustomSubMixMixBufferCount);

    pOutValue->SetMemoryPoolAttachCheckEnabled(g_IsMemoryPoolAttachCheckEnabled);
#endif
}

}  // namespace nn::atk
