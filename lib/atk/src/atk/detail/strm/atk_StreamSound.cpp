#include <nn/atk/atk_StreamSound.h>

#include <nn/atk/atk_DriverCommand.h>

namespace nn::atk::detail {

StreamSound::StreamSound(StreamSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool StreamSound::Initialize()
#else
bool StreamSound::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool result{BasicSound::Initialize()};
#else
    bool result{BasicSound::Initialize(pOutputReceiver)};
#endif

    if (!result)
        return false;

    m_pTempSpecialHandle = nullptr;

    for (int i{0}; i < static_cast<int>(StreamTrackCount); ++i) {
        m_TrackVolume[i].InitValue(0);
        m_TrackVolume[i].SetTarget(1.0f, 1);
    }

    for (int i{0}; i < static_cast<int>(WaveChannelMax); ++i)
        m_AvailableTrackBitFlag[i] = 0;

    m_InitializeFlag = true;
    return true;
}

void StreamSound::Finalize() {
    if (m_InitializeFlag) {
        m_InitializeFlag = false;
        BasicSound::Finalize();
        m_Manager.Free(this);
    }
}

void StreamSound::Setup(const driver::StreamSoundPlayer::SetupArg& arg) {
    m_AllocTrackFlag = arg.allocTrackFlag;

    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundSetup>()};
        command->id = DriverCommandId_StrmSetup;
        command->player = &m_PlayerInstance;
        command->arg = arg;

        cmdmgr.PushCommand(command);
    }

    for (uint32_t trackNo{0}; trackNo < StreamTrackCount; ++trackNo) {
        const uint8_t trackChannelCount{arg.trackInfos.track[trackNo].channelCount};
        for (uint8_t channelNo{0}; channelNo < trackChannelCount; ++channelNo)
            m_AvailableTrackBitFlag[channelNo] += 1 << trackNo;
    }
}

void StreamSound::Prepare(const driver::StreamSoundPlayer::PrepareBaseArg& arg) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundPrepare>()};
        command->id = DriverCommandId_StrmPrepare;
        command->player = &m_PlayerInstance;
        command->arg.baseArg = arg;
        command->arg.cacheBuffer = m_pCacheBuffer;
        command->arg.cacheSize = m_CacheSize;

        cmdmgr.PushCommand(command);
    }
}

}  // namespace nn::atk::detail
