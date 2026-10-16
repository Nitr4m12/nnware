#include <nn/atk/fnd/basis/atkfnd_Time.h>

namespace {

int64_t NanoSecondsToTicks(int64_t value) {
    nn::TimeSpan timeSpan{nn::TimeSpanType{value}};
    nn::os::Tick tick{nn::os::ConvertToTick(timeSpan)};
    return tick.GetInt64Value();
}

}  // anonymous namespace

namespace nn::atk::detail::fnd {

TimeSpan Time::Current() {
    return os::GetSystemTick().GetInt64Value();
}

TimeSpan TimeSpan::FromNanoSeconds(int64_t value) {
    return NanoSecondsToTicks(value);
}

}  // namespace nn::atk::detail::fnd
