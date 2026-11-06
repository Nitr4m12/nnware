#include <nn/atk/atk_BasicSound.h>

#include <nn/atk/atk_DriverCommand.h>
#include <nn/atk/atk_ExternalSoundPlayer.h>
#include <nn/atk/atk_SoundActor.h>
#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_SoundPlayer.h>

namespace nn::atk::detail {

namespace {

void SetSoundPlayerCalculationValues(SoundParamCalculationValues::SoundPlayerParam& param,
                                     const SoundPlayer& player) {
    param.volume = player.GetVolume();
    param.lpf = player.GetLowPassFilterFrequency();
    param.bqfType = player.GetBiquadFilterType();
    param.bqfValue = player.GetBiquadFilterValue();

    for (int i{0}; i < OutputDevice_Count; ++i) {
        const OutputDevice device{static_cast<OutputDevice>(i)};

        param.outputVolume[device] = player.GetOutputVolume(device);
        param.outputMainSend[device] = player.GetOutputMainSend(device);

        for (int k{0}; k < AuxBus_Count; ++k) {
            const AuxBus bus{static_cast<AuxBus>(k)};
            param.outputEffectSend[device][bus] = player.GetOutputFxSend(device, bus);
        }
    }
}

void SetSound3DCalculationValues(SoundParamCalculationValues::Sound3DParam& param,
                                 const SoundParam& ambientParam) {
    param.volume = ambientParam.GetVolume();
    param.pitch = ambientParam.GetPitch();
    param.lpf = ambientParam.GetLpf();
    param.bqfType = ambientParam.GetBiquadFilterType();
    param.bqfValue = ambientParam.GetBiquadFilterValue();
    param.outputLineFlag = ambientParam.GetOutputLineFlag();
    param.playerPriority = ambientParam.GetPriority();

    for (int i{0}; i < OutputDevice_Count; ++i) {
        const OutputAmbientParam* pAmbientParam{&ambientParam.GetTvParam()};
        const OutputDevice device{static_cast<OutputDevice>(i)};

        param.outputVolume[device] = pAmbientParam->GetVolume();
        param.outputPan[device] = pAmbientParam->GetPan();
        param.outputSurroundPan[device] = pAmbientParam->GetSurroundPan();

        for (int k{0}; k < AuxBus_Count; ++k) {
            const AuxBus bus{static_cast<AuxBus>(k)};
            param.outputEffectSend[device][bus] = pAmbientParam->GetEffectSend(bus);
        }
    }
}

void SetSoundActorCalculationValues(SoundParamCalculationValues::SoundActorParam& param,
                                    const SoundActorParam& actorParam) {
    param.volume = actorParam.volume;
    param.pitch = actorParam.pitch;
    param.lpf = actorParam.lpf;

    for (int i{0}; i < OutputDevice_Count; ++i) {
        const OutputDevice device{static_cast<OutputDevice>(i)};

        param.outputVolume[device] = actorParam.tvVolume;
        param.outputPan[device] = actorParam.tvPan;
    }
}

}  // anonymous namespace

BasicSound::BasicSound() = default;

// NON_MATCHING: bad order of instructions
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool BasicSound::Initialize()
#else
bool BasicSound::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
    m_InstanceId = g_LastInstanceId++;

    m_pPlayerHeap = nullptr;

    m_pExtSoundPlayer = nullptr;
    m_pGeneralHandle = nullptr;
    m_pTempGeneralHandle = nullptr;
    m_pSoundPlayer = nullptr;
    m_pSoundActor = nullptr;
    m_pSoundArchive = nullptr;

    m_AutoStopCounter = 0;
    m_UpdateCounter = 0;
    m_Id = 0xffffffff;
    m_PlayingCounter = 0;

    m_PlayerState = PlayerState_Init;
    m_PauseState = PauseState_Normal;
    m_MuteState = MuteState_Normal;
    m_PauseMode = PauseMode_Default;

    m_AmbientInfo.paramUpdateCallback = nullptr;
    m_AmbientInfo.argUpdateCallback = nullptr;
    m_AmbientInfo.argAllocatorCallback = nullptr;
    m_AmbientInfo.arg = nullptr;
    m_AmbientInfo.argSize = 0;

    m_StartFlag = false;
    m_StartedFlag = false;
    m_AutoStopFlag = false;
    m_FadeOutFlag = false;
    m_PlayerAvailableFlag = false;
    m_UnPauseFlag = false;

    m_FadeVolume.InitValue(1.0f);
    m_PauseFadeVolume.InitValue(1.0f);
    m_MuteFadeVolume.InitValue(1.0f);

    m_InitVolume = 1.0f;
    m_Pitch = 1.0f;
    m_BiquadFilterType = BiquadFilterType_Inherit;
    m_LpfFreq = 0.0f;
    m_BiquadFilterValue = 0.0f;
    m_OutputLineFlag = OutputLine_Main;
    // m_Priority = 0;

    m_CommonParam.Initialize();

    for (int i{0}; i < OutputDevice_Count; ++i) {
        m_OutputParam[i].Initialize();
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        if (m_pOutputAdditionalParam[i] != nullptr)
            m_pOutputAdditionalParam[i]->Reset();
#endif
    }

    m_AmbientParam.Initialize();
    m_ActorParam.Reset();

    if (m_UserParamSize != 0)
        std::memset(m_pUserParam, 0, m_UserParamSize);

    {
        driver::BasicSoundPlayer* basicPlayer{GetBasicSoundPlayerHandle()};

        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandPlayerInit>()};
        command->id = DriverCommandId_PlayerInit;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        command->pOutputReceiver = pOutputReceiver;
#endif
        command->availableFlagPtr = &m_PlayerAvailableFlag;
        command->player = basicPlayer;

        cmdmgr.PushCommand(command);

        basicPlayer->InitializeEvent();
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    pOutputReceiver->AddReferenceCount(1);
    m_pOutputReceiver = pOutputReceiver;
#endif
    m_State = State_Initialized;
    return true;
}

void BasicSound::SetPriority(int priority, int ambientPriority) {
    m_Priority = priority;
    m_AmbientParam.SetPriority(ambientPriority);
}

