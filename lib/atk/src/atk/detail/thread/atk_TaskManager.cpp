#include <nn/atk/atk_TaskManager.h>

#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>

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

    m_BlockingQueue.TrySend(Message_Append);
}

Task* TaskManager::GetNextTask(TaskPriority priority, bool doRemove) {
    {
        fnd::ScopedLock<fnd::CriticalSection> scopeLock{m_CriticalSection};
        if (m_TaskList[priority].empty())
            return nullptr;

        Task* task{&m_TaskList[priority].front()};
        if (doRemove)
            m_TaskList[priority].pop_front();

        return task;
    }
}

Task* TaskManager::PopTask() {
    fnd::ScopedLock<fnd::CriticalSection> scopeLock{m_CriticalSection};

    Task* task{GetNextTask(TaskPriority_High, true)};
    if (task != nullptr)
        return task;

    task = GetNextTask(TaskPriority_Middle, true);
    if (task != nullptr)
        return task;

    task = GetNextTask(TaskPriority_Low, true);
    return task;
}

Task* TaskManager::PeekTask() {
    Task* task{GetNextTask(TaskPriority_High, false)};
    if (task != nullptr)
        return task;

    task = GetNextTask(TaskPriority_Middle, false);
    if (task != nullptr)
        return task;

    task = GetNextTask(TaskPriority_Low, false);
    return task;
}

void TaskManager::ExecuteTask() {
    for (Task* task{PopTask()}; task != nullptr; task = PopTask()) {
        task->Execute(m_TaskProfileLogger);
        task->m_Status = Task::Status_Done;
        task->m_Event.Signal();
    }
}

void TaskManager::CancelTask(Task* task) {
    if (!TryRemoveTask(task))
        task->m_Event.Wait();
}

bool TaskManager::TryRemoveTask(Task* task) {
    fnd::ScopedLock<fnd::CriticalSection> scopeLock{m_CriticalSection};

    for (int i{0}; i < PriorityCount; ++i) {
        TaskPriority priority{static_cast<TaskPriority>(i)};
        for (auto itr{m_TaskList[priority].begin()}; itr != m_TaskList[priority].end();) {
            auto curItr{itr++};
            if (task == &*curItr) {
                m_TaskList[priority].erase(curItr);
                curItr->m_Status = Task::Status_Cancel;
                curItr->m_Event.Signal();
                return true;
            }
        }
    }

    return false;
}

}  // namespace nn::atk::detail
