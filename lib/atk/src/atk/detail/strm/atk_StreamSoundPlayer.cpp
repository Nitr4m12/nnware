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

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void StreamSoundPlayer::Initialize()
#else
void StreamSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    BasicSoundPlayer::Initialize();
#else
    BasicSoundPlayer::Initialize(pOutputReceiver);
#endif
    m_LoopCounter = 0;

}

bool StreamSoundPlayer::TryAllocLoader() {
    if (m_pLoader != nullptr)
        return true;

    if (m_pLoaderManager == nullptr)
        return false;

    StreamSoundLoader* loader{m_pLoaderManager->Alloc()};
    if (loader == nullptr)
        return false;

    m_pLoader = loader;
    return true;
}

}  // namespace nn::atk::detail::driver