void BasicSound::GetPriority(int* priority, int* ambientPriority) const {
    if (priority != nullptr)
        *priority = m_Priority;

    if (ambientPriority != nullptr)
        *ambientPriority = m_AmbientParam.GetPriority();
}

void BasicSound::ClearIsFinalizedForCannotAllocatedResourceFlag() {
    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandPlayerClearResourceFlag>()};
#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    if (command == nullptr)
        return;
#endif

    command->id = DriverCommandId_PlayerClearResourceFlag;
    command->player = GetBasicSoundPlayerHandle();
    cmdmgr.PushCommand(command);
}

void BasicSound::Finalize() {
    if (m_State != State_Initialized)
        return;

    SetId(0xffffffff);

    if (IsAttachedGeneralHandle())
        DetachGeneralHandle();

    if (IsAttachedTempGeneralHandle())
        DetachTempGeneralHandle();

    if (IsAttachedTempSpecialHandle())
        DetachTempSpecialHandle();

    if (m_pSoundPlayer != nullptr)
        m_pSoundPlayer->detail_RemoveSound(this);

    if (m_pExtSoundPlayer != nullptr)
        m_pExtSoundPlayer->RemoveSound(this);

    if (m_AmbientInfo.argAllocatorCallback != nullptr) {
        m_AmbientInfo.argAllocatorCallback->RemoveSoundImpl(m_AmbientInfo.arg, this);
        m_AmbientInfo.arg = nullptr;
    }

    {
        if (m_StartedFlag) {
            DriverCommand& cmdmgr{DriverCommand::GetInstance()};

            auto* command{cmdmgr.AllocCommand<DriverCommandPlayer>()};
            command->id = DriverCommandId_PlayerStop;
            command->player = GetBasicSoundPlayerHandle();
            command->flag = m_FadeOutFlag;

            cmdmgr.PushCommand(command);

            m_StartedFlag = false;
        }

        m_PlayerAvailableFlag = false;
        m_PlayerState = PlayerState_Stop;

        DriverCommand& cmdmgr{DriverCommand::GetInstance()};

        auto* command{cmdmgr.AllocCommand<DriverCommandPlayer>()};
        command->id = DriverCommandId_PlayerFinalize;
        command->player = GetBasicSoundPlayerHandle();

        cmdmgr.PushCommand(command);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_pOutputReceiver = nullptr;
#endif

    if (m_SoundStopCallback != nullptr) {
        m_SoundStopCallback();
        m_SoundStopCallback = nullptr;
    }
    m_FadeOutFlag = false;
    m_State = State_Finalized;
}

void BasicSound::SetId(uint32_t id) {
    m_Id = id;
}

bool BasicSound::IsAttachedGeneralHandle() {
    return m_pGeneralHandle != nullptr;
}

void BasicSound::DetachGeneralHandle() {
    m_pGeneralHandle->DetachSound();
}

bool BasicSound::IsAttachedTempGeneralHandle() {
    return m_pTempGeneralHandle != nullptr;
}

void BasicSound::DetachTempGeneralHandle() {
    m_pTempGeneralHandle->DetachSound();
}

void BasicSound::StartPrepared() {
    m_StartFlag = true;
}

void BasicSound::Stop(int fadeFrames) {
    if ((fadeFrames <= 0 || m_PauseState == PauseState_Paused) ||
        (!m_StartFlag && !m_StartedFlag)) {
        Finalize();
        return;
    }

    int frames{static_cast<int>(fadeFrames * m_FadeVolume.GetValue())};
    m_FadeVolume.SetTarget(0.0f, frames);
    SetPlayerPriority(0);

    m_AutoStopFlag = false;
    m_PauseState = PauseState_Normal;
    m_UnPauseFlag = false;
    m_PauseMode = PauseMode_Default;
    m_FadeOutFlag = true;
    m_MuteState = MuteState_Normal;
}

void BasicSound::Update() {
    driver::BasicSoundPlayer* player{GetBasicSoundPlayerHandle()};

    if (m_PlayerAvailableFlag && GetBasicSoundPlayerHandle()->IsPlayFinished()) {
        Finalize();
        return;
    }

    if (player->IsFinalizedForCannotAllocateResource()) {
        ClearIsFinalizedForCannotAllocatedResourceFlag();
        Finalize();
        return;
    }

    if (!IsPrepared())
        return;

    if (m_AutoStopFlag) {
        if (m_AutoStopCounter != 0) {
            --m_AutoStopCounter;
        } else {
            switch (m_PauseState) {
            case PauseState_Normal:
            case PauseState_Unpausing:
                Finalize();
                return;
            default:
                break;
            }
        }
    }

    bool playerStartFlag{false};
    if (!m_StartedFlag) {
        if (!m_StartFlag)
            return;

        if (!IsPrepared())
            return;

        playerStartFlag = true;
        m_PlayerState = PlayerState_Play;
    }

    if (m_PlayerState == PlayerState_Play) {
        if (m_UpdateCounter != 0xffffffff)
            ++m_UpdateCounter;
    }

    if (m_PlayerState == PlayerState_Play && m_PauseState != PauseState_Paused) {
        if (m_PlayingCounter != 0xffffffff)
            ++m_PlayingCounter;
    }

    bool pauseFadeIsFinished{m_PauseFadeVolume.IsFinished()};
    switch (m_PauseState) {
    case PauseState_Pausing:
        m_PauseFadeVolume.Update();
        break;
    case PauseState_Unpausing:
        m_PauseFadeVolume.Update();
        UpdateMoveValue();
        break;
    case PauseState_Normal:
        UpdateMoveValue();
        break;
    default:
        break;
    }

    if (m_AmbientInfo.argUpdateCallback != nullptr)
        m_AmbientInfo.argUpdateCallback->UpdateSoundImpl(m_AmbientInfo.arg, this);

    if (m_AmbientInfo.paramUpdateCallback != nullptr) {
        SoundAmbientParam ambientParam;
        if (m_UpdateCounter != 0) {
            ambientParam.SetVolume(m_AmbientParam.GetVolume());
            ambientParam.SetPitch(m_AmbientParam.GetPitch());
            ambientParam.SetLowPassFilter(m_AmbientParam.GetLpf());
            ambientParam.SetBiquadFilterValue(m_AmbientParam.GetBiquadFilterValue());
            ambientParam.SetBiquadFilterType(m_AmbientParam.GetBiquadFilterType());
            ambientParam.SetUserData(m_AmbientParam.GetUserData());
            ambientParam.SetPriority(m_AmbientParam.GetPriority());

            ambientParam.SetOutputLineFlag(m_AmbientParam.GetOutputLineFlag());
            ambientParam.SetTvParam(m_AmbientParam.GetTvParam());
        } else {
            ambientParam.SetUserData(0);
        }

        m_AmbientInfo.paramUpdateCallback->UpdateAmbientParamImpl(m_AmbientInfo.arg, m_Id,
                                                                  &ambientParam);

        m_AmbientParam.SetVolume(ambientParam.GetVolume());
        m_AmbientParam.SetPitch(ambientParam.GetPitch());
        m_AmbientParam.SetLpf(ambientParam.GetLowPassFilter());
        m_AmbientParam.SetBiquadFilterValue(ambientParam.GetBiquadFilterValue());
        m_AmbientParam.SetBiquadFilterType(ambientParam.GetBiquadFilterType());
        m_AmbientParam.SetUserData(ambientParam.GetUserData());
        m_AmbientParam.SetPriority(ambientParam.GetPriority());

        m_AmbientParam.SetOutputLineFlag(ambientParam.GetOutputLineFlag());
        m_AmbientParam.SetTvParam(ambientParam.GetTvParam());
    }

    if (m_pSoundActor != nullptr)
        m_ActorParam = m_pSoundActor->GetActorParam();

    UpdateParam();

    if (m_FadeOutFlag && m_FadeVolume.IsFinished()) {
        m_FadeOutFlag = false;
        Finalize();
        return;
    }

    DriverCommand& cmdmgr{DriverCommand::GetInstance()};
    if (playerStartFlag) {
        auto* command{cmdmgr.AllocCommand<DriverCommandPlayer>()};
        command->id = DriverCommandId_PlayerStart;
        command->player = GetBasicSoundPlayerHandle();

        cmdmgr.PushCommand(command);
        m_StartFlag = false;
        m_StartedFlag = true;
    }

    if (m_PauseMode == PauseMode_PauseImmediately)
        pauseFadeIsFinished = m_PauseFadeVolume.IsFinished();

    switch (m_PauseState) {
    case PauseState_Pausing: {
        if (!pauseFadeIsFinished)
            break;

        auto* command{cmdmgr.AllocCommand<DriverCommandPlayer>()};
        command->id = DriverCommandId_PlayerPause;
        command->player = GetBasicSoundPlayerHandle();
        command->flag = true;
        cmdmgr.PushCommand(command);
        m_PauseState = PauseState_Paused;
        break;
    }
    case PauseState_Unpausing:
        if (!m_PauseFadeVolume.IsFinished())
            break;

        m_PauseState = PauseState_Normal;
        break;
    default:
        break;
    }

    if (m_UnPauseFlag) {
        auto* command{cmdmgr.AllocCommand<DriverCommandPlayer>()};
        command->id = DriverCommandId_PlayerPause;
        command->player = GetBasicSoundPlayerHandle();
        command->flag = false;
        cmdmgr.PushCommand(command);
        m_UnPauseFlag = false;
    }

    switch (m_MuteState) {
    case MuteState_Muting:
        if (m_MuteFadeVolume.IsFinished())
            m_MuteState = MuteState_Muted;
        break;
    case MuteState_Unmuting:
        if (m_MuteFadeVolume.IsFinished())
            m_MuteState = MuteState_Normal;
        break;
    default:
        break;
    }
}

void BasicSound::UpdateParam() {
    OnUpdateParam();

    const float volume{CalculateVolume()};
    const float pitch{CalculatePitch()};
    const float lpfFreq{CalculateLpfFrequency()};
    const uint32_t outputLineFlag{CalculateOutLineFlag()};

    int biquadFilterType;
    float biquadFilterValue;
    CalculateBiquadFilter(&biquadFilterType, &biquadFilterValue);

    OutputParam main;
    CalculateOutputParam(&main, OutputDevice_Main);

    DriverCommand& cmdmgr{DriverCommand::GetInstance()};

    auto* command{cmdmgr.AllocCommand<DriverCommandPlayerParam>()};
    command->id = DriverCommandId_PlayerParam;
    command->player = GetBasicSoundPlayerHandle();
    command->volume = volume;
    command->pitch = pitch;
    command->lpfFreq = lpfFreq;
    command->biquadFilterType = biquadFilterType;
    command->biquadFilterValue = biquadFilterValue;
    command->outputLineFlag = outputLineFlag;
    command->tvParam = main;

    cmdmgr.PushCommand(command);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_pOutputAdditionalParam[OutputDevice_Main] == nullptr)
        return;

    if (m_pOutputAdditionalParam[OutputDevice_Main]->IsAdditionalSendEnabled()) {
        SendArray* pSend{m_pOutputAdditionalParam[OutputDevice_Main]->GetAdditionalSendAddr()};
        SoundPlayer* pSoundPlayer{m_pSoundPlayer};

        for (int i{0}; i < pSend->GetCount(); ++i) {
            const int Bus{i + 4};
            const float PlayerSend{pSoundPlayer != nullptr ? pSoundPlayer->GetSend(Bus) : 0.0f};

            auto* sendCommand{cmdmgr.AllocCommand<DriverCommandPlayerAdditionalSend>()};
            sendCommand->id = DriverCommandId_PlayerAdditionalSend;
            sendCommand->player = GetBasicSoundPlayerHandle();
            sendCommand->bus = Bus;
            sendCommand->send = PlayerSend + pSend->GetValue(i);

            cmdmgr.PushCommand(sendCommand);
        }
    }

    if (m_pOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeEnabled()) {
        const bool IsUsed{m_pOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeUsed()};

        auto* busMixUsedCommand{cmdmgr.AllocCommand<DriverCommandPlayerBusMixVolumeUsed>()};
        busMixUsedCommand->id = DriverCommandId_PlayerBusMixVolumeUsed;
        busMixUsedCommand->player = GetBasicSoundPlayerHandle();
        busMixUsedCommand->isUsed = IsUsed;

        cmdmgr.PushCommand(busMixUsedCommand);

        if (IsUsed) {
            OutputBusMixVolume busMixParam;
            CalculateOutputBusMixVolume(&busMixParam, OutputDevice_Main);
            auto* busMixCommand{cmdmgr.AllocCommand<DriverCommandPlayerBusMixVolume>()};
            busMixCommand->id = DriverCommandId_PlayerBusMixVolume;
            busMixCommand->player = GetBasicSoundPlayerHandle();
            busMixCommand->tvBusMixVolume = busMixParam;

            cmdmgr.PushCommand(busMixCommand);

            for (int i{0}; i < m_pOutputAdditionalParam[OutputDevice_Main]
                                   ->GetBusMixVolumePacketAddr()
                                   ->GetBusCount();
                 ++i) {
                auto* busMixEnabledCommand{
                    cmdmgr.AllocCommand<DriverCommandPlayerBusMixVolumeEnabled>(),
                };
                busMixEnabledCommand->id = DriverCommandId_PlayerBusMixVolumeEnabled;
                busMixEnabledCommand->player = GetBasicSoundPlayerHandle();
                busMixEnabledCommand->bus = i;
                busMixEnabledCommand->isEnabled =
                    m_pOutputAdditionalParam[OutputDevice_Main]->IsBusMixVolumeEnabledForBus(i);

                cmdmgr.PushCommand(busMixEnabledCommand);
            }
        }
    }

    if (m_pOutputAdditionalParam[OutputDevice_Main]->IsVolumeThroughModeEnabled()) {
        auto* volumeThroughModeEnabledCommand{
            cmdmgr.AllocCommand<DriverCommandPlayerVolumeThroughModeUsed>(),
        };
        volumeThroughModeEnabledCommand->id = DriverCommandId_PlayerVolumeThroughModeUsed;
        volumeThroughModeEnabledCommand->player = GetBasicSoundPlayerHandle();
        volumeThroughModeEnabledCommand->isVolumeThroughModeUsed = IsVolumeThroughModeUsed();

        cmdmgr.PushCommand(volumeThroughModeEnabledCommand);

        if (volumeThroughModeEnabledCommand->isVolumeThroughModeUsed) {
            const float binaryVolume{m_InitVolume};

            VolumeThroughModePacket* pPacket{
                m_pOutputAdditionalParam[OutputDevice_Main]->GetVolumeThroughModePacketAddr(),
            };
            auto* binaryVolumeCommand{cmdmgr.AllocCommand<DriverCommandPlayerBinaryVolume>()};
            binaryVolumeCommand->id = DriverCommandId_PlayerBinaryVolume;
            binaryVolumeCommand->player = GetBasicSoundPlayerHandle();
            binaryVolumeCommand->volume = binaryVolume;

            cmdmgr.PushCommand(binaryVolumeCommand);

            for (int i{0}; i < pPacket->GetBusCount(); ++i) {
                auto* volumeThroughModeCommand{
                    cmdmgr.AllocCommand<DriverCommandPlayerVolumeThroughMode>(),
                };
                volumeThroughModeCommand->id = DriverCommandId_PlayerVolumeThroughMode;
                volumeThroughModeCommand->player = GetBasicSoundPlayerHandle();
                volumeThroughModeCommand->bus = i;
                volumeThroughModeCommand->volumeThroughMode = pPacket->GetVolumeThroughMode(i);

                cmdmgr.PushCommand(volumeThroughModeCommand);
            }
        }
    }
#endif
}

