#include <nn/atk/atk_TaskManager.h>
#include "nn/atk/fnd/os/atkfnd_ScopedLock.h"

namespace nn::atk::detail {

TaskManager& TaskManager::GetInstance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() = default;

void TaskManager::Initialize(bool isEnableProfiling) {
    m_TaskProfileLogger.SetProfilingEnabled(isEnableProfiling);
}

void TaskManager::Finalize() {
    m_TaskProfileLogger.SetProfilingEnabled(false);
    m_TaskProfileLogger.Finalize();
}

void TaskManager::AppendTask(Task* task, TaskPriority priority) {
    task->m_Event.Clear();
    task->m_Status = Task::Status_Append;

    {
        fnd::ScopedLock<fnd::CriticalSection> scopeLock{m_CriticalSection};
        m_TaskList[priority].push_back(*task);
    }

    m_BlockingQueue.TrySend(0);
}

}  // namespace nn::atk::detail
