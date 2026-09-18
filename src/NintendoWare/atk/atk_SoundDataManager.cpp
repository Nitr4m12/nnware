#include <nn/atk/atk_SoundDataManager.h>

namespace nn::atk {

SoundDataManager::SoundDataManager() = default;

SoundDataManager::~SoundDataManager() = default;

size_t SoundDataManager::GetRequiredMemSize(const SoundArchive* arc) const {
    size_t size{0};
    size += arc->detail_GetFileCount() * sizeof(FileAddress) + 4;
    size = util::align_up(size, BufferAlignSize);

    return size;
}

}  // namespace nn::atk