void BasicSound::SetPlayerPriority(int priority) {
    m_Priority = priority;

    if (m_pSoundPlayer != nullptr)
        m_pSoundPlayer->detail_SortPriorityList(this);

    OnUpdatePlayerPriority();
}

void BasicSound::ForceStop() {
    m_FadeOutFlag = true;

    Finalize();
}

void BasicSound::Pause(bool flag, int fadeFrames) {
    Pause(flag, fadeFrames, PauseMode_Default);
}

void BasicSound::Pause(bool flag, int fadeFrames, PauseMode pauseMode) {
    int frames;

    if (flag) {
        switch (m_PauseState) {
        case PauseState_Normal:
        case PauseState_Unpausing:
        case PauseState_Pausing:
            frames = static_cast<int>(fadeFrames * m_PauseFadeVolume.GetValue());
            frames = frames > 0 ? frames : 1;
            m_PauseFadeVolume.SetTarget(0.0f, frames);
            m_PauseState = PauseState_Pausing;
            m_UnPauseFlag = false;
            break;
        case PauseState_Paused:
        default:
            return;
        }
    } else {
        switch (m_PauseState - 1) {
        case PauseState_Normal:
        case PauseState_Pausing:
        case PauseState_Paused:
            frames = static_cast<int>(fadeFrames * (1.0f - m_PauseFadeVolume.GetValue()));
            frames = frames > 0 ? frames : 1;
            m_PauseFadeVolume.SetTarget(1.0f, frames);
            m_PauseState = PauseState_Unpausing;
            m_UnPauseFlag = true;
            break;
        case PauseState_Unpausing:
        default:
            return;
        }
    }

    m_PauseMode = pauseMode;
}

