#include <nn/atk/atk_SoundThread.h>

namespace nn::atk::detail::driver {

SoundThread& SoundThread::GetInstance() {
    static SoundThread instance;
    return instance;
}

SoundThread::SoundThread() = default;

}  // namespace nn::atk::detail::driver
