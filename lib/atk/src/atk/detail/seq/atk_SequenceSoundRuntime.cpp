#include <nn/atk/atk_SequenceSoundRuntime.h>

#include <nn/atk/atk_SoundSystem.h>

namespace nn::atk::detail {

SequenceSoundRuntime::SequenceSoundRuntime() {
    m_SequenceCallback.m_pSequenceSoundRuntime = this;
}

SequenceSoundRuntime::~SequenceSoundRuntime() {
    m_pSoundArchiveManager = nullptr;
}

bool SequenceSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                      const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 != 0)
        return false;

    {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size_t requireSize{
            util::align_up(SequenceSoundInstanceManager::GetRequiredMemSize(soundCount), 64),
        };
#else
        size_t requireSize{
            util::align_up(SequenceSoundInstanceManager::GetRequiredMemSize(soundCount, config),
                           64),
        };
#endif

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_SequenceSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize)),
        };
#else
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_SequenceSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
        };
#endif
        *pOutAllocatedAddr = estimateEndAddr;
    }

    {
        size_t requireSize{
            util::align_up(driver::SequenceSoundLoaderManager::GetRequiredMemSize(soundCount), 64),
        };

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

        [[maybe_unused]] uint32_t createdSoundLoaderCount{
            static_cast<uint32_t>(
                m_SequenceSoundLoaderManager.Create(*pOutAllocatedAddr, requireSize)),
        };

        *pOutAllocatedAddr = estimateEndAddr;
    }

    auto& list{m_SequenceSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr)
        itr->SetLoaderManager(m_SequenceSoundLoaderManager);

    driver::SoundThread::GetInstance().RegisterSoundFrameCallback(&m_SequenceSoundLoaderManager);
    m_pSequenceTrackAllocator = &m_MmlSequenceTrackAllocator;

    return true;
}

void SequenceSoundRuntime::Finalize() {
    m_pSequenceTrackAllocator = nullptr;

    m_SequenceSoundInstanceManager.Destroy();
    m_SequenceSoundLoaderManager.Destroy();

    driver::SoundThread::GetInstance().UnregisterSoundFrameCallback(&m_SequenceSoundLoaderManager);
    m_MmlSequenceTrackAllocator.Destroy();
}

bool SequenceSoundRuntime::SetupSequenceTrack(int32_t trackCount, void** pOutAllocatedAddr,
                                              const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 != 0)
        return false;

    size_t requireSize{util::align_up(trackCount * (sizeof(driver::SequenceTrack) + 0x8), 64)};

    void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
    if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
        return false;

    [[maybe_unused]] int32_t createdTrackCount{
        m_MmlSequenceTrackAllocator.Create(*pOutAllocatedAddr, requireSize)};
    *pOutAllocatedAddr = estimateEndAddr;
    return true;
}

void SequenceSoundRuntime::SetupUserParam(void** pOutAllocatedAddr, size_t adjustSize) {
    void* curAddr{*pOutAllocatedAddr};

    auto& list{m_SequenceSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr) {
        itr->SetUserParamBuffer(curAddr, adjustSize);
        curAddr = util::BytePtr(curAddr, adjustSize).Get();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    *pOutAllocatedAddr = curAddr;
#endif
}

}  // namespace nn::atk::detail
