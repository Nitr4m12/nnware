#pragma once

#include <nn/os/os_MessageQueue.h>

#include <nn/atk/atk_Task.h>

namespace nn::atk::detail {

class TaskManager {
public:
    enum TaskPriority {
        TaskPriority_Low,
        TaskPriority_Middle,
        TaskPriority_High,
    };

    static TaskManager& GetInstance();

    void Initialize(bool isEnableProfiling);
    void Finalize();

    void ExecuteTask();
    void AppendTask(Task* task, TaskPriority priority);

    void CancelTask(Task* task);
    void CancelTaskById(uint32_t id);
    void CancelAllTask();

    void WaitTask();
    void CancelWaitTask();

    bool TryRemoveTask(Task* task);

    void RegisterTaskProfileReader(TaskProfileReader& profileReader) {
        m_TaskProfileLogger.RegisterReader(profileReader);
    }

    void UnregisterTaskProfileReader(const TaskProfileReader& profileReader) {
        m_TaskProfileLogger.UnregisterReader(profileReader);
    }

private:
    static const int32_t PriorityCount{3};
    static const int32_t ThreadMessageBufferSize{32};

    enum Message {
        Message_Append,
    };

    TaskManager();

    Task* PopTask();
    Task* PeekTask();

    void RemoveTaskById(uint32_t id);

    Task* GetNextTask(TaskPriority priority, bool doRemove);

    using TaskList =
        util::IntrusiveList<Task, util::IntrusiveListMemberNodeTraits<Task, &Task::m_TaskLink>>;

    TaskList m_TaskList[PriorityCount];
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    Task* m_ActiveTask{};
#endif
    volatile bool m_IsWaitTaskCancel{false};
    fnd::CriticalSection m_CriticalSection;
    os::MessageQueue m_BlockingQueue{m_MsgBuffer, ThreadMessageBufferSize};
    uintptr_t m_MsgBuffer[ThreadMessageBufferSize];
    TaskProfileLogger m_TaskProfileLogger;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(TaskManager) == 0x1e0);
#else
static_assert(sizeof(TaskManager) == 0x1d8);
#endif

}  // namespace nn::atk::detail
