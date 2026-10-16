#include <nn/atk/fnd/basis/atkfnd_Time.h>

namespace {

int64_t NanoSecondsToTicks(int64_t value) {
    nn::TimeSpan timeSpan{nn::TimeSpanType{value}};
    nn::os::Tick tick{timeSpan};
    return tick.GetInt64Value();
}

int64_t MicroSecondsToTicks(int64_t value) {
    return NanoSecondsToTicks(value * 1000);
}

int64_t MilliSecondsToTicks(int64_t value) {
    return MicroSecondsToTicks(value * 1000);
}

int64_t TicksToNanoSeconds(int64_t ticks) {
    nn::os::Tick tick{ticks};
    nn::TimeSpan timeSpan{tick.ToTimeSpan()};
    return timeSpan.GetNanoSeconds();
}

int64_t TicksToMicroSeconds(int64_t ticks) {
    nn::os::Tick tick{ticks};
    nn::TimeSpan timeSpan{tick.ToTimeSpan()};
    return timeSpan.GetMicroSeconds();
}

int64_t TicksToMilliSeconds(int64_t ticks) {
    nn::os::Tick tick{ticks};
    nn::TimeSpan timeSpan{tick.ToTimeSpan()};
    return timeSpan.GetMilliSeconds();
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

TimeSpan TimeSpan::FromMilliSeconds(int64_t value) {
    return MilliSecondsToTicks(value);
}

int64_t TimeSpan::ToNanoSeconds() const {
    return TicksToNanoSeconds(m_TickSpan);
}

int64_t TimeSpan::ToMicroSeconds() const {
    return TicksToMicroSeconds(m_TickSpan);
}

}  // namespace nn::atk::detail::fnd
