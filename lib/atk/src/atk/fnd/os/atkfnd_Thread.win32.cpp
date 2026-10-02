#include <nn/atk/fnd/os/atkfnd_Thread.h>

#include <nn/fs/fs_Priority.h>
#include <nn/os.h>

namespace {

nn::fs::Priority GetNnFsPriority(nn::atk::detail::fnd::Thread::FsPriority priority) {}

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

}  // namespace nn::atk::detail::fnd
