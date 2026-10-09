#include <nn/atk/atk_SequenceSound.h>

#include <nn/atk/atk_DriverCommand.h>

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

}  // namespace nn::atk::detail
