#pragma once

#include <nn/audio/audio_MemoryPoolTypes.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_FrameHeap.h>
#include <nn/atk/atk_SoundMemoryAllocatable.h>
#include <nn/atk/fnd/os/atkfnd_CriticalSection.h>

namespace nn::atk {

class SoundHeap : public SoundMemoryAllocatable {
public:
    SoundHeap();
    ~SoundHeap() override;

    bool Create(void* startAddress, size_t size);
    bool Create(void* startAddress, size_t size, bool enableAutoMemoryPoolManagement);

    void Destroy();

    void* Allocate(size_t size) override;
    void* Allocate(size_t size, SoundMemoryAllocatable::DisposeCallback heapCallback,
                   void* heapCallbackArg) override;

    size_t GetAllocateSize(size_t size, bool needMemoryPool) override;

    void Clear();

    bool IsValid() const { return m_FrameHeap.IsValid(); }

    int32_t SaveState();
    void LoadState(int32_t level);

    int32_t GetCurrentLevel() const { return m_FrameHeap.GetCurrentLevel(); }

    size_t GetSize() const { return m_FrameHeap.GetSize(); }

    size_t GetFreeSize() const { return m_FrameHeap.GetFreeSize(); }

    void Dump(const SoundDataManager&, const SoundArchive&) const;

private:
    static void DisposeCallbackFunc(void* mem, size_t size, void* arg);

    detail::fnd::CriticalSection m_CriticalSection;
    detail::FrameHeap m_FrameHeap;
    audio::MemoryPoolType m_MemoryPool;
    bool m_IsAutoMemoryPoolManagementEnabled{false};
};
static_assert(sizeof(SoundHeap) == 0x50);

}  // namespace nn::atk
