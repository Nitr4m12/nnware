#include <nn/atk/atk_SoundThread.h>
#include "nn/atk/fnd/os/atkfnd_Thread.h"

namespace {

void GetTick(nn::os::Tick* pVariable) {
    *pVariable = nn::os::GetSystemTick();
}

void DoNothing([[maybe_unused]] nn::os::Tick* pVariable) {}

}  // anonymous namespace

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

}  // namespace nn::atk::detail::driver
