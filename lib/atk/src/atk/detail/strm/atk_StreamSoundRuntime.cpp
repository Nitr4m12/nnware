#include <nn/atk/atk_StreamSoundRuntime.h>

#include <nn/atk/atk_SoundSystem.h>

namespace {
const int32_t MemAlignSize{64};
}

namespace nn::atk::detail {

StreamSoundRuntime::StreamSoundRuntime() = default;

StreamSoundRuntime::~StreamSoundRuntime() = default;

size_t StreamSoundRuntime::GetRequiredStreamInstanceSize(int32_t soundCount) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    return util::align_up(StreamSoundInstanceManager::GetRequiredMemSize(soundCount), MemAlignSize);
#else
    return util::align_up(StreamSoundInstanceManager::GetRequiredMemSize(
                              soundCount, SoundSystem::GetSoundInstanceConfig()),
                          MemAlignSize);
#endif
}

}  // namespace nn::atk::detail
