#include <nn/atk/fnd/basis/atkfnd_Time.h>

namespace nn::atk::detail::fnd {

TimeSpan Time::Current() {
    return os::GetSystemTick().GetInt64Value();
}

}  // namespace nn::atk::detail::fnd
