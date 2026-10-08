#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>

#include <nn/atk/atk_WaveArchiveFileReader.h>
#include <nn/atk/atk_WaveFileReader.h>

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

void AdvancedWaveSoundPlayer::Finalize() {
    TearDownPlayer();

    if (IsActive()) {
        SetActiveFlag(false);
        ReleaseTracks();
    }

    if (m_IsInitialized) {
        BasicSoundPlayer::Finalize();
        m_IsInitialized = false;
    }
}

void AdvancedWaveSoundPlayer::TearDownPlayer() {
    SetFinishFlag(true);

    if (m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = false;
    }

    if (IsStarted())
        SetStartedFlag(false);
}

void AdvancedWaveSoundPlayer::ReleaseTracks() {
    int trackCount{m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackCount};

    for (int trackIndex{0}; trackIndex < trackCount; ++trackIndex) {
        AdvancedWaveSoundTrackInfo& trackInfo{
            m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackInfo[trackIndex],
        };

        TrackParam& trackParam{m_TrackParamSet.trackParam[trackIndex]};

        for (int clipIndex{0}; clipIndex < trackInfo.waveSoundClipCount; ++clipIndex) {
            ClipParam& clipParam{trackParam.clipParam[clipIndex]};
            ReleaseClip(&clipParam);
        }
    }
}

void AdvancedWaveSoundPlayer::Start() {
    SetStartedFlag(true);
}

void AdvancedWaveSoundPlayer::Stop() {
    TearDownPlayer();
}

void AdvancedWaveSoundPlayer::Pause(bool isPauseEnabled) {
    SetPauseFlag(isPauseEnabled);
}

void AdvancedWaveSoundPlayer::Prepare(const PrepareParameter& parameter) {
    if (IsActive())
        TearDownPlayer();

    m_AdvancedWaveSoundInfo = parameter.advancedWaveSoundInfo;
    m_UpdateType = parameter.updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    m_SubMixIndex = parameter.subMixIndex;
#endif
    m_pAwsdFile = parameter.pAwsdFile;
    m_pWarcFile = parameter.pWarcFile;

    SetActiveFlag(true);
    SetupPlayer();

    if (!SetupTracks())
        TearDownPlayer();
    else
        m_IsPrepared = true;
}

void AdvancedWaveSoundPlayer::SetupPlayer() {
    if (!m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().RegisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = true;
    }

    m_CurrentTime = 0;
}

bool AdvancedWaveSoundPlayer::SetupTracks() {
    AdvancedWaveSoundFileReader reader{m_pAwsdFile};
    if (!reader.ReadWaveSoundTrackInfoSet(&m_AdvancedWaveSoundTrackInfoSet))
        return false;

    InitializeTrackParams();
    return true;
}

void AdvancedWaveSoundPlayer::Update() {
    if (!IsActive() || !IsStarted())
        return;

    if (!IsPause() && UpdateTracks())
        TearDownPlayer();
}

bool AdvancedWaveSoundPlayer::UpdateTracks() {
    bool isAllTrackPlayed{true};

    int trackCount{m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackCount};
    for (int trackIndex{0}; trackIndex < trackCount; ++trackIndex) {
        AdvancedWaveSoundTrackInfo& trackInfo{
            m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackInfo[trackIndex],
        };
        TrackParam& trackParam{m_TrackParamSet.trackParam[trackIndex]};

        bool isAllClipPlayed{true};
        for (int clipIndex{0}; clipIndex < trackInfo.waveSoundClipCount; ++clipIndex) {
            AdvancedWaveSoundClipInfo& waveSoundClipInfo{trackInfo.waveSoundClipInfo[clipIndex]};
            ClipParam& clipParam{trackParam.clipParam[clipIndex]};

            if (!clipParam.isPlayed) {
                if (waveSoundClipInfo.position * 1000 < m_CurrentTime)
                    StartClip(&clipParam, &waveSoundClipInfo);

                isAllClipPlayed = false;
            }

            if (!clipParam.isPlayed)
                continue;

            if (clipParam.pChannel != nullptr) {
                if (clipParam.pChannel->IsActive()) {
                    UpdateClip(&clipParam, &waveSoundClipInfo);
                    isAllClipPlayed = false;
                }
                // clipParam.pChannel = nullptr;
            }
        }

        isAllTrackPlayed = isAllTrackPlayed && isAllClipPlayed;
    }

    m_CurrentTime += SoundFrameIntervalMicroSeconds;
    return isAllTrackPlayed;
}

