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

void StreamSound::PreparePrefetch(const void* strmPrefetchFile,
                                  const driver::StreamSoundPlayer::PrepareBaseArg& arg) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundPreparePrefetch>()};
        command->id = DriverCommandId_StrmPreparePrefetch;
        command->player = &m_PlayerInstance;
        command->arg.strmPrefetchFile = strmPrefetchFile;
        command->arg.baseArg = arg;

        cmdmgr.PushCommand(command);
    }
}

void StreamSound::UpdateMoveValue() {
    BasicSound::UpdateMoveValue();

    uint16_t bitFlag{m_AllocTrackFlag};
    for (int trackNo{0}; trackNo < static_cast<int>(StreamTrackCount); ++trackNo, bitFlag >>= 1) {
        if (bitFlag & 1)
            m_TrackVolume[trackNo].Update();
    }
}

void StreamSound::OnUpdateParam() {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        uint16_t bitFlag{m_AllocTrackFlag};
        for (int trackNo{0}; trackNo < static_cast<int>(StreamTrackCount);
             ++trackNo, bitFlag >>= 1) {
            if (bitFlag & 1) {
                auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundTrackParam>()};
                command->id = DriverCommandId_StrmTrackVolume;
                command->player = &m_PlayerInstance;
                command->trackBitFlag = 1 << trackNo;
                command->value = m_TrackVolume[trackNo].GetValue();

                cmdmgr.PushCommand(command);
            }
        }
    }
}

void StreamSound::SetTrackVolume(uint32_t trackBitFlag, float volume, int32_t frames) {
    if (trackBitFlag == 0)
        return;

    volume = volume < 0.0f ? 0.0f : volume;

    uint16_t bitFlag{static_cast<uint16_t>(m_AllocTrackFlag & trackBitFlag)};
    for (int trackNo{0}; trackNo < static_cast<int>(StreamTrackCount); ++trackNo, bitFlag >>= 1) {
        if (bitFlag & 1)
            m_TrackVolume[trackNo].SetTarget(volume, frames);
    }
}

void StreamSound::SetTrackInitialVolume(uint32_t trackBitFlag, uint32_t volume) {
    uint16_t bitFlag{static_cast<uint16_t>(trackBitFlag & m_AllocTrackFlag)};

    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundTrackInitialVolume>()};
    command->id = DriverCommandId_StrmTrackInitialVolume;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = bitFlag;
    command->value = volume;

    cmdmgr.PushCommand(command);
}

void StreamSound::SetTrackOutputLine(uint32_t trackBitFlag, uint32_t lineFlag) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandStreamSoundTrackParam>()};
    command->id = DriverCommandId_StrmTrackOutputline;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->uint32Value = lineFlag;

    cmdmgr.PushCommand(command);
}

}  // namespace nn::atk::detail
