#pragma once

#include <nn/os/os_Mutex.h>

namespace nn::atk::detail::fnd {

class CriticalSection {
public:
    CriticalSection() : m_Mutex(true) {}

    ~CriticalSection() = default;

    void Enter() { m_Mutex.Lock(); }

    bool TryEnter() { return m_Mutex.TryLock(); }

    void Leave() { m_Mutex.Unlock(); }

    void Lock() { m_Mutex.Lock(); }

    bool TryLock() { return m_Mutex.TryLock(); }

    void Unlock() { m_Mutex.Unlock(); }

private:
    static const uint32_t CriticalSectionSize{24};
    static const uint32_t CriticalSectionAlign{4};

    os::Mutex m_Mutex;
};
static_assert(sizeof(CriticalSection) == 0x20);

}  // namespace nn::atk::detail::fnd
