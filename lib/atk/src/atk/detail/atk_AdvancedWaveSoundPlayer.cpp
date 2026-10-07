#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>
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
            m_AdvancedWaveSoundTrackInfoSet.waveSoundTrackInfo[trackIndex]};
        TrackParam& trackParam{m_TrackParamSet.trackParam[trackIndex]};

        for (int clipIndex{0}; clipIndex < trackInfo.waveSoundClipCount; ++clipIndex) {
            ClipParam& clipParam{trackParam.clipParam[clipIndex]};
            ReleaseClip(&clipParam);
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
