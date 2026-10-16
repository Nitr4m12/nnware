#include <nn/atk/fnd/basis/atkfnd_Time.h>

namespace {

int64_t NanoSecondsToTicks(int64_t value) {
    nn::TimeSpan timeSpan{nn::TimeSpanType{value}};
    nn::os::Tick tick{nn::os::ConvertToTick(timeSpan)};
    return tick.GetInt64Value();
}

int64_t MicroSecondsToTicks(int64_t value) {
    return NanoSecondsToTicks(value * 1000);
}

}  // anonymous namespace

namespace nn::atk::detail::fnd {

TimeSpan Time::Current() {
    return os::GetSystemTick().GetInt64Value();
}

TimeSpan TimeSpan::FromNanoSeconds(int64_t value) {
    return NanoSecondsToTicks(value);
}

TimeSpan TimeSpan::FromMicroSeconds(int64_t value) {
    return MicroSecondsToTicks(value);
}

}  // namespace nn::atk::detail::fnd
