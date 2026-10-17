#pragma once

#include <nn/atk/atk_Global.h>
#include <nn/atk/fnd/os/atkfnd_CriticalSection.h>
#include <nn/atk/fnd/os/atkfnd_Thread.h>

namespace nn::atk::detail {

class TaskThread : public fnd::Thread::Handler {
public:
    TaskThread();
    ~TaskThread() override;

    bool Create(int32_t priority, void* stackBase, size_t stackSize, int32_t idealCoreNumber,
                uint32_t affinityMask, FsPriority fsPriority);

    void Destroy();

    bool IsCreated() const { return m_IsCreated; }

    void Lock() { m_CriticalSection.Lock(); }
    void Unlock() { m_CriticalSection.Unlock(); }

    static TaskThread& GetInstance();

protected:
    uint32_t Run(void* param) override;

private:
    fnd::Thread m_Thread;
    fnd::CriticalSection m_CriticalSection;
    volatile bool m_IsFinished{false};
    bool m_IsCreated{false};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    FsPriority m_FsPriority{FsPriority_Normal};
#endif
};
static_assert(sizeof(TaskThread) == 0x220);

}  // namespace nn::atk::detail
