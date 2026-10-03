#include <nn/atk/atk_SoundThread.h>

#include <nn/audio.h>

#include <nn/atk/atk_ChannelManager.h>
#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_MultiVoiceManager.h>
#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
namespace {

void GetTick(nn::os::Tick* pVariable) {
    *pVariable = nn::os::GetSystemTick();
}

void DoNothing([[maybe_unused]] nn::os::Tick* pVariable) {}

}  // anonymous namespace
#endif

namespace nn::atk::detail::driver {

SoundThread& SoundThread::GetInstance() {
    static SoundThread instance;
    return instance;
}

SoundThread::SoundThread() = default;

SoundThread::~SoundThread() = default;

bool SoundThread::CreateSoundThread(int32_t threadPriority, void* stackBase, size_t stackSize,
                                    int32_t idealCoreNumber, uint32_t affinityMask) {
    m_SoundThreadAffinityMask = affinityMask;

    fnd::Thread::RunArgs args;
    args.name = "nn::atk::detail::driver::SoundThread";
    args.stack = stackBase;
    args.stackSize = stackSize;
    args.idealCoreNumber = idealCoreNumber;
    args.affinityMask = static_cast<fnd::Thread::AffinityMask>(affinityMask);
    args.priority = threadPriority;
    args.param = nullptr;
    args.handler = this;

    m_CreateFlag = m_Thread.Run(args);

    return m_CreateFlag;
}

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
void SoundThread::Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                             bool isProfilingEnabled)
#elif NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void SoundThread::Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                             bool isProfilingEnabled, bool isUserThreadRenderingEnabled)
#else
void SoundThread::Initialize(void* performanceFrameBuffer, size_t performanceFrameBufferSize,
                             bool isProfilingEnabled, bool isDetailSoundThreadProfilerEnabled,
                             bool isUserThreadRenderingEnabled)
#endif
{
    uintptr_t ptr{reinterpret_cast<uintptr_t>(performanceFrameBuffer)};

    m_pAudioRendererPerformanceReader = nullptr;
    m_CurrentPerformanceFrameBufferIndex = 0;
    m_IsProfilingEnabled = isProfilingEnabled;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_pSoundThreadProfileFunc = isDetailSoundThreadProfilerEnabled ? GetTick : DoNothing;
#endif
#if NN_WARE_VER >= NN_MAKE_VER(3, 0, 0)
    m_IsUserThreadRenderingEnabled = isUserThreadRenderingEnabled;
#endif
    m_RendererEventWaitTimeMilliSeconds = 0;

    const size_t bufferSize{performanceFrameBufferSize / 3};
    m_PerformanceFrameUpdateBufferSize = bufferSize;

    for (int i{0}; i < 3; ++i) {
        m_pPerformanceFrameUpdateBuffer[i] = reinterpret_cast<void*>(ptr);
        ptr += bufferSize;
    }
}

void SoundThread::Destroy() {
    if (!m_CreateFlag)
        return;

    m_BlockingQueue.Jam(Message_Shutdown);

    if (m_IsUserThreadRenderingEnabled)
        HardwareManager::GetInstance().ExecuteAudioRendererRendering();

    m_Thread.WaitForExit();
    m_Thread.Release();

    m_SoundThreadAffinityMask = fnd::Thread::AffinityMask_CoreDefault;
    m_CreateFlag = false;
}

void SoundThread::Finalize() {
    for (auto itr{m_PlayerCallbackList.begin()}; itr != m_PlayerCallbackList.end();) {
        auto curItr{itr++};
        curItr->OnShutdownSoundThread();
    }

    m_pAudioRendererPerformanceReader = nullptr;
    m_CurrentPerformanceFrameBufferIndex = 0;
    m_PerformanceFrameUpdateBufferSize = 0;

    for (int i{0}; i < 3; ++i)
        m_pPerformanceFrameUpdateBuffer[i] = nullptr;
}

