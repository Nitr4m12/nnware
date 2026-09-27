#pragma once

#include <nn/os/os_Event.h>

#include <nn/atk/atk_TaskProfileReader.h>

namespace nn::atk::detail {

class TaskManager;

class Task {
public:
    enum Status {
        Status_Free,
        Status_Append,
        Status_Execute,
        Status_Done,
        Status_Cancel,
    };

    Task();

    virtual ~Task();

    void SetId(uint32_t id) { m_Id = id; }
    Status GetStatus() const { return m_Status; };

    void Wait() { m_Event.Wait(); }
    bool TryWait() { return m_Event.TryWait(); }

protected:
    virtual void Execute(TaskProfileLogger& logger) = 0;

    void InitializeStatus() { m_Status = Status_Free; }

private:
    NN_NO_COPY(Task);

    friend TaskManager;

    util::IntrusiveListNode m_TaskLink;
    os::Event m_Event;
    Status m_Status;
    uint32_t m_Id;
};
static_assert(sizeof(Task) == 0x48);

}  // namespace nn::atk::detail
