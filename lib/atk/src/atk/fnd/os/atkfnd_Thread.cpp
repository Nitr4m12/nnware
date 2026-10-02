#include <nn/atk/fnd/os/atkfnd_Thread.h>

namespace nn::atk::detail::fnd {

Thread::RunArgs::RunArgs() = default;

bool Thread::RunArgs::IsValid() const {
    if (stack == nullptr)
        return false;

    if (stackSize == 0)
        return false;

    if (priority > MaxThreadPriority)
        return false;

    return handler != nullptr;
}

Thread::~Thread() = default;

bool Thread::Run(const RunArgs& args) {
    if (!args.IsValid())
        return false;

    m_Param = args.param;
    m_Handler = args.handler;

    if (!Create(m_Handle, m_Id, args))
        return false;

    SetName(args.name);
    if (args.affinityMask != AffinityMask_CoreDefault)
        SetAffinityMask(args.idealCoreNumber, args.affinityMask);

    m_Priority = args.priority;
    Resume();

    return true;
}

void Thread::WaitForExit() {
    Join();
}

void Thread::Release() {
    if (m_State != State_Exited)
        return;

    Detach();
    SetState(State_Released);
}

void Thread::SetState(State value) {
    m_State = value;
}

int32_t Thread::GetPriority() const {
    return m_Priority;
}

Thread::State Thread::GetState() const {
    return static_cast<State>(m_State);
}

void Thread::OnRun() {
    SetState(State_Running);
}

}  // namespace nn::atk::detail::fnd