void AdvancedWaveSoundPlayer::InitializeTrackParams() {
    m_TrackParamSet.isPlayed = false;
    int trackCount{m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackCount};

    for (int trackIndex{0}; trackIndex < trackCount; ++trackIndex) {
        TrackParam& trackParam{m_TrackParamSet.trackParam[trackIndex]};
        trackParam.isPlayed = false;

        int clipCount{
            m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackInfo[trackIndex].waveSoundClipCount,
        };

        for (int clipIndex{0}; clipIndex < clipCount; ++clipIndex) {
            ClipParam& clipParam{trackParam.clipParam[clipIndex]};
            clipParam.isPlayed = false;
            clipParam.pChannel = nullptr;
        }
    }
}

bool AdvancedWaveSoundPlayer::StartClip(ClipParam* pClipParam,
                                        AdvancedWaveSoundClipInfo* pWaveSoundClipInfo) {
    const int priority{128};

    WaveArchiveFileReader warcReader{m_pWarcFile, false};
    const void* pWaveFile{warcReader.GetWaveFile(pWaveSoundClipInfo->waveIndex)};
    if (pWaveFile == nullptr)
        return false;

    WaveInfo waveInfo;
    {
        WaveFileReader waveReader{pWaveFile, WaveType_Nwwav};
        if (!waveReader.ReadWaveInfo(&waveInfo, nullptr))
            return false;
    }

    Channel* pChannel{Channel::AllocChannel(waveInfo.channelCount > 2 ? 2 : waveInfo.channelCount,
                                            priority, nullptr, this)};
    if (pChannel == nullptr)
        return false;

    pChannel->SetUpdateType(m_UpdateType);
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    pChannel->SetSubMixIndex(m_SubMixIndex);
#else
    pChannel->SetOutputReceiver(GetOutputReceiver());
#endif

    uint32_t startOffsetSamples{0};
    startOffsetSamples +=
        (static_cast<position_t>(pWaveSoundClipInfo->startOffset) * waveInfo.sampleRate) / 1000;

    pChannel->Start(waveInfo, pWaveSoundClipInfo->duration, startOffsetSamples, false);
    pClipParam->pChannel = pChannel;
    pClipParam->isPlayed = true;

    return true;
}

void AdvancedWaveSoundPlayer::UpdateClip(ClipParam* pClipParam,
                                         AdvancedWaveSoundClipInfo* pWaveSoundClipInfo) {
    Channel* pChannel{pClipParam->pChannel};
    if (pChannel == nullptr)
        return;

    int remainingTimeMilliSeconds{
        static_cast<int>(pChannel->GetLength() - (SoundFrameIntervalMicroSeconds / 1000))};
    if (remainingTimeMilliSeconds <= 0) {
        StopClip(pClipParam);
        return;
    }

    pChannel->SetLength(remainingTimeMilliSeconds);

    float volume{1.0f};
    volume *= GetVolume();
    volume *= pWaveSoundClipInfo->volume / 255.0f;
    pChannel->SetUserVolume(volume);

    float pitchRatio{1.0f};
    pitchRatio *= GetPitch();
    pitchRatio *= pWaveSoundClipInfo->pitch;
    pChannel->SetUserPitchRatio(pitchRatio);

    float panBase{0.0f};
    panBase = (pWaveSoundClipInfo->pan - 64) / 64.0f;

    OutputParam tvParam{GetTvParam()};
    tvParam.pan += panBase;
    pChannel->SetTvParam(tvParam);

    pChannel->SetPanMode(GetPanMode());
    pChannel->SetPanCurve(GetPanCurve());
}

void AdvancedWaveSoundPlayer::ReleaseClip(ClipParam* pClipParam) {
    Channel* pChannel{pClipParam->pChannel};
    if (pChannel != nullptr) {
        if (pChannel->IsActive())
            pChannel->Release();

        pClipParam->pChannel = nullptr;
    }
}

void AdvancedWaveSoundPlayer::StopClip(ClipParam* pClipParam) {
    Channel* pChannel{pClipParam->pChannel};
    if (pChannel != nullptr) {
        if (pChannel->IsActive())
            pChannel->Stop();

        pClipParam->pChannel = nullptr;
    }
}

void AdvancedWaveSoundPlayer::OnUpdateFrameSoundThread() {
    Update();
}

void AdvancedWaveSoundPlayer::OnUpdateFrameSoundThreadWithAudioFrameFrequency() {
    if (m_UpdateType == UpdateType_AudioFrame)
        Update();
}

void AdvancedWaveSoundPlayer::OnShutdownSoundThread() {
    Stop();
}

}  // namespace nn::atk::detail::driver
