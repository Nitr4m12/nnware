#include <nn/atk/atk_WaveSoundPlayer.h>

namespace nn::atk::detail::driver {

WaveSoundPlayer::WaveSoundPlayer() = default;

WaveSoundPlayer::~WaveSoundPlayer() {
    Finalize();
}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
void WaveSoundPlayer::Initialize()
#else
void WaveSoundPlayer::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    BasicSoundPlayer::Initialize();
#else
    BasicSoundPlayer::Initialize(pOutputReceiver);
#endif

    m_ReleasePriorityFixFlag = false;
    m_Priority = DefaultPriority;

    m_PanRange = 1.0f;

    m_pWsdFile = nullptr;
    m_pWaveFile = nullptr;

    m_WaveSoundIndex = -1;
    m_DelayCount = 0;
    m_UpdateType = UpdateType_AudioFrame;

    m_WaveSoundInfo.pitch = 1.0f;
    m_WaveSoundInfo.pan = DefaultPriority;
    m_WaveSoundInfo.surroundPan = 0;

    for (int i{0}; i < AuxBus_Count; ++i)
        m_WaveSoundInfo.fxSend[i] = 0;
    m_WaveSoundInfo.mainSend = 127;

    m_LfoParam.Initialize();

    m_WaveType = WaveType_Invalid;
    m_WavePlayFlag = false;

    m_pChannel = nullptr;
    m_IsRegisterPlayerCallback = false;
    m_ResState = ResState_Invalid;
    m_IsInitialized = true;
}

}  // namespace nn::atk::detail::driver
