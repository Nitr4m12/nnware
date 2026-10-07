#include <nn/atk/atk_WaveSoundPlayer.h>

namespace nn::atk::detail::driver {

WaveSoundPlayer::WaveSoundPlayer() = default;

WaveSoundPlayer::~WaveSoundPlayer() {
    Finalize();
}

}  // namespace nn::atk::detail::driver
