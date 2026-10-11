#include <nn/atk/atk_WaveSoundRuntime.h>

#include <nn/atk/atk_SoundSystem.h>

namespace {

const int MemAlignSize{64};

}  // anonymous namespace

namespace nn::atk::detail {

WaveSoundRuntime::WaveSoundRuntime() = default;

WaveSoundRuntime::~WaveSoundRuntime() = default;

bool WaveSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                  const void* endAddr) {
    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 != 0)
        return false;

    {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size_t requireSize{
            util::align_up(WaveSoundInstanceManager::GetRequiredMemSize(soundCount), 64),
        };
#else
        size_t requireSize{
            util::align_up(WaveSoundInstanceManager::GetRequiredMemSize(soundCount, config), 64),
        };
#endif

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(m_WaveSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize)),
        };
#else
        [[maybe_unused]] size_t createdCount{
            static_cast<size_t>(
                m_WaveSoundInstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
        };
#endif
        *pOutAllocatedAddr = estimateEndAddr;
    }

    {
        size_t requireSize{driver::WaveSoundLoaderManager::GetRequiredMemSize(soundCount)};

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0)
            return false;

        [[maybe_unused]] uint32_t createdSoundLoaderCount{
            static_cast<uint32_t>(m_WaveSoundLoaderManager.Create(*pOutAllocatedAddr, requireSize)),
        };

        *pOutAllocatedAddr = estimateEndAddr;
    }

    auto& list{m_WaveSoundInstanceManager.GetFreeList()};
    for (auto itr{list.begin()}; itr != list.end(); ++itr)
        itr->SetLoaderManager(m_WaveSoundLoaderManager);

    driver::SoundThread::GetInstance().RegisterSoundFrameCallback(&m_WaveSoundLoaderManager);

    return true;
}

void WaveSoundRuntime::Finalize() {
    m_WaveSoundInstanceManager.Destroy();
    m_WaveSoundLoaderManager.Destroy();

    driver::SoundThread::GetInstance().UnregisterSoundFrameCallback(&m_WaveSoundLoaderManager);
}

size_t WaveSoundRuntime::GetRequiredMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, int32_t alignment) {
    size_t size{0};
    size = soundArchivePlayerInfo.waveSoundCount;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size = WaveSoundInstanceManager::GetRequiredMemSize(size);
#else
    size =
        WaveSoundInstanceManager::GetRequiredMemSize(size, SoundSystem::GetSoundInstanceConfig());
#endif
    size = util::align_up(size, alignment);

    size += util::align_up(
        driver::WaveSoundLoaderManager::GetRequiredMemSize(soundArchivePlayerInfo.waveSoundCount),
        alignment);

    return size;
}

int32_t WaveSoundRuntime::GetActiveCount() const {
    return m_WaveSoundInstanceManager.GetActiveCount();
}

int32_t WaveSoundRuntime::GetFreeWaveSoundCount() const {
    return m_WaveSoundInstanceManager.GetFreeCount();
}

}  // namespace nn::atk::detail
