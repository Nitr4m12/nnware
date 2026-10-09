#include <nn/atk/atk_SequenceSound.h>

#include <nn/atk/atk_DriverCommand.h>
#include "nn/atk/atk_Global.h"

namespace nn::atk::detail {

SequenceSound::SequenceSound(SequenceSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool SequenceSound::Initialize()
#else
bool SequenceSound::Initialize(OutputReceiver* pOutputReceiver)
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
    m_IsCalledPrepare = false;
    m_InitializeFlag = true;
    return true;
}

void SequenceSound::Finalize() {
    if (m_InitializeFlag) {
        m_InitializeFlag = false;
        m_IsCalledPrepare = false;
        BasicSound::Finalize();
        m_Manager.Free(this);
    }
}

void SequenceSound::Setup(driver::SequenceTrackAllocator* trackAllocator, uint32_t allocTracks,
                          driver::NoteOnCallback* noteOnCallback, int32_t channelPriority,
                          bool isReleasePriorityFix, SequenceUserProcCallback userproc,
                          void* userprocArg) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundSetup>()};
        command->id = DriverCommandId_SeqSetup;
        command->player = &m_PlayerInstance;

        driver::SequenceSoundPlayer::SetupArg arg;
        arg.trackAllocator = trackAllocator;
        arg.allocTracks = allocTracks;
        arg.callback = noteOnCallback;

        command->arg = arg;
        command->channelPriority = channelPriority;
        command->isReleasePriorityFix = isReleasePriorityFix;
        command->userproc = reinterpret_cast<uintptr_t>(userproc);
        command->userprocArg = userprocArg;

        cmdmgr.PushCommand(command);
    }
}

void SequenceSound::Prepare(const Resource& res,
                            const driver::SequenceSoundPlayer::StartInfo& startInfo) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundPrepare>()};
        command->id = DriverCommandId_SeqPrepare;
        command->player = &m_PlayerInstance;

        driver::SequenceSoundPlayer::PrepareArg arg;
        arg.seqFile = res.seq;

        for (int i{0}; i < static_cast<int>(SeqBankMax); ++i) {
            arg.bankFiles[i] = res.banks[i];
            arg.warcFiles[i] = res.warcs[i];
            arg.warcIsIndividuals[i] = res.warcIsIndividuals[i];
        }

        arg.seqOffset = startInfo.seqOffset;
        arg.delayTime = startInfo.delayTime;
        arg.delayCount = startInfo.delayCount;
        arg.updateType = startInfo.updateType;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        arg.subMixIndex = startInfo.subMixIndex;
#endif

        command->arg = arg;

        cmdmgr.PushCommand(command);
    }

    if (startInfo.startOffset > 0)
        Skip(startInfo.startOffsetType, startInfo.startOffset);

    m_IsCalledPrepare = true;
}

void SequenceSound::Skip(driver::SequenceSoundPlayer::StartOffsetType offsetType, int32_t offset) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundSkip>()};
        command->id = DriverCommandId_SeqSkip;
        command->player = &m_PlayerInstance;
        command->offsetType = offsetType;
        command->offset = offset;

        cmdmgr.PushCommand(command);
    }
}

void SequenceSound::SetTempoRatio(float tempoRatio) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTempoRatio>()};
    command->id = DriverCommandId_SeqTempoRatio;
    command->player = &m_PlayerInstance;
    command->tempoRatio = tempoRatio;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetChannelPriority(int32_t priority) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundChannelPrio>()};
    command->id = DriverCommandId_SeqChannelPrio;
    command->player = &m_PlayerInstance;
    command->priority = static_cast<uint8_t>(priority);

    cmdmgr.PushCommand(command);
}

void SequenceSound::OnUpdatePlayerPriority() {
    int priority{CalcCurrentPlayerPriority()};
    m_Manager.UpdatePriority(this, priority);
}

void SequenceSound::SetTrackMute(uint32_t trackBitFlag, SequenceMute mute) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackMute>()};
    command->id = DriverCommandId_SeqTrackMute;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->mute = mute;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackMute(uint32_t trackBitFlag, bool muteFlag) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackMute>()};
    command->id = DriverCommandId_SeqTrackMute;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->mute = muteFlag ? SequenceMute_Stop : SequenceMute_Off;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackSilence(uint32_t trackBitFlag, bool silenceFlag, int32_t fadeFrames) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackSilence>()};
    command->id = DriverCommandId_SeqTrackSilence;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->silenceFlag = silenceFlag;
    command->fadeFrames = fadeFrames;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackVolume(uint32_t trackBitFlag, float volume) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackParam>()};
    command->id = DriverCommandId_SeqTrackVolume;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->value = volume;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackPitch(uint32_t trackBitFlag, float pitch) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackParam>()};
    command->id = DriverCommandId_SeqTrackPitch;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->value = pitch;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackMainOutVolume(uint32_t trackBitFlag, float volume) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackParam>()};
    command->id = DriverCommandId_SeqTrackTvVolume;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->value = volume;

    cmdmgr.PushCommand(command);
}

void SequenceSound::SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                                const MixParameter& mixParam) {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandSequenceSoundTrackMixParameter>()};
    command->id = DriverCommandId_SeqTrackTvMixParameter;
    command->player = &m_PlayerInstance;
    command->trackBitFlag = trackBitFlag;
    command->srcChNo = srcChNo;

    for (int channel{0}; channel < ChannelIndex_Count; ++channel)
        command->param.ch[channel] = mixParam.ch[channel];

    cmdmgr.PushCommand(command);
}

}  // namespace nn::atk::detail
