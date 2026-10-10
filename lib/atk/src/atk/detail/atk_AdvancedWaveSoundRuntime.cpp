#include <nn/atk/detail/atk_AdvancedWaveSoundRuntime.h>

#include <nn/atk/atk_SoundSystem.h>
#include <nn/util/util_BytePtr.h>

namespace nn::atk::detail {

AdvancedWaveSoundRuntime::AdvancedWaveSoundRuntime() = default;

AdvancedWaveSoundRuntime::~AdvancedWaveSoundRuntime() = default;

bool AdvancedWaveSoundRuntime::Initialize(int32_t soundCount, void** pOutAllocatedAddr,
                                          const void* endAddr) {
    bool isSuccess{false};

    if (reinterpret_cast<uintptr_t>(*pOutAllocatedAddr) % 64 == 0) {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        size_t requireSize{
            util::align_up(AdvancedWaveSoundInstanceManager::GetRequiredMemSize(soundCount), 64),
        };
#else
        size_t requireSize{
            util::align_up(AdvancedWaveSoundInstanceManager::GetRequiredMemSize(soundCount, config),
                           64),
        };
#endif

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0) {
            isSuccess = false;
        } else {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
            size_t createdCount{
                static_cast<size_t>(m_InstanceManager.Create(*pOutAllocatedAddr, requireSize)),
            };
#else
            size_t createdCount{
                static_cast<size_t>(
                    m_InstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
            };
#endif
            *pOutAllocatedAddr = estimateEndAddr;
            isSuccess = static_cast<int64_t>(createdCount) == soundCount;
            return isSuccess;
        }
    }

    return isSuccess;
}

void AdvancedWaveSoundRuntime::Finalize() {
    m_InstanceManager.Destroy();
}

size_t AdvancedWaveSoundRuntime::GetRequiredMemorySize(
    const SoundArchive::SoundArchivePlayerInfo& soundArchivePlayerInfo, size_t alignmentSize) {
    size_t size{0};
    size = soundArchivePlayerInfo.waveSoundCount;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    size = AdvancedWaveSoundInstanceManager::GetRequiredMemSize(size);
#else
    size = AdvancedWaveSoundInstanceManager::GetRequiredMemSize(
        size, SoundSystem::GetSoundInstanceConfig());
#endif
    size = util::align_up(size, alignmentSize);

    return size;
}

int32_t AdvancedWaveSoundRuntime::GetFreeAdvancedWaveSoundCount() const {
    return m_InstanceManager.GetFreeCount();
}

void AdvancedWaveSoundRuntime::SetupUserParam(void** startAddr, size_t adjustSize) {
    void* curAddr{*startAddr};

    auto& list{m_InstanceManager.GetFreeList()};
    for (auto itr(list.begin()); itr != list.end(); ++itr) {
        itr->SetUserParamBuffer(curAddr, adjustSize);
        curAddr = util::BytePtr(curAddr, adjustSize).Get();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    *startAddr = curAddr;
#endif
}

}  // namespace nn::atk::detail
