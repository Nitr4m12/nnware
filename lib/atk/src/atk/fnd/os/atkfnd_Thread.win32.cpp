#include <nn/atk/fnd/os/atkfnd_Thread.h>

#include <nn/fs.h>
#include <nn/fs/fs_Priority.h>
#include <nn/os.h>

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
    static void Run(void* ptrArg);
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
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        m_FsPriority = args.fsPriority;
        os::StartThread(&handle);
#endif
        m_Id = reinterpret_cast<int64_t>(&handle);
        return true;
    } else {
        m_Id = InvalidId;
        return false;
    }
}

}  // namespace nn::atk::detail::fnd