void BasicSound::Mute(bool flag, int fadeFrames) {
    int frames;

    if (flag) {
        switch (m_MuteState) {
        case MuteState_Normal:
        case MuteState_Muting:
        case MuteState_Unmuting:
            frames = static_cast<int>(fadeFrames * m_MuteFadeVolume.GetValue());
            frames = frames > 0 ? frames : 1;
            m_MuteFadeVolume.SetTarget(0.0f, frames);
            m_MuteState = MuteState_Muting;
            break;
        case MuteState_Muted:
        default:
            return;
        }
    } else {
        switch (m_MuteState - 1) {
        case MuteState_Normal:
        case MuteState_Muting:
        case MuteState_Muted:
            frames = static_cast<int>(fadeFrames * (1.0f - m_MuteFadeVolume.GetValue()));
            frames = frames > 0 ? frames : 1;
            m_MuteFadeVolume.SetTarget(1.0f, frames);
            m_MuteState = MuteState_Unmuting;
            break;
        case MuteState_Unmuting:
        default:
            return;
        }
    }
}

void BasicSound::SetAutoStopCounter(int frames) {
    m_AutoStopCounter = frames;
    m_AutoStopFlag = frames > 0;
}

void BasicSound::FadeIn(int frames) {
    if (m_FadeOutFlag || m_UpdateCounter > 0)
        return;

    m_FadeVolume.InitValue(0.0f);
    m_FadeVolume.SetTarget(1.0f, frames);
}

