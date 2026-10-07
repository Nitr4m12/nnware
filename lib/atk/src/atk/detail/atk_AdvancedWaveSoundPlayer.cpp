#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>

namespace {

const uint32_t SoundFrameIntervalMicroSeconds{5000};

}  // anonymous namespace

namespace nn::atk::detail::driver {

AdvancedWaveSoundPlayer::AdvancedWaveSoundPlayer() = default;

AdvancedWaveSoundPlayer::~AdvancedWaveSoundPlayer() {
    Finalize();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void AdvancedWaveSoundPlayer::Initialize()
#else
void AdvancedWaveSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
    if (!m_IsInitialized) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        BasicSoundPlayer::Initialize();
#else
        BasicSoundPlayer::Initialize(pOutputReceiver);
#endif
        m_IsInitialized = true;
    }

    m_IsPrepared = false;
}

}  // namespace nn::atk::detail::driver
