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

}  // namespace nn::atk::detail::fnd
