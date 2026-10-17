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

TaskThread& TaskThread::GetInstance() {
    static TaskThread instance;
    return instance;
}

}  // namespace nn::atk::detail
