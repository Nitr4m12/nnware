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
        const SoundInstanceConfig config{SoundSystem::GetSoundInstanceConfig()};

        size_t requireSize{
            util::align_up(AdvancedWaveSoundInstanceManager::GetRequiredMemSize(soundCount, config),
                           64),
        };

        void* estimateEndAddr{util::BytePtr(*pOutAllocatedAddr, requireSize).Get()};
        if (util::ConstBytePtr(endAddr).Distance(estimateEndAddr) > 0) {
            isSuccess = false;
        } else {
            size_t createdCount{
                static_cast<size_t>(
                    m_InstanceManager.Create(*pOutAllocatedAddr, requireSize, config)),
            };
            *pOutAllocatedAddr = estimateEndAddr;
            isSuccess = static_cast<int64_t>(createdCount) == soundCount;
            return isSuccess;
        }
    }

    return isSuccess;
}

}  // namespace nn::atk::detail
