#include <nn/atk/atk_StreamSoundPlayer.h>

namespace {
const uint8_t OpusFileType{nn::atk::detail::StreamFileType_Opus};
const float OpusPitchMax{4.0f};
const uint32_t LoopRegionSizeMin{nn::atk::DataBlockSizeMarginSamples};
}  // anonymous namespace

namespace nn::atk::detail::driver {

StreamSoundPlayer::StreamSoundPlayer() = default;

StreamSoundPlayer::~StreamSoundPlayer() {
    Finalize();
}

}  // namespace nn::atk::detail::driver