void SoundThread::UpdateLowLevelVoices() {
    OutputMode outputMode{HardwareManager::GetInstance().GetOutputMode(OutputDevice_Main)};
    HardwareManager::GetInstance().GetLowLevelVoiceAllocator().UpdateAllVoiceState(outputMode);
}

void SoundThread::ForceWakeup() {
    m_BlockingQueue.TrySend(Message_ForceWakeup);
}

void SoundThread::RegisterSoundFrameUserCallback(SoundFrameUserCallback callback, uintptr_t arg) {
    SoundThreadLock lock{};

    m_UserCallbackArg = arg;
    m_UserCallback = callback;
}

void SoundThread::ClearSoundFrameUserCallback() {
    SoundThreadLock lock{};

    m_UserCallback = nullptr;
    m_UserCallbackArg = 0;
}

void SoundThread::RegisterThreadBeginUserCallback(SoundThreadUserCallback callback, uintptr_t arg) {
    SoundThreadLock lock{};

    m_ThreadBeginUserCallbackArg = arg;
    m_ThreadBeginUserCallback = callback;
}

void SoundThread::ClearThreadBeginUserCallback() {
    SoundThreadLock lock{};

    m_ThreadBeginUserCallback = nullptr;
    m_ThreadBeginUserCallbackArg = 0;
}

void SoundThread::RegisterThreadEndUserCallback(SoundThreadUserCallback callback, uintptr_t arg) {
    SoundThreadLock lock{};

    m_ThreadEndUserCallbackArg = arg;
    m_ThreadEndUserCallback = callback;
}

void SoundThread::ClearThreadEndUserCallback() {
    SoundThreadLock lock{};

    m_ThreadEndUserCallback = nullptr;
    m_ThreadEndUserCallbackArg = 0;
}

void SoundThread::RegisterSoundFrameCallback(SoundFrameCallback* callback) {
    fnd::ScopedLock<fnd::CriticalSection> lock{m_CriticalSection};

    m_SoundFrameCallbackList.push_back(*callback);
}

void SoundThread::UnregisterSoundFrameCallback(SoundFrameCallback* callback) {
    fnd::ScopedLock<fnd::CriticalSection> lock{m_CriticalSection};

    m_SoundFrameCallbackList.erase(m_SoundFrameCallbackList.iterator_to(*callback));
}

void SoundThread::RegisterPlayerCallback(PlayerCallback* callback) {
    m_PlayerCallbackList.push_back(*callback);
}

void SoundThread::UnregisterPlayerCallback(PlayerCallback* callback) {
    m_PlayerCallbackList.erase(m_PlayerCallbackList.iterator_to(*callback));
}

void SoundThread::RegisterSoundThreadInfoRecorder(ThreadInfoRecorder& recorder) {
    fnd::ScopedLock<fnd::CriticalSection> lock{m_LockRecordInfo};

    m_InfoRecorderList.push_back(recorder);
}

void SoundThread::UnregisterSoundThreadInfoRecorder(ThreadInfoRecorder& recorder) {
    fnd::ScopedLock<fnd::CriticalSection> lock{m_LockRecordInfo};

    m_InfoRecorderList.erase(m_InfoRecorderList.iterator_to(recorder));
}

