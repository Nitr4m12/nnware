#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>

namespace {

const uint32_t SoundFrameIntervalMicroSeconds{5000};

}  // anonymous namespace

namespace nn::atk::detail::driver {

AdvancedWaveSoundPlayer::AdvancedWaveSoundPlayer() = default;

AdvancedWaveSoundPlayer::~AdvancedWaveSoundPlayer() {
    Finalize();
}

}  // namespace nn::atk::detail::driver
