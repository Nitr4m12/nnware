#include <nn/atk/atk_SoundHeap.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>

namespace nn::atk {

SoundHeap::SoundHeap() = default;

SoundHeap::~SoundHeap() {
    Destroy();
}

void SoundHeap::Clear() {
    if (!m_FrameHeap.IsValid())
        return;

    {
        detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_CriticalSection};
        m_FrameHeap.Clear();
    }

    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    if (cmdmgr.IsAvailable()) {
        uint32_t tag{cmdmgr.FlushCommand(true)};
        cmdmgr.WaitCommandReply(tag);
    }
}

}  // namespace nn::atk
