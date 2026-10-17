#include <nn/atk/atk_SoundHeap.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_SoundSystem.h>
#include <nn/atk/fnd/os/atkfnd_ScopedLock.h>

namespace nn::atk {

SoundHeap::SoundHeap() = default;

SoundHeap::~SoundHeap() {
    Destroy();
}

void SoundHeap::Destroy() {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_CriticalSection};
    if (!m_FrameHeap.IsValid())
        return;

    Clear();

    if (m_IsAutoMemoryPoolManagementEnabled) {
        SoundSystem::DetachMemoryPool(&m_MemoryPool);
        m_IsAutoMemoryPoolManagementEnabled = false;
    }

    m_FrameHeap.Destroy();
}

bool SoundHeap::Create(void* startAddress, size_t size) {
    return Create(startAddress, size, true);
}

bool SoundHeap::Create(void* startAddress, size_t size, bool enableAutoMemoryPoolManagement) {
    detail::Util::IsValidMemoryForDsp(startAddress, size);
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_CriticalSection};

    void* frameHeapAddress{startAddress};
    size_t frameHeapSize{size};

    if (enableAutoMemoryPoolManagement) {
        void* alignedStartAddress{
            util::BytePtr(startAddress).AlignUp(audio::MemoryPoolType::AddressAlignment).Get(),
        };
        ptrdiff_t alignedSizeDistance{util::BytePtr(startAddress).Distance(alignedStartAddress)};
        if (alignedSizeDistance > size)
            return false;

        frameHeapAddress = alignedStartAddress;
        frameHeapSize =
            util::align_down(size - alignedSizeDistance, audio::MemoryPoolType::AddressAlignment);
    }

    bool result{m_FrameHeap.Create(frameHeapAddress, frameHeapSize)};
    if (!result)
        return false;

    if (enableAutoMemoryPoolManagement) {
        SoundSystem::AttachMemoryPool(&m_MemoryPool, frameHeapAddress, frameHeapSize);
        m_IsAutoMemoryPoolManagementEnabled = true;
    }
    return true;
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
