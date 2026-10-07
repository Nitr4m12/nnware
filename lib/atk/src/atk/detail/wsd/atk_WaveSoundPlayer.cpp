#include <nn/atk/atk_WaveSoundPlayer.h>

#include <nn/atk/atk_DisposeCallbackManager.h>

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

void WaveSoundPlayer::Finalize() {
    FinishPlayer();

    if (IsActive()) {
        DisposeCallbackManager::GetInstance().UnregisterDisposeCallback(this);
        CloseChannel();
        SetActiveFlag(false);
    }

    if (m_IsInitialized) {
        BasicSoundPlayer::Finalize();
        m_IsInitialized = false;
    }

    FreeLoader();
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

void WaveSoundPlayer::Prepare(const StartInfo& info, const PrepareArg& arg) {
    if (IsActive())
        FinishPlayer();

    m_WaveSoundIndex = info.index;
    m_StartOffsetType = info.startOffsetType;
    m_StartOffset = info.startOffset;
    m_DelayCount = info.delayCount != 0 ? info.delayCount : ToDelayCount(info.delayTime);
    m_WaveSoundParameterFlag = info.waveSoundParameterFlag;
    m_Release = info.release;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_IsContextCalculationSkipMode =
        (m_WaveSoundParameterFlag & 0b10) != 0 && info.isContextCalculationSkipMode;
#endif
    m_UpdateType = info.updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = info.subMixIndex;
#endif

    m_pWsdFile = arg.wsdFile;
    m_pWaveFile = arg.waveFile;
    m_WaveType = arg.waveType;

    m_ResState = ResState_Assigned;

    SetActiveFlag(true);

    DisposeCallbackManager::GetInstance().RegisterDisposeCallback(this);
    SoundThread::GetInstance().RegisterPlayerCallback(this);
    m_IsRegisterPlayerCallback = true;
}

void WaveSoundPlayer::PrepareForPlayerHeap(const PrepareArg& arg) {
    if (IsActive())
        FinishPlayer();

    m_pWsdFile = arg.wsdFile;
    m_pWaveFile = arg.waveFile;
    m_WaveType = arg.waveType;

    m_ResState = ResState_Assigned;

    SetActiveFlag(true);

    DisposeCallbackManager::GetInstance().RegisterDisposeCallback(this);
}

void WaveSoundPlayer::RequestLoad(const StartInfo& info, const WaveSoundLoader::Arg& arg) {
    m_WaveSoundIndex = info.index;
    m_StartOffsetType = info.startOffsetType;
    m_StartOffset = info.startOffset;
    m_DelayCount = info.delayCount != 0 ? info.delayCount : ToDelayCount(info.delayTime);
    m_Release = info.release;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_IsContextCalculationSkipMode =
        (m_WaveSoundParameterFlag & 0b10) != 0 && info.isContextCalculationSkipMode;
#endif
    m_UpdateType = info.updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = info.subMixIndex;
#endif

    m_LoaderArg = arg;
    m_ResState = ResState_ReceiveLoadReq;

    SoundThread::GetInstance().RegisterPlayerCallback(this);
    m_IsRegisterPlayerCallback = true;
}

void WaveSoundPlayer::Start() {
    SetStartedFlag(true);
}

void WaveSoundPlayer::Stop() {
    FinishPlayer();
}

void WaveSoundPlayer::Pause(bool flag) {
    SetPauseFlag(flag);

    if (IsChannelActive() && m_pChannel->IsPause() != flag) {
        m_pChannel->Pause(flag);
    }
}

}  // namespace nn::atk::detail::driver
