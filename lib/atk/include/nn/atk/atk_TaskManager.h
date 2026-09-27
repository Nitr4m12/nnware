#pragma once

#include <nn/os/os_MessageQueue.h>

#include <nn/atk/atk_Task.h>

namespace nn::atk::detail {

class TaskManager {
public:
    using TaskList =
        util::IntrusiveList<Task, util::IntrusiveListMemberNodeTraits<Task, &Task::m_TaskLink>>;

    enum Message {
        Message_Append,
    };

    enum TaskPriority {
        TaskPriority_Low,
        TaskPriority_Middle,
        TaskPriority_High,
    };

    constexpr static uint32_t PriorityCount = 3;
    constexpr static uint32_t ThreadMessageBufferSize = 32;

    static TaskManager& GetInstance();

    TaskManager();
    ~TaskManager();

    void Initialize(bool isEnableProfiling);
    void Finalize();

    void AppendTask(Task* task, TaskPriority priority);

    Task* GetNextTask(TaskPriority priority, bool);

    Task* PopTask();
    Task* PeekTask();

    void ExecuteTask();
    void CancelTask(Task* task);

    bool TryRemoveTask(Task* task);

    void CancelTaskById(uint32_t id);
    void RemoveTaskById(uint32_t id);

    void CancelAllTask();

    void WaitTask();
    void CancelWaitTask();

private:
    TaskList m_TaskList[PriorityCount];
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    Task* m_ActiveTask;
#endif
    bool m_IsWaitTaskCancel;
    fnd::CriticalSection m_CriticalSection;
    os::MessageQueue m_BlockingQueue;
    std::uintptr_t m_MsgBuffer[ThreadMessageBufferSize];
    TaskProfileLogger m_TaskProfileLogger;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(TaskManager) == 0x1e0);
#else
static_assert(sizeof(TaskManager) == 0x1d8);
#endif

}  // namespace nn::atk::detail
