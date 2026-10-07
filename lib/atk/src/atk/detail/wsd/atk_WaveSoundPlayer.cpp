#include <nn/atk/atk_WaveSoundPlayer.h>

#include <nn/atk/atk_DisposeCallbackManager.h>
#include "nn/atk/atk_WaveFileReader.h"

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

void WaveSoundPlayer::SetPanRange(float panRange) {
    m_PanRange = panRange;
}

void WaveSoundPlayer::SetChannelPriority(int32_t priority) {
    m_Priority = priority;
}

void WaveSoundPlayer::SetReleasePriorityFix(bool fix) {
    m_ReleasePriorityFixFlag = fix;
}

void WaveSoundPlayer::InvalidateData(const void* start, const void* end) {
    if (IsActive()) {
        const void* current{m_pWsdFile};

        if (start <= current && current <= end)
            FinishPlayer();
    }
}

position_t WaveSoundPlayer::GetPlaySamplePosition(bool isOriginalSamplePosition) const {
    if (m_pChannel == nullptr)
        return 0;

    return m_pChannel->GetCurrentPlayingSample(isOriginalSamplePosition);
}

void WaveSoundPlayer::Update() {
    switch (m_ResState) {
    case ResState_ReceiveLoadReq:
        if (!TryAllocLoader())
            return;

        m_pLoader->Initialize(m_LoaderArg);
    case ResState_AppendLoadTask: {
        if (!m_pLoader->TryWait())
            return;

        if (!m_pLoader->IsLoadSuccess()) {
            FinishPlayer();
            return;
        }

        PrepareArg arg;
        arg.wsdFile = m_pLoader->GetWsdFile();
        arg.waveFile = m_pLoader->GetWaveFile();

        PrepareForPlayerHeap(arg);
    }
    default:
        if (m_DelayCount > 0) {
            --m_DelayCount;
        } else if (IsActive() && IsStarted()) {
            if (!IsPause()) {
                if (!m_WavePlayFlag) {
                    if (!StartChannel()) {
                        FinishPlayer();
                        return;
                    }
                } else if (m_pChannel == nullptr) {
                    FinishPlayer();
                    return;
                }
            }
            UpdateChannel();
        }
    }
}

bool WaveSoundPlayer::TryAllocLoader() {
    if (m_pLoaderManager == nullptr)
        return false;

    WaveSoundLoader* loader{m_pLoaderManager->Alloc()};
    if (loader == nullptr)
        return false;

    m_pLoader = loader;
    m_ResState = ResState_AppendLoadTask;
    return true;
}

bool WaveSoundPlayer::StartChannel() {
    const int priority{GetChannelPriority() + DefaultPriority};

    WaveInfo waveInfo;
    {
        WaveFileReader reader{m_pWaveFile, m_WaveType};
        if (!reader.ReadWaveInfo(&waveInfo, nullptr))
            return false;
    }

    position_t startOffsetSamples{0};
    switch (m_StartOffsetType) {
    case StartOffsetType_Sample:
        startOffsetSamples = m_StartOffset;
        break;
    case StartOffsetType_Millisec:
        startOffsetSamples = (m_StartOffset * waveInfo.sampleRate) / 1000;
        break;
    }

    if (static_cast<uint32_t>(startOffsetSamples) > waveInfo.loopEndFrame)
        return false;

    Channel* channel{Channel::AllocChannel(waveInfo.channelCount <= 2 ? waveInfo.channelCount : 2,
                                           priority, ChannelCallbackFunc, this)};
    if (channel == nullptr)
        return false;

    int release{0};
    {
        WaveSoundFileReader reader{m_pWsdFile};
        if (!reader.ReadWaveSoundInfo(&m_WaveSoundInfo, m_WaveSoundIndex))
            return false;

        if ((m_WaveSoundParameterFlag & 1) != 0)
            release = m_Release;
        else
            release = m_WaveSoundInfo.adshr.GetRelease();
    }

    channel->SetAttack(m_WaveSoundInfo.adshr.GetAttack());
    channel->SetHold(m_WaveSoundInfo.adshr.GetHold());
    channel->SetDecay(m_WaveSoundInfo.adshr.GetDecay());
    channel->SetSustain(m_WaveSoundInfo.adshr.GetSustain());
    channel->SetRelease(release);

    channel->SetReleasePriorityFix(m_ReleasePriorityFixFlag);
    channel->SetUpdateType(m_UpdateType);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    channel->SetSubMixIndex(m_SubMixIndex);
    channel->Start(waveInfo, -1, static_cast<uint32_t>(startOffsetSamples));
#else
    channel->SetOutputReceiver(GetOutputReceiver());
    channel->Start(waveInfo, -1, static_cast<uint32_t>(startOffsetSamples),
                   m_IsContextCalculationSkipMode);
#endif

    m_pChannel = channel;
    m_WavePlayFlag = true;
    return true;
}

}  // namespace nn::atk::detail::driver