void SoundThread::FrameProcess(UpdateType updateType) {
    m_CriticalSection.Lock();

    audio::AudioRendererConfig& config{HardwareManager::GetInstance().GetAudioRendererConfig()};
    void* performanceFrameBuffer{};

    if (m_IsProfilingEnabled) {
        {
            HardwareManager::UpdateAudioRendererScopedLock lock{};
            performanceFrameBuffer = audio::SetPerformanceFrameBuffer(
                &config, m_pPerformanceFrameUpdateBuffer[m_CurrentPerformanceFrameBufferIndex],
                m_PerformanceFrameUpdateBufferSize);
        }

        ++m_CurrentPerformanceFrameBufferIndex;
        if (m_CurrentPerformanceFrameBufferIndex > 2)
            m_CurrentPerformanceFrameBufferIndex = 0;
    }

    os::Tick beginTick{os::GetSystemTick()};

    m_UpdateAtkStateAndParameterSection.Lock();

    for (auto itr{m_SoundFrameCallbackList.begin()}; itr != m_SoundFrameCallbackList.end();) {
        auto curItr{itr++};
        curItr->OnBeginSoundFrame();
    }

    uint32_t nwVoiceCount{0};

    if (updateType == UpdateType_AudioFrame)
        MultiVoiceManager::GetInstance().UpdateAudioFrameVoiceStatus();
    else
        MultiVoiceManager::GetInstance().UpdateAllVoiceStatus();

    while (DriverCommand::GetInstanceForTaskThread().ProcessCommand()) {
    };
    while (DriverCommand::GetInstance().ProcessCommand()) {
    };

    for (auto itr{m_PlayerCallbackList.begin()}; itr != m_PlayerCallbackList.end();) {
        auto curItr{itr++};
        if (updateType == UpdateType_AudioFrame)
            curItr->OnUpdateFrameSoundThreadWithAudioFrameFrequency();
        else
            curItr->OnUpdateFrameSoundThread();
    }

    if (updateType == UpdateType_AudioFrame) {
        ChannelManager::GetInstance().UpdateAudioFrameChannel();
        MultiVoiceManager::GetInstance().UpdateAudioFrameVoices();
    } else {
        ChannelManager::GetInstance().UpdateAllChannel();
        MultiVoiceManager::GetInstance().UpdateAllVoices();
    }

    m_UpdateAtkStateAndParameterSection.Unlock();

    HardwareManager::GetInstance().Update();
    Util::CalcRandom();

    m_UpdateAtkStateAndParameterSection.Lock();

    for (auto itr{m_SoundFrameCallbackList.begin()}; itr != m_SoundFrameCallbackList.end();) {
        auto curItr{itr++};
        curItr->OnEndSoundFrame();
    }

    m_UpdateAtkStateAndParameterSection.Unlock();

    if (m_UserCallback != nullptr)
        m_UserCallback(m_UserCallbackArg);

    HardwareManager::GetInstance().UpdateRecorder();

    m_CriticalSection.Unlock();

    os::Tick endTick{os::GetSystemTick()};

    if (performanceFrameBuffer != nullptr && m_IsProfilingEnabled) {
        audio::PerformanceInfo performanceInfo;

        if (performanceInfo.SetBuffer(performanceFrameBuffer, m_PerformanceFrameUpdateBufferSize)) {
            do {
                RecordPerformanceInfo(performanceInfo, m_LastPerformanceFrameBegin,
                                      m_LastPerformanceFrameEnd, nwVoiceCount);
            } while (performanceInfo.MoveToNextFrame());
        }

        if (m_pAudioRendererPerformanceReader != nullptr)
            m_pAudioRendererPerformanceReader->Record(
                performanceFrameBuffer, m_PerformanceFrameUpdateBufferSize, beginTick);
    }

    m_LastPerformanceFrameBegin = beginTick;
    m_LastPerformanceFrameEnd = endTick;
}

// TODO: SoundThread::RecordPerformanceInfo

void SoundThread::EffectFrameProcess() {
    HardwareManager::GetInstance().UpdateEffect();
}

void SoundThread::RecordUpdateProfile(const SoundThreadUpdateProfile& updateProfile) {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_UpdateProfileReaderList.empty())
        return;
#endif

    {
        fnd::ScopedLock<fnd::CriticalSection> lock{m_LockUpdateProfile};

        for (auto itr{m_UpdateProfileReaderList.begin()}; itr != m_UpdateProfileReaderList.end();
             ++itr) {
            itr->Record(updateProfile);
        }
    }
}

}  // namespace nn::atk::detail::driver
