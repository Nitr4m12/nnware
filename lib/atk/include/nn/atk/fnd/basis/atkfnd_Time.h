#pragma once

#include <nn/os.h>

namespace nn::atk::detail::fnd {

class Time {
public:
    int64_t Current();
};

class TimeSpan {
public:
    using TickType = int64_t;

    static TimeSpan FromNanoSeconds(TickType);
    static TimeSpan FromMicroSeconds(TickType);
    static TimeSpan FromMilliSeconds(TickType);

    TickType ToNanoSeconds() const;
    TickType ToMicroSeconds() const;
    TickType ToMilliSeconds() const;

private:
    TickType m_TickSpan;
};
static_assert(sizeof(TimeSpan) == 0x8);

}  // namespace nn::atk::detail::fnd
