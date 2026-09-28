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
    m_LoopCounter = 0;
#endif

    m_PlayingBlockLoopCounter = 0;
    m_PrefetchOffset = 0;
    m_IsPrefetchRevisionCheckEnabled = false;
    m_PrefetchRevisionValue = 0;
    m_DelayCount = 0;
    m_UseDelayCount = false;
    m_LoadFinishFlag = false;
    m_PauseStatus = false;
    m_LoadWaitFlag = false;
    m_IsInitialized = false;
    m_IsPrepared = false;
    m_IsFinalizing = false;
    m_IsPreparedPrefetch = false;
    m_OriginalPlaySamplePosition = 0;
    m_PlaySamplePosition = 0;

    if (TryAllocLoader())
        m_pLoader->Initialize();

    m_ItemData.pitch = 1.0f;
    m_ItemData.mainSend = 1.0f;
    for (int i{0}; i < AuxBus_Count; ++i)
        m_ItemData.fxSend[i] = 0.0f;

    for (int trackIndex{0}; trackIndex < static_cast<int>(StreamTrackCount); ++trackIndex) {
        StreamTrack& track{m_Tracks[trackIndex]};

        track.m_ActiveFlag = false;
        track.m_Volume = 1.0f;
        track.m_OutputLine = -1;
        track.m_TvParam.Initialize();
    }

    for (int channelIndex{0}; channelIndex < StreamChannelCount; ++channelIndex) {
        StreamChannel& channel{m_Channels[channelIndex]};

        channel.m_pBufferAddress = nullptr;
        channel.m_pVoice = nullptr;
    }
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

void StreamSoundPlayer::Finalize() {
    FinishPlayer();

    if (!m_IsInitialized)
        return;

    m_IsFinalizing = true;
    FreeStreamBuffers();
    FreeVoices();
    FreeLoader();

    m_pBufferPool = nullptr;
    BasicSoundPlayer::Finalize();
    SetActiveFlag(false);
    m_IsInitialized = false;
}

void StreamSoundPlayer::FinishPlayer() {
    if (m_pLoader != nullptr)
        m_pLoader->CancelRequest();

    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        MultiVoice* voice{m_Channels[ch].m_pVoice};
        if (voice != nullptr)
            voice->Stop();
    }

    if (m_IsRegisterPlayerCallback) {
        SoundThread::GetInstance().UnregisterPlayerCallback(this);
        m_IsRegisterPlayerCallback = false;
    }

    if (IsStarted())
        SetStartedFlag(false);
}

void StreamSoundPlayer::FreeStreamBuffers() {
    for (int index{0}; index < m_ChannelCount; ++index) {
        if (m_Channels[index].m_pBufferAddress != nullptr) {
            m_pBufferPool->Free(m_Channels[index].m_pBufferAddress);
            m_Channels[index].m_pBufferAddress = nullptr;
        }
    }
}

void StreamSoundPlayer::FreeVoices() {
    for (int ch{0}; ch < m_ChannelCount; ++ch) {
        StreamChannel& channel{m_Channels[ch]};

        if (channel.m_pVoice != nullptr) {
            channel.m_pVoice->Free();
            channel.m_pVoice = nullptr;
        }
    }
}

void StreamSoundPlayer::FreeLoader() {
    if (m_pLoader == nullptr || m_pLoaderManager == nullptr)
        return;

    m_pLoaderManager->Free(m_pLoader);
    m_pLoader = nullptr;
}

void StreamSoundPlayer::Setup(const SetupArg& arg) {
    if (m_pLoader == nullptr) {
        m_SetupArg = arg;
        return;
    }

    m_FileType = arg.fileType;
    m_DecodeMode = arg.decodeMode;
    m_LoopFlag = arg.loopFlag;
    m_LoopStart = arg.loopStart;
    m_LoopEnd = arg.loopEnd;
    m_AssignNumber = g_AssignNumberCount++;
    m_pLoader->SetAssignNumber(g_AssignNumberCount);
    m_ItemData.Set(arg);
}

void StreamSoundPlayer::ItemData::Set(const SetupArg& arg) {
    pitch = arg.pitch;
    mainSend = arg.mainSend / 127.0f - 1.0f;

    for (int i{0}; i < AuxBus_Count; ++i)
        fxSend[i] = arg.fxSend[i] / 127.0f;
}

}  // namespace nn::atk::detail::driver
