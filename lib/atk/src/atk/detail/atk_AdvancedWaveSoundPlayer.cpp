#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>
#include "nn/atk/atk_BasicSoundPlayer.h"
#include "nn/atk/atk_SoundThread.h"

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

void AdvancedWaveSoundPlayer::ReleaseClip(ClipParam* pClipParam) {
    Channel* pChannel{pClipParam->pChannel};
    if (pChannel != nullptr) {
        if (pChannel->IsActive())
            pChannel->Release();

        pClipParam->pChannel = nullptr;
    }
}

}  // namespace nn::atk::detail::driver
