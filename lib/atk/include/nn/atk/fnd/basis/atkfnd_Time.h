#pragma once

#include <nn/os.h>

namespace nn::atk::detail::fnd {

class TimeSpan {
public:
    using TickType = int64_t;

    TimeSpan() = default;
    TimeSpan(TickType tickType) : m_TickSpan(tickType) {}

    static TimeSpan FromNanoSeconds(int64_t value);
    static TimeSpan FromMicroSeconds(int64_t value);
    static TimeSpan FromMilliSeconds(int64_t value);

    TickType GetTick() const { return m_TickSpan; }

    int64_t ToNanoSeconds() const;
    int64_t ToMicroSeconds() const;
    int64_t ToMilliSeconds() const;

    TimeSpan& operator=(const TimeSpan& rhs);
    TimeSpan& operator=(TickType value);

    operator TickType() const;

    TimeSpan operator+(const TimeSpan& rhs) const;
    TimeSpan operator-(const TimeSpan& rhs) const;
    TimeSpan operator*(const TimeSpan& rhs) const;
    TimeSpan operator/(const TimeSpan& rhs) const;

    TimeSpan& operator+=(const TimeSpan& rhs);
    TimeSpan& operator-=(const TimeSpan& rhs);
    TimeSpan& operator*=(const TimeSpan& rhs);
    TimeSpan& operator/=(const TimeSpan& rhs);

    bool operator==(const TimeSpan& rhs) const;
    bool operator!=(const TimeSpan& rhs) const;
    bool operator<(const TimeSpan& rhs) const;
    bool operator>(const TimeSpan& rhs) const;
    bool operator<=(const TimeSpan& rhs) const;
    bool operator>=(const TimeSpan& rhs) const;

private:
    TickType m_TickSpan{0};
};
static_assert(sizeof(TimeSpan) == 0x8);

class Time {
public:
    Time();

    static TimeSpan Current();
};
static_assert(sizeof(Time) == 0x1);

}  // namespace nn::atk::detail::fnd
