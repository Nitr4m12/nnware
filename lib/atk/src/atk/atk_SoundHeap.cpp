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

void* SoundHeap::Allocate(size_t size) {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_CriticalSection};

    void* buffer{m_FrameHeap.Alloc(size, DisposeCallbackFunc, nullptr, nullptr, nullptr)};
    return buffer;
}

void SoundHeap::DisposeCallbackFunc(void* mem, size_t size, [[maybe_unused]] void* arg) {
    detail::DriverCommand& cmdmgr{detail::DriverCommand::GetInstance()};
    if (cmdmgr.IsAvailable()) {
        auto* command{cmdmgr.AllocCommand<detail::DriverCommandInvalidateData>()};
        command->id = detail::DriverCommandId_InvalidateData;
        command->mem = mem;
        command->size = size;

        cmdmgr.PushCommand(command);
    }
}

void* SoundHeap::Allocate(size_t size, SoundMemoryAllocatable::DisposeCallback heapCallback,
                          void* heapCallbackArg) {
    detail::fnd::ScopedLock<detail::fnd::CriticalSection> lock{m_CriticalSection};

    void* buffer{
        m_FrameHeap.Alloc(size, DisposeCallbackFunc, nullptr, heapCallback, heapCallbackArg),
    };
    return buffer;
}

size_t SoundHeap::GetAllocateSize(size_t size, [[maybe_unused]] bool needMemoryPool) {
    return size;
}

}  // namespace nn::atk
