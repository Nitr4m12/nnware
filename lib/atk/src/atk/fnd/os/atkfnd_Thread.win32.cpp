#include <nn/atk/fnd/os/atkfnd_Thread.h>

#include <nn/fs.h>
#include <nn/fs/fs_Priority.h>
#include <nn/os.h>
#include <nn/util.h>

#include <nn/atk/fnd/basis/atkfnd_Time.h>

namespace {

nn::fs::Priority GetNnFsPriority(nn::atk::detail::fnd::Thread::FsPriority priority) {
    switch (priority) {
    case nn::atk::detail::fnd::Thread::FsPriority_RealTime:
        return nn::fs::Priority_Realtime;
    case nn::atk::detail::fnd::Thread::FsPriority_Normal:
        return nn::fs::Priority_Normal;
    case nn::atk::detail::fnd::Thread::FsPriority_Low:
        return nn::fs::Priority_Low;
    }
}

}  // anonymous namespace

namespace nn::atk::detail::fnd {

class Thread::ThreadMain {
public:
    static void Run(void* ptrArg) {
        Thread* owner{static_cast<Thread*>(ptrArg)};

        owner->m_IsTerminated = false;
        owner->OnRun();
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        switch (owner->GetFsPriority()) {
        default:
            NN_UNEXPECTED_DEFAULT;
        case FsPriority_RealTime:
        case FsPriority_Normal:
        case FsPriority_Low:
            owner->SetFsPriority(owner->GetFsPriority());
#endif
            owner->m_Handler->Run(owner->m_Param);
            owner->OnExit();
            owner->m_IsTerminated = true;
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
            break;
        }
#endif
    }
};

Thread::Thread() = default;

void Thread::SetPriority(int32_t value) {
    os::ChangeThreadPriority(&m_Handle, value);
}

void Thread::SetFsPriority(FsPriority value) {
    fs::SetPriorityOnCurrentThread(GetNnFsPriority(value));
}

Thread::FsPriority Thread::GetFsPriority() const {
    return m_FsPriority;
}

// TODO: implement nn::TimeSpan and nn::TimeSpanType in nnsdk
// void Thread::Sleep(const fnd::TimeSpan& timeSpan) {
//     os::SleepThread(timeSpan.ToNanoSeconds());
// }

bool Thread::Create(Handle& handle, [[maybe_unused]] int64_t& id, const RunArgs& args) {
    if (os::CreateThread(&handle, ThreadMain::Run, this, args.stack, args.stackSize, args.priority,
                         args.idealCoreNumber)
            .IsSuccess()) {
#if NN_WARE_VER >= NN_MAKE_VER(5, 0, 0)
        m_FsPriority = args.fsPriority;
#endif
        os::StartThread(&handle);
        m_Id = reinterpret_cast<int64_t>(&handle);
        return true;
    } else {
        m_Id = InvalidId;
        return false;
    }
}

void Thread::Detach() {
    os::DestroyThread(&m_Handle);
}

void Thread::SetName(const char* name) {
    os::SetThreadNamePointer(&m_Handle, name == nullptr ? "" : name);
}

void Thread::SetAffinityMask(int32_t idealCoreNumber, AffinityMask value) {
    os::SetThreadCoreMask(&m_Handle, idealCoreNumber, value);
}

void Thread::Resume() {}

void Thread::Join() {
    os::WaitThread(&m_Handle);
}

}  // namespace nn::atk::detail::fnd
