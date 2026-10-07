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
    m_PanRange = 1.0f;
    m_Priority = DefaultPriority;

    m_pWsdFile = nullptr;
    m_pWaveFile = nullptr;

    m_WaveSoundIndex = -1;
    m_DelayCount = 0;
    m_UpdateType = UpdateType_AudioFrame;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = 0;
#endif

    m_WaveSoundInfo.pitch = 1.0f;
    m_WaveSoundInfo.pan = DefaultPriority;
    m_WaveSoundInfo.surroundPan = 0;

    for (int i{0}; i < AuxBus_Count; ++i)
        m_WaveSoundInfo.fxSend[i] = 0;
    m_WaveSoundInfo.mainSend = 127;

    m_LfoParam.Initialize();

    m_WavePlayFlag = false;

    m_pChannel = nullptr;

    m_WaveType = WaveType_Invalid;
    m_ResState = ResState_Invalid;
    m_IsRegisterPlayerCallback = false;
    m_IsInitialized = true;
}

void WaveSoundPlayer::FinishPlayer() {
    SetFinishFlag(true);
    if (m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = false;
    }

    if (IsStarted())
        SetStartedFlag(false);
}

void WaveSoundPlayer::CloseChannel() {
    if (m_pChannel != nullptr) {
        if (IsChannelActive()) {
            UpdateChannel();
            m_pChannel->Release();
            if (m_pChannel == nullptr)
                return;
        }
        m_pChannel->DetachChannel(m_pChannel);
        m_pChannel = nullptr;
    }
}

void WaveSoundPlayer::FreeLoader() {
    if (m_pLoader != nullptr) {
        m_pLoaderManager->Free(m_pLoader);
        m_pLoader = nullptr;
    }
}

}  // namespace nn::atk::detail::driver