bool BasicSound::IsPause() const {
    switch (m_PauseState - 1) {
    case PauseState_Normal:
    case PauseState_Pausing:
        return true;
    case PauseState_Paused:
    case PauseState_Unpausing:
    default:
        return false;
    }
}

bool BasicSound::IsMute() const {
    switch (m_MuteState - 1) {
    case MuteState_Normal:
    case MuteState_Muting:
        return true;
    case MuteState_Muted:
    case MuteState_Unmuting:
    default:
        return false;
    }
}

void BasicSound::UpdateMoveValue() {
    m_FadeVolume.Update();
    m_MuteFadeVolume.Update();
    m_CommonParam.Update();
}

float BasicSound::CalculateVolume() const {
    float volume{1.0f};

    if (m_MuteState == MuteState_Muted)
        return 0.0f;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (!IsVolumeThroughModeUsed())
#endif
        volume = m_InitVolume;

    volume *= m_pSoundPlayer->GetVolume();
    volume *= m_CommonParam.GetVolume();
    volume *= m_FadeVolume.GetValue();
    volume *= m_PauseFadeVolume.GetValue();
    volume *= m_MuteFadeVolume.GetValue();
    volume *= m_AmbientParam.GetVolume();
    volume *= m_ActorParam.volume;

    return volume;
}

float BasicSound::CalculatePitch() const {
    return m_Pitch * m_AmbientParam.GetPitch() * m_ActorParam.pitch;
}

float BasicSound::CalculateLpfFrequency() const {
    return m_LpfFreq + m_AmbientParam.GetLpf() + GetSoundPlayer()->GetLowPassFilterFrequency() +
           m_ActorParam.lpf;
}

uint32_t BasicSound::CalculateOutLineFlag() const {
    uint32_t outputLineFlag{m_OutputLineFlag};

    if (m_AmbientParam.GetOutputLineFlag() != SoundAmbientParam::OutputLineFlagInherit)
        outputLineFlag = m_AmbientParam.GetOutputLineFlag();

    return outputLineFlag;
}

void BasicSound::CalculateBiquadFilter(int* pOutBiquadType, float* pOutBiquadValue) const {
    int biquadFilterType{m_BiquadFilterType};
    float biquadFilterValue{m_BiquadFilterValue};

    if (biquadFilterType == BiquadFilterType_Inherit) {
        biquadFilterType = GetSoundPlayer()->GetBiquadFilterType();
        biquadFilterValue = GetSoundPlayer()->GetBiquadFilterValue();
        if (biquadFilterType == BiquadFilterType_Inherit) {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
            biquadFilterType = m_ActorParam.biquadFilterType;
            biquadFilterValue = m_ActorParam.biquadFilterValue;
#endif
            if (biquadFilterType == BiquadFilterType_Inherit) {
                biquadFilterType = m_AmbientParam.GetBiquadFilterType();
                biquadFilterValue = m_AmbientParam.GetBiquadFilterValue();
            }
        }
    }

    *pOutBiquadType = biquadFilterType;
    *pOutBiquadValue = biquadFilterValue;
}

