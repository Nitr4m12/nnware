#include <nn/atk/atk_TaskThread.h>

#include <nn/atk/atk_TaskManager.h>

namespace nn::atk::detail {

TaskThread::TaskThread() = default;

TaskThread::~TaskThread() {
    Destroy();
}

void TaskThread::Destroy() {
    if (!m_IsCreated)
        return;

    m_IsFinished = true;
    TaskManager::GetInstance().CancelWaitTask();
    m_Thread.WaitForExit();
    m_Thread.Release();
    m_IsCreated = false;
}

#if NN_WARE_VER < NN_MAKE_VER(5, 0, 0)
bool TaskThread::Create(int32_t priority, void* stackBase, size_t stackSize,
                        int32_t idealCoreNumber, uint32_t affinityMask)
#else
bool TaskThread::Create(int32_t priority, void* stackBase, size_t stackSize,
                        int32_t idealCoreNumber, uint32_t affinityMask, FsPriority fsPriority)
#endif
{
    Destroy();
    m_IsFinished = false;
    fnd::Thread::RunArgs args;
    args.name = "nn::atk::detail::TaskThread";
    args.stack = stackBase;
    args.stackSize = stackSize;
    args.idealCoreNumber = idealCoreNumber;
    args.affinityMask = static_cast<fnd::Thread::AffinityMask>(affinityMask);
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    args.fsPriority = static_cast<fnd::Thread::FsPriority>(fsPriority);
#endif
    args.priority = priority;
    args.param = nullptr;
    args.handler = this;

    bool result{m_Thread.Run(args)};
    if (!result)
        return false;

#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
    m_FsPriority = fsPriority;
#endif
    m_IsCreated = true;

    return true;
}

TaskThread& TaskThread::GetInstance() {
    static TaskThread instance;
    return instance;
}

}  // namespace nn::atk::detail
