#include <nn/atk/atk_SoundHeap.h>

namespace nn::atk {

SoundHeap::SoundHeap() = default;

SoundHeap::~SoundHeap() {
    Destroy();
}

}  // namespace nn::atk