void BasicSound::CalculateOutputParam(OutputParam* pOutParam, OutputDevice device) const {
    if (device != OutputDevice_Main) {
        NN_UNEXPECTED_DEFAULT;
        return;
    }

    const OutputAmbientParam* pAmbientParam{&m_AmbientParam.GetTvParam()};
    float actorVolume{m_ActorParam.tvVolume};
    float actorPan{m_ActorParam.tvPan};

    *pOutParam = m_OutputParam[device];

    ApplyCommonParam(*pOutParam);

    const SoundPlayer* pSoundPlayer{m_pSoundPlayer};

    pOutParam->volume *=
        pSoundPlayer->GetOutputVolume(device) * pAmbientParam->GetVolume() * actorVolume;
    pOutParam->pan += actorPan + pAmbientParam->GetPan();
    pOutParam->span += pAmbientParam->GetSurroundPan();

    for (int i{0}; i < DefaultBusCount; ++i) {
        if (i == OutputDeviceIndex_Main)
            pOutParam->send[i] += pSoundPlayer->GetOutputMainSend(device);
        else
            pOutParam->send[i] +=
                pSoundPlayer->GetOutputFxSend(device, static_cast<AuxBus>(i - 1)) +
                pAmbientParam->GetEffectSend(i - 1);
    }
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void BasicSound::CalculateOutputBusMixVolume(OutputBusMixVolume* pOutParam,
                                             OutputDevice device) const {
    *pOutParam = m_pOutputAdditionalParam[device]->GetBusMixVolume();
}
#endif

void BasicSound::ApplyCommonParam(OutputParam& param) const {
    param.mixMode = m_CommonParam.mixMode;
    param.pan += m_CommonParam.pan;
    param.span += m_CommonParam.span;

    for (int i{0}; i < DefaultBusCount; ++i)
        param.send[i] += m_CommonParam.send[i];
}

void BasicSound::AttachPlayerHeap(PlayerHeap* pHeap) {
    m_pPlayerHeap = pHeap;
}

void BasicSound::DetachPlayerHeap([[maybe_unused]] PlayerHeap* pHeap) {
    m_pPlayerHeap = nullptr;
}

void BasicSound::AttachSoundPlayer(SoundPlayer* player) {
    m_pSoundPlayer = player;
}

void BasicSound::DetachSoundPlayer([[maybe_unused]] SoundPlayer* player) {
    m_pSoundPlayer = nullptr;
}

void BasicSound::AttachSoundActor(SoundActor* actor) {
    m_pSoundActor = actor;
}

void BasicSound::DetachSoundActor([[maybe_unused]] SoundActor* actor) {
    m_pSoundActor = nullptr;
}

void BasicSound::AttachExternalSoundPlayer(ExternalSoundPlayer* extPlayer) {
    m_pExtSoundPlayer = extPlayer;
}

void BasicSound::DetachExternalSoundPlayer([[maybe_unused]] ExternalSoundPlayer* extPlayer) {
    m_pExtSoundPlayer = nullptr;
}

int BasicSound::GetRemainingFadeFrames() const {
    return m_FadeVolume.GetRemainingCount();
}

int BasicSound::GetRemainingPauseFadeFrames() const {
    return m_PauseFadeVolume.GetRemainingCount();
}

int BasicSound::GetRemainingMuteFadeFrames() const {
    return m_MuteFadeVolume.GetRemainingCount();
}

void BasicSound::CalculateSoundParamCalculationValues(
    SoundParamCalculationValues* pOutValue) const {
    pOutValue->soundArchiveParam.volume = m_InitVolume;

    SetSoundPlayerCalculationValues(pOutValue->soundPlayerParam, *m_pSoundPlayer);
    SetSound3DCalculationValues(pOutValue->sound3DParam, m_AmbientParam);
    SetSoundActorCalculationValues(pOutValue->soundActorParam, m_ActorParam);

    pOutValue->soundHandleParam.volume = m_CommonParam.GetVolume();
    pOutValue->soundHandleParam.pitch = m_Pitch;
    pOutValue->soundHandleParam.lpf = m_LpfFreq;
    pOutValue->soundHandleParam.bqfType = static_cast<int>(m_BiquadFilterType);
    pOutValue->soundHandleParam.bqfValue = m_BiquadFilterValue;
    pOutValue->soundHandleParam.outputLineFlag = m_OutputLineFlag;
    pOutValue->soundHandleParam.mixMode = m_CommonParam.mixMode;
    pOutValue->soundHandleParam.pan = m_CommonParam.pan;
    pOutValue->soundHandleParam.surroundPan = m_CommonParam.span;
    pOutValue->soundHandleParam.mainSend = m_CommonParam.send[OutputDeviceIndex_Main];
    pOutValue->soundHandleParam.playerPriority = m_Priority;

    for (int i{0}; i < AuxBus_Count; ++i)
        pOutValue->soundHandleParam.effectSend[i] = m_CommonParam.send[1L + i];

    for (int i{0}; i < OutputDevice_Count; ++i) {
        const OutputDevice device{static_cast<OutputDevice>(i)};

        pOutValue->soundHandleParam.outputVolume[device] = m_OutputParam[device].volume;
        pOutValue->soundHandleParam.outputPan[device] = m_OutputParam[device].pan;
        pOutValue->soundHandleParam.outputSurroundPan[device] = m_OutputParam[device].span;
        pOutValue->soundHandleParam.outputMainSend[device] =
            m_OutputParam[device].send[OutputDeviceIndex_Main];

        for (int k{0}; k < AuxBus_Count; ++k)
            pOutValue->soundHandleParam.outputEffectSend[device][k] =
                m_OutputParam[device].send[1 + k];

        pOutValue->soundHandleParam.outputMixParameter[device][0] =
            m_OutputParam[device].mixParameter[0];
        pOutValue->soundHandleParam.outputMixParameter[device][1] =
            m_OutputParam[device].mixParameter[1];
    }

    pOutValue->resultParam.volume = CalculateVolume();
    pOutValue->resultParam.pitch = CalculatePitch();
    pOutValue->resultParam.lpf = CalculateLpfFrequency();
    pOutValue->resultParam.outputLineFlag = CalculateOutLineFlag();
    pOutValue->resultParam.playerPriority = CalcCurrentPlayerPriority();
    CalculateBiquadFilter(&pOutValue->resultParam.bqfType, &pOutValue->resultParam.bqfValue);
    for (int i{0}; i < OutputDevice_Count; ++i)
        CalculateOutputParam(&pOutValue->resultParam.outputParamResult[i],
                             static_cast<OutputDevice>(i));

    pOutValue->fadeVolumeParam.stopFadeVolume = m_FadeVolume.GetValue();
    pOutValue->fadeVolumeParam.pauseFadeVolume = m_PauseFadeVolume.GetValue();
    pOutValue->fadeVolumeParam.muteFadeVolume = m_MuteFadeVolume.GetValue();
    pOutValue->fadeVolumeParam.isMuted = m_MuteState == MuteState_Muted;
}

void BasicSound::SetInitialVolume(float volume) {
    m_InitVolume = volume < 0.0f ? 0.0f : volume;
}

float BasicSound::GetInitialVolume() const {
    return m_InitVolume;
}

void BasicSound::SetVolume(float volume, int frames) {
    m_CommonParam.SetVolume(volume < 0.0f ? 0.0f : volume, frames);
}

float BasicSound::GetVolume() const {
    return m_CommonParam.GetVolume();
}

void BasicSound::SetPitch(float pitch) {
    m_Pitch = pitch;
}

float BasicSound::GetPitch() const {
    return m_Pitch;
}

void BasicSound::SetLpfFreq(float lpfFreq) {
    m_LpfFreq = lpfFreq;
}

float BasicSound::GetLpfFreq() const {
    return m_LpfFreq;
}

void BasicSound::SetBiquadFilter(int type, float value) {
    m_BiquadFilterType = static_cast<int8_t>(type);
    m_BiquadFilterValue = value;
}

void BasicSound::GetBiquadFilter(int* type, float* value) const {
    if (type != nullptr)
        *type = static_cast<int>(m_BiquadFilterType);

    if (value != nullptr)
        *value = m_BiquadFilterValue;
}

void BasicSound::SetOutputLine(uint32_t lineFlag) {
    m_OutputLineFlag = lineFlag;
}

uint32_t BasicSound::GetOutputLine() const {
    return m_OutputLineFlag;
}

void BasicSound::ResetOutputLine() {
    m_OutputLineFlag = m_pSoundPlayer->GetDefaultOutputLine();
}

void BasicSound::SetMixMode(MixMode mixMode) {
    m_CommonParam.mixMode = mixMode;
}

MixMode BasicSound::GetMixMode() {
    return m_CommonParam.mixMode;
}

void BasicSound::SetPan(float pan) {
    m_CommonParam.pan = pan;
}

float BasicSound::GetPan() const {
    return m_CommonParam.pan;
}

void BasicSound::SetSurroundPan(float span) {
    m_CommonParam.span = span;
}

float BasicSound::GetSurroundPan() const {
    return m_CommonParam.span;
}

void BasicSound::SetMainSend(float send) {
    m_CommonParam.send[0] = send;
}

float BasicSound::GetMainSend() const {
    return m_CommonParam.send[0];
}

void BasicSound::SetFxSend(AuxBus bus, float send) {
    m_CommonParam.send[1L + bus] = send;
}

float BasicSound::GetFxSend(AuxBus bus) const {
    return m_CommonParam.send[1L + bus];
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void BasicSound::SetSend(int bus, float send) {
    if (bus < DefaultBusCount) {
        m_CommonParam.send[bus] = send;
        return;
    }

    for (int device{0}; device < OutputDevice_Count; ++device)
        SetOutputAdditionalSend(static_cast<OutputDevice>(device), bus, send);
}

float BasicSound::GetSend(int bus) const {
    if (bus < DefaultBusCount)
        return m_CommonParam.send[bus];

    return GetOutputAdditionalSend(OutputDevice_Main, bus);
}

void BasicSound::SetVolumeThroughMode(int bus, uint8_t modeBitFlag) {
    for (int i{0}; i < OutputDevice_Count; ++i)
        SetOutputVolumeThroughMode(static_cast<OutputDevice>(i), bus, modeBitFlag);
}

uint8_t BasicSound::GetVolumeThroughMode(int bus) {
    return GetOutputVolumeThroughMode(OutputDevice_Main, bus);
}

int32_t BasicSound::GetSendBusCount() {
    if (m_pOutputReceiver == nullptr)
        return 0;

    return m_pOutputReceiver->GetBusCount();
}

int32_t BasicSound::GetSendChannelCount() {
    if (m_pOutputReceiver == nullptr)
        return 0;

    return m_pOutputReceiver->GetChannelCount();
}

bool BasicSound::IsVolumeThroughModeUsed() const {
    bool isVolumeThroughMode{false};

    for (int i{0}; i < OutputDevice_Count; ++i) {
        if (m_pOutputAdditionalParam[i] == nullptr)
            continue;

        if (!m_pOutputAdditionalParam[i]->IsVolumeThroughModeEnabled())
            continue;

        for (int j{0}; j < m_pOutputReceiver->GetBusCount(); ++j) {
            if (m_pOutputAdditionalParam[i]
                    ->GetVolumeThroughModePacketAddr()
                    ->TryGetVolumeThroughMode(j) != 0) {
                isVolumeThroughMode = true;
                return isVolumeThroughMode;
            }
        }
    }

    return isVolumeThroughMode;
}

void BasicSound::SetOutputAdditionalSend(OutputDevice device, int bus, float send) {
    if (m_pOutputAdditionalParam[device] == nullptr)
        return;

    if (!m_pOutputAdditionalParam[device]->IsAdditionalSendEnabled())
        return;

    SendArray* pAdditionalSend{m_pOutputAdditionalParam[device]->GetAdditionalSendAddr()};
    if (bus >= DefaultBusCount && pAdditionalSend->GetCount() > Util::GetAdditionalSendIndex(bus))
        pAdditionalSend->SetValue(Util::GetAdditionalSendIndex(bus), send);
}

void BasicSound::SetOutputVolumeThroughMode(OutputDevice device, int bus, uint8_t modeBitFlag) {
    if (m_pOutputAdditionalParam[device] == nullptr)
        return;

    if (!m_pOutputAdditionalParam[device]->IsVolumeThroughModeEnabled())
        return;

    VolumeThroughModePacket* pVolumeThroughMode{
        m_pOutputAdditionalParam[device]->GetVolumeThroughModePacketAddr(),
    };
    if (bus < 0 || pVolumeThroughMode->GetBusCount() <= bus)
        return;

    pVolumeThroughMode->SetVolumeThroughMode(bus, modeBitFlag);
}

void BasicSound::SetOutputBusMixVolume(OutputDevice device, int srcChNo, int subMixBus,
                                       ChannelMixVolume param) {
    if (m_pOutputReceiver == nullptr)
        return;

    if (m_pOutputAdditionalParam[device] == nullptr)
        return;

    if (!m_pOutputAdditionalParam[device]->IsBusMixVolumeEnabled())
        return;

    const int ChannelCount{std::min(param.GetChannelCount(), m_pOutputReceiver->GetChannelCount())};

    for (int i{0}; i < ChannelCount; ++i) {
        const float channelVolume{param.GetChannelVolume(i)};

        m_pOutputAdditionalParam[device]->SetBusMixVolume(
            srcChNo, Util::GetOutputReceiverMixBufferIndex(m_pOutputReceiver, subMixBus, i),
            channelVolume);
    }
}

void BasicSound::SetOutputBusMixVolumeEnabled(OutputDevice device, int subMixBus, bool isEnabled) {
    if (m_pOutputReceiver == nullptr)
        return;

    if (m_pOutputAdditionalParam[device] == nullptr)
        return;

    if (!m_pOutputAdditionalParam[device]->IsBusMixVolumeEnabled())
        return;

    OutputAdditionalParam* pAdditionalParam{m_pOutputAdditionalParam[device]};
    pAdditionalParam->SetBusMixVolumeEnabledForBus(subMixBus, isEnabled);
    pAdditionalParam->SetBusMixVolumeUsed(false);

    for (int i{0}; i < pAdditionalParam->GetBusMixVolumePacketAddr()->GetBusCount(); ++i) {
        if (pAdditionalParam->IsBusMixVolumeEnabledForBus(subMixBus)) {
            pAdditionalParam->SetBusMixVolumeUsed(true);
            break;
        }
    }
}

float BasicSound::GetOutputAdditionalSend(OutputDevice device, int bus) const {
    if (m_pOutputAdditionalParam[device] == nullptr)
        return 0.0f;

    if (m_pOutputAdditionalParam[device]->IsAdditionalSendEnabled())
        return 0.0f;

    SendArray* pAdditionalSend{m_pOutputAdditionalParam[device]->GetAdditionalSendAddr()};
    if (bus < DefaultBusCount || pAdditionalSend->GetCount() <= Util::GetAdditionalSendIndex(bus))
        return 0.0f;

    return pAdditionalSend->GetValue(Util::GetAdditionalSendIndex(bus));
}

uint8_t BasicSound::GetOutputVolumeThroughMode(OutputDevice device, int bus) const {
    if (m_pOutputAdditionalParam[device] == nullptr)
        return 0;

    if (m_pOutputAdditionalParam[device]->IsVolumeThroughModeEnabled())
        return 0;

    VolumeThroughModePacket* pVolumeThroughMode{
        m_pOutputAdditionalParam[device]->GetVolumeThroughModePacketAddr(),
    };
    if (bus < 0 || pVolumeThroughMode->GetBusCount() <= bus)
        return 0;

    return pVolumeThroughMode->GetVolumeThroughMode(bus);
}

ChannelMixVolume BasicSound::GetOutputBusMixVolume(OutputDevice device, int srcChNo,
                                                   int subMixBus) const {
    ChannelMixVolume result;
    if (m_pOutputReceiver == nullptr)
        return result;

    if (m_pOutputReceiver->GetBusCount() >= subMixBus)
        return result;

    if (m_pOutputAdditionalParam[device] == nullptr)
        return result;

    if (m_pOutputAdditionalParam[device]->IsBusMixVolumeEnabled())
        return result;

    const int ChannelCount{m_pOutputReceiver->GetChannelCount()};

    result.SetChannelCount(ChannelCount);

    for (int i{0}; i < ChannelCount; ++i) {
        const float volume{m_pOutputAdditionalParam[device]->GetBusMixVolume(srcChNo, subMixBus)};
        result.SetChannelVolume(i, volume);
    }

    return result;
}

bool BasicSound::IsOutputBusMixVolumeEnabled(OutputDevice device, int subMixBus) const {
    if (m_pOutputReceiver == nullptr)
        return false;

    if (m_pOutputAdditionalParam[device] == nullptr)
        return false;

    if (!m_pOutputAdditionalParam[device]->IsBusMixVolumeEnabled())
        return false;

    return m_pOutputAdditionalParam[device]->IsBusMixVolumeEnabledForBus(subMixBus);
}
#endif

void BasicSound::SetOutputVolume(OutputDevice device, float volume) {
    m_OutputParam[device].volume = volume;
}

void BasicSound::SetOutputChannelMixParameter(OutputDevice device, uint32_t srcChNo,
                                              MixParameter param) {
    for (int i{0}; i < ChannelIndex_Count; ++i)
        m_OutputParam[device].mixParameter[srcChNo].ch[i] = param.ch[i];
}

void BasicSound::SetOutputPan(OutputDevice device, float pan) {
    m_OutputParam[device].pan = pan;
}

void BasicSound::SetOutputSurroundPan(OutputDevice device, float span) {
    m_OutputParam[device].span = span;
}

void BasicSound::SetOutputMainSend(OutputDevice device, float send) {
    m_OutputParam[device].send[OutputDeviceIndex_Main] = send;
}

void BasicSound::SetOutputFxSend(OutputDevice device, AuxBus bus, float send) {
    m_OutputParam[device].send[1L + bus] = send;
}

float BasicSound::GetOutputVolume(OutputDevice device) const {
    return m_OutputParam[device].volume;
}

MixParameter BasicSound::GetOutputChannelMixParameter(OutputDevice device, uint32_t srcChNo) const {
    return m_OutputParam[device].mixParameter[srcChNo];
}

float BasicSound::GetOutputPan(OutputDevice device) const {
    return m_OutputParam[device].pan;
}

float BasicSound::GetOutputSurroundPan(OutputDevice device) const {
    return m_OutputParam[device].span;
}

float BasicSound::GetOutputMainSend(OutputDevice device) const {
    return m_OutputParam[device].send[OutputDeviceIndex_Main];
}

float BasicSound::GetOutputFxSend(OutputDevice device, AuxBus bus) const {
    return m_OutputParam[device].send[1L + bus];
}

void BasicSound::SetPanMode(PanMode mode) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandPlayerPanParam>()};

        command->id = DriverCommandId_PlayerPanmode;
        command->player = GetBasicSoundPlayerHandle();
        command->panMode = mode;

        cmdmgr.PushCommand(command);
    }
}

