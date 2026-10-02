#include <nn/atk/fnd/os/atkfnd_Thread.h>

namespace nn::atk::detail::fnd {

class Thread::ThreadMain {
public:
    static void Run(void* ptrArg);
};

Thread::Thread() = default;

}  // namespace nn::atk::detail::fnd