void BasicSound::SetPanCurve(PanCurve curve) {
    {
        DriverCommand& cmdmgr{DriverCommand::GetInstance()};
        auto* command{cmdmgr.AllocCommand<DriverCommandPlayerPanParam>()};

        command->id = DriverCommandId_PlayerPancurve;
        command->player = GetBasicSoundPlayerHandle();
        command->panCurve = curve;

        cmdmgr.PushCommand(command);
    }
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void BasicSound::SetOutputAdditionalParamAddr(OutputDevice device, OutputAdditionalParam* addr,
                                              OutputAdditionalParam* addrForPlayer) {
    m_pOutputAdditionalParam[device] = addr;
    GetBasicSoundPlayerHandle()->SetTvAdditionalParamAddr(addrForPlayer);
}
#endif

void BasicSound::SetAmbientInfo(const AmbientInfo& ambientArgInfo) {
    void* ambientArg{ambientArgInfo.argAllocatorCallback->AllocateArgImpl(ambientArgInfo.argSize)};

    if (ambientArg != nullptr) {
        std::memcpy(ambientArg, ambientArgInfo.arg, ambientArgInfo.argSize);
        m_AmbientInfo = ambientArgInfo;
        m_AmbientInfo.arg = ambientArg;
    }
}

int BasicSound::GetAmbientPriority(const AmbientInfo& ambientInfo, uint32_t soundId) {
    int priority{0};

    if (ambientInfo.paramUpdateCallback != nullptr)
        priority = ambientInfo.paramUpdateCallback->GetPriority(ambientInfo.arg, soundId);

    return priority;
}

void BasicSound::SetSetupTick(const os::Tick& tick) {
    m_SetupTick = tick;
}

void BasicSound::SetSoundArchive(const SoundArchive* soundArchive) {
    m_pSoundArchive = soundArchive;
}

}  // namespace nn::atk::detail
