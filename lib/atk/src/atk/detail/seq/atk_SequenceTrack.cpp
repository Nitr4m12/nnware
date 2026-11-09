#include <nn/atk/atk_SequenceTrack.h>

#include <nn/atk/atk_Bank.h>
#include <nn/atk/atk_SequenceSoundPlayer.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail::driver {

void SequenceTrack::SetPlayerTrackNo(int playerTrackNo) {
    m_PlayerTrackNo = playerTrackNo;
}

SequenceTrack::SequenceTrack()
    : m_OpenFlag{false}, m_pSequenceSoundPlayer{nullptr}, m_pChannelList{nullptr} {
    InitParam();
}

void SequenceTrack::InitParam() {
    m_ExtVolume = 1.0f;
    m_ExtPitch = 1.0f;
    m_PanRange = 1.0f;

    m_TvParam.Initialize();

    m_ParserTrackParam.baseAddr = nullptr;
    m_ParserTrackParam.currentAddr = nullptr;

    m_ParserTrackParam.cmpFlag = true;
    m_ParserTrackParam.noteWaitFlag = true;
    m_ParserTrackParam.tieFlag = false;
    m_ParserTrackParam.monophonicFlag = false;

    m_ParserTrackParam.callStackDepth = 0;

    m_ParserTrackParam.muteFlag = false;
    m_ParserTrackParam.silenceFlag = false;
    m_ParserTrackParam.wait = 0;

    m_ParserTrackParam.noteFinishWait = false;
    m_ParserTrackParam.portaFlag = false;
    m_ParserTrackParam.damperFlag = false;

    m_ParserTrackParam.bankIndex = 0;
    m_ParserTrackParam.prgNo = 0;

    for (int i{0}; i < Channel::ModCount; ++i) {
        m_ParserTrackParam.lfoParam[i].Initialize();
        m_ParserTrackParam.lfoTarget[i] = 0;
    }

    m_ParserTrackParam.sweepPitch = 0.0f;

    m_ParserTrackParam.volume.InitValue(127);
    m_ParserTrackParam.volume2.InitValue(127);
    m_ParserTrackParam.pan.InitValue(0);
    m_ParserTrackParam.surroundPan.InitValue(0);

    m_ParserTrackParam.velocityRange = 127;

    m_ParserTrackParam.pitchBend.InitValue(0);
    m_ParserTrackParam.bendRange = DefaultBendRange;
    m_ParserTrackParam.initPan = 0;

    m_ParserTrackParam.transpose = 0;
    m_ParserTrackParam.priority = DefaultPriority;

    m_ParserTrackParam.portaKey = DefaultPortaKey;
    m_ParserTrackParam.portaTime = 0;

    m_ParserTrackParam.attack = InvalidEnvelope;
    m_ParserTrackParam.decay = InvalidEnvelope;
    m_ParserTrackParam.sustain = InvalidEnvelope;
    m_ParserTrackParam.release = InvalidEnvelope;
    m_ParserTrackParam.envHold = InvalidEnvelope;

    m_ParserTrackParam.mainSend = MaxEnvelopeValue;
    for (int i{0}; i < AuxBus_Count; ++i)
        m_ParserTrackParam.fxSend[i] = 0;

    m_ParserTrackParam.lpfFreq = 0.0f;

    m_ParserTrackParam.biquadType = BiquadFilterType_None;
    m_ParserTrackParam.biquadValue = 0.0f;

    m_ParserTrackParam.outputLine = -1;

    for (int varNo{0}; varNo < TrackVariableCount; ++varNo)
        m_TrackVariable[varNo] = -1;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    m_ForceMute = false;
#endif
}

SequenceTrack::~SequenceTrack() {
    Close();
}

void SequenceTrack::Close() {
    ReleaseAllChannel(-1);
    FreeAllChannel();
    m_OpenFlag = false;
}

void SequenceTrack::SetSeqData(const void* seqBase, int seqOffset) {
    m_ParserTrackParam.baseAddr = static_cast<const uint8_t*>(seqBase);
    m_ParserTrackParam.currentAddr = m_ParserTrackParam.baseAddr + seqOffset;
}

void SequenceTrack::Open() {
    m_ParserTrackParam.noteFinishWait = false;
    m_ParserTrackParam.callStackDepth = 0;
    m_ParserTrackParam.wait = 0;

    m_OpenFlag = true;
}

void SequenceTrack::ReleaseAllChannel(int release) {
    UpdateChannelParam();

    Channel* channel{m_pChannelList};
    while (channel != nullptr) {
        if (channel->IsActive()) {
            if (release >= 0)
                channel->SetRelease(static_cast<uint8_t>(release));
            channel->Release();
        }

        channel = channel->GetNextTrackChannel();
    }
}

void SequenceTrack::FreeAllChannel() {
    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        channel->DetachChannel(channel);
        channel = channel->GetNextTrackChannel();
    }

    m_pChannelList = nullptr;
}

void SequenceTrack::UpdateChannelLength() {
    if (!m_OpenFlag)
        return;

    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        if (channel->GetLength() > 0)
            channel->SetLength(channel->GetLength() - 1);

        UpdateChannelRelease(channel);

        if (!channel->IsAutoUpdateSweep())
            channel->UpdateSweep(1);

        channel = channel->GetNextTrackChannel();
    }
}

void SequenceTrack::UpdateChannelRelease(Channel* channel) {
    if (channel->GetLength() == 0 && !channel->IsRelease() && !m_ParserTrackParam.damperFlag)
        channel->NoteOff();
}

int SequenceTrack::ParseNextTick(bool doNoteOn) {
    if (!m_OpenFlag)
        return 0;

    m_ParserTrackParam.volume.Update();
    m_ParserTrackParam.volume2.Update();
    m_ParserTrackParam.pan.Update();
    m_ParserTrackParam.surroundPan.Update();
    m_ParserTrackParam.pitchBend.Update();

    if (m_ParserTrackParam.noteFinishWait) {
        if (m_pChannelList != nullptr)
            return 1;

        m_ParserTrackParam.noteFinishWait = false;
    }

    if (m_ParserTrackParam.wait > 0) {
        --m_ParserTrackParam.wait;
        if (m_ParserTrackParam.wait > 0)
            return 1;
    }

    if (m_ParserTrackParam.currentAddr != nullptr) {
        int counter{0};
        const int CounterMax{10000};

        while (m_ParserTrackParam.wait == 0 && !m_ParserTrackParam.noteFinishWait) {
            ++counter;
            if (counter > CounterMax)
                break;

            ParseResult result{Parse(doNoteOn)};
            if (result == ParseResult_Finish)
                return -1;
        }
    }

    return 1;
}

void SequenceTrack::StopAllChannel() {
    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        Channel* nextChannel{channel->GetNextTrackChannel()};

        channel->Stop();
        channel->CallChannelCallback(Channel::ChannelCallbackStatus_Stopped);
        channel->FreeChannel(channel);

        channel = nextChannel;
    }

    m_pChannelList = nullptr;
}

void SequenceTrack::UpdateChannelParam() {
    if (!m_OpenFlag)
        return;

    if (m_pChannelList == nullptr)
        return;

    float volume;
    volume = m_ParserTrackParam.volume.GetValue() * m_ParserTrackParam.volume2.GetValue() *
             m_pSequenceSoundPlayer->GetParserPlayerParam().volume.GetValue() / static_cast<float>(PauseReleaseValue * MuteReleaseValue * MaxEnvelopeValue);
    volume = volume * volume * m_ExtVolume * m_pSequenceSoundPlayer->GetVolume();

    float pitch{(m_ParserTrackParam.pitchBend.GetValue() / 128.0f) * m_ParserTrackParam.bendRange};

    float pitchRatio{m_pSequenceSoundPlayer->GetPitch() * m_ExtPitch};

    float lpfFreq{m_ParserTrackParam.lpfFreq + m_pSequenceSoundPlayer->GetLpfFreq()};

    int32_t biquadType{m_ParserTrackParam.biquadType};
    float biquadValue{m_ParserTrackParam.biquadValue};
    if (m_pSequenceSoundPlayer->GetBiquadFilterType() != BiquadFilterType_Inherit) {
        biquadType = m_pSequenceSoundPlayer->GetBiquadFilterType();
        biquadValue = m_pSequenceSoundPlayer->GetBiquadFilterValue();
    }

    uint32_t outputLine{m_pSequenceSoundPlayer->GetOutputLine()};
    if (m_ParserTrackParam.outputLine != -1)
        outputLine = m_ParserTrackParam.outputLine;

    float panBase{fnd::Clamp(m_ParserTrackParam.pan.GetValue() / 63.0f, -1.0f, 1.0f) * m_PanRange * m_pSequenceSoundPlayer->GetPanRange()};

    float spanBase{fnd::Clamp(m_ParserTrackParam.surroundPan.GetValue() / 63.0f, 0.0f, 2.0f)};

    float mainSendBase{static_cast<float>(m_ParserTrackParam.mainSend) / 127.0f - 1.0f};

    float fxSendBase[AuxBus_Count];
    for (int i{0}; i < AuxBus_Count; ++i)
        fxSendBase[i] = m_ParserTrackParam.fxSend[i] / 127.0f;

    OutputParam tvParam{m_pSequenceSoundPlayer->GetTvParam()};
    {
        tvParam.volume *= m_TvParam.volume;

        for (int i{0}; i < WaveChannelMax; ++i) {
            for (int j{0}; j < ChannelIndex_Count; ++j)
                tvParam.mixParameter[i].ch[j] *= m_TvParam.mixParameter[i].ch[j];
        }

        tvParam.pan += panBase;
        tvParam.pan += m_TvParam.pan;
        tvParam.span += spanBase;
        tvParam.span += m_TvParam.span;

        const int BusCount{DefaultBusCount};
        for (int i{0}; i < BusCount; ++i) {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
            tvParam.send[i] += i < BusCount - AuxBus_Count ? mainSendBase : fxSendBase[i - 1];
#endif
            tvParam.send[i] += m_TvParam.send[i];
        }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        for (int i{0}; i < BusCount; ++i) {
            tvParam.send[i] += i < BusCount - AuxBus_Count ? mainSendBase : fxSendBase[i - 1];
        }
#endif
    }

    Channel* channel{m_pChannelList};
    while (channel != nullptr) {
        channel->SetUserVolume(volume);
        channel->SetUserPitch(pitch);
        channel->SetUserPitchRatio(pitchRatio);
        channel->SetUserLpfFreq(lpfFreq);
        channel->SetBiquadFilter(biquadType, biquadValue);
        channel->SetOutputLine(outputLine);

        for (int i{0}; i < Channel::ModCount; ++i) {
            channel->SetLfoParam(m_ParserTrackParam.lfoParam[i], i);
            channel->SetLfoTarget(static_cast<Channel::LfoTarget>(m_ParserTrackParam.lfoTarget[i]),
                                  i);
        }

        channel->SetTvParam(tvParam);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        if (m_pSequenceSoundPlayer->GetTvAdditionalParamAddr() != nullptr)
            channel->SetTvAdditionalParam(*m_pSequenceSoundPlayer->GetTvAdditionalParamAddr());
#endif

        channel = channel->GetNextTrackChannel();
    }
}

void SequenceTrack::PauseAllChannel(bool flag) {
    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        if (channel->IsActive() && channel->IsPause() != flag)
            channel->Pause(flag);

        channel = channel->GetNextTrackChannel();
    }
}

void SequenceTrack::AddChannel(Channel* channel) {
    channel->SetNextTrackChannel(m_pChannelList);
    m_pChannelList = channel;
}

int SequenceTrack::GetChannelCount() const {
    int count{0};
    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        ++count;
        channel = channel->GetNextTrackChannel();
    }

    return count;
}

void SequenceTrack::ChannelCallbackFunc(Channel* dropChannel,
                                        [[maybe_unused]] Channel::ChannelCallbackStatus status,
                                        void* userData) {
    SequenceTrack* track{static_cast<SequenceTrack*>(userData)};

    if (track->m_pSequenceSoundPlayer != nullptr)
        track->m_pSequenceSoundPlayer->ChannelCallback(dropChannel);

    Channel* channel{track->m_pChannelList};
    if (channel == dropChannel) {
        track->m_pChannelList = dropChannel->GetNextTrackChannel();
        return;
    }

    for (; channel->GetNextTrackChannel() != nullptr; channel = channel->GetNextTrackChannel()) {
        if (channel->GetNextTrackChannel() == dropChannel) {
            channel->SetNextTrackChannel(dropChannel->GetNextTrackChannel());
            break;
        }
    }
}

void SequenceTrack::SetMute(SequenceMute mute) {
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (m_ForceMute)
        return;
#endif

    switch (mute) {
    case SequenceMute_NoStop:
        m_ParserTrackParam.muteFlag = true;
        return;

    case SequenceMute_Off:
        m_ParserTrackParam.muteFlag = false;
        return;

    case SequenceMute_Release:
        ReleaseAllChannel(-1);
        FreeAllChannel();
        m_ParserTrackParam.muteFlag = true;
        return;

    case SequenceMute_Stop:
        StopAllChannel();
        m_ParserTrackParam.muteFlag = true;
        return;
    }
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
void SequenceTrack::ForceMute() {
    m_ForceMute = true;
    m_ParserTrackParam.muteFlag = true;
}
#endif

void SequenceTrack::SetSilence(bool silenceFlag, int fadeTimes) {
    m_ParserTrackParam.silenceFlag = silenceFlag;

    Channel* channel{m_pChannelList};

    while (channel != nullptr) {
        channel->SetSilence(silenceFlag, static_cast<uint32_t>(fadeTimes + 4) / 5);
        channel = channel->GetNextTrackChannel();
    }
}

void SequenceTrack::SetBiquadFilter(int type, float value) {
    m_ParserTrackParam.biquadType = type;
    m_ParserTrackParam.biquadValue = value;
}

void SequenceTrack::SetBankIndex(int bankIndex) {
    m_ParserTrackParam.bankIndex = bankIndex;
}

void SequenceTrack::SetTranspose(int8_t transpose) {
    m_ParserTrackParam.transpose = transpose;
}

void SequenceTrack::SetVelocityRange(uint8_t range) {
    m_ParserTrackParam.velocityRange = range;
}

void SequenceTrack::SetOutputLine(int outputLine) {
    m_ParserTrackParam.outputLine = outputLine;
}

void SequenceTrack::SetTvMixParameter(uint32_t srcChNo, int mixChNo, float param) {
    m_TvParam.mixParameter[srcChNo].ch[mixChNo] = param;
}

int16_t SequenceTrack::GetTrackVariable(int varNo) const {
    return m_TrackVariable[varNo];
}

void SequenceTrack::SetTrackVariable(int varNo, int16_t var) {
    m_TrackVariable[varNo] = var;
}

volatile int16_t* SequenceTrack::GetVariablePtr(int varNo) {
    if (varNo < TrackVariableCount)
        return m_TrackVariable + varNo;

    return nullptr;
}

Channel* SequenceTrack::NoteOn(int key, int velocity, int length, bool tieFlag) {
    const SequenceSoundPlayer::ParserPlayerParam& playerParam{
        m_pSequenceSoundPlayer->GetParserPlayerParam(),
    };
    Channel* channel{};
    velocity = (m_ParserTrackParam.velocityRange * velocity) / 127;
    if (tieFlag) {
        channel = GetLastChannel();
        if (channel != nullptr) {
            channel->SetKey(key);
            channel->SetVelocity(Bank::CalcChannelVelocityVolume(velocity));
        }
    }

    if (m_ParserTrackParam.monophonicFlag) {
        channel = GetLastChannel();
        if (channel != nullptr) {
            if (channel->IsRelease()) {
                channel->Stop();
                channel->CallChannelCallback(Channel::ChannelCallbackStatus_Stopped);
                Channel::FreeChannel(channel);
                channel = nullptr;
            } else {
                channel->SetKey(key);
                channel->SetVelocity(Bank::CalcChannelVelocityVolume(velocity));
                channel->SetLength(length);
            }
        }
    }

    if (channel == nullptr) {
        driver::NoteOnInfo noteOnInfo;
        noteOnInfo.prgNo = m_ParserTrackParam.prgNo;
        noteOnInfo.key = key;
        noteOnInfo.velocity = velocity;
        noteOnInfo.length = tieFlag ? -1 : length;
        noteOnInfo.initPan = m_ParserTrackParam.initPan;
        noteOnInfo.priority = playerParam.priority + m_ParserTrackParam.priority;
        noteOnInfo.channelCallback = ChannelCallbackFunc;
        noteOnInfo.channelCallbackData = this;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        noteOnInfo.pOutputReceiver = m_pSequenceSoundPlayer->GetOutputReceiver();
#endif
        noteOnInfo.updateType = m_pSequenceSoundPlayer->GetUpdateType();
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        noteOnInfo.subMixIndex = m_pSequenceSoundPlayer->GetSubMixIndex();
#endif

        channel = m_pSequenceSoundPlayer->NoteOn(m_ParserTrackParam.bankIndex, noteOnInfo);
        if (channel == nullptr)
            return nullptr;

        if (channel->GetKeyGroupId() > 0) {
            for (Channel* itr{m_pChannelList}; itr != nullptr; itr = itr->GetNextTrackChannel()) {
                if (itr->GetKeyGroupId() == channel->GetKeyGroupId()) {
                    itr->SetRelease(126);
                    itr->Release();
                }
            }
        }

        AddChannel(channel);
    }

    if (m_ParserTrackParam.attack <= MaxEnvelopeValue)
        channel->SetAttack(m_ParserTrackParam.attack);

    if (m_ParserTrackParam.decay <= MaxEnvelopeValue)
        channel->SetDecay(m_ParserTrackParam.decay);

    if (m_ParserTrackParam.sustain <= MaxEnvelopeValue)
        channel->SetSustain(m_ParserTrackParam.sustain);

    if (m_ParserTrackParam.release <= MaxEnvelopeValue)
        channel->SetRelease(m_ParserTrackParam.release);

    if (m_ParserTrackParam.envHold <= MaxEnvelopeValue)
        channel->SetHold(m_ParserTrackParam.envHold);

    float sweepPitch{m_ParserTrackParam.sweepPitch};
    if (m_ParserTrackParam.portaFlag)
        sweepPitch += m_ParserTrackParam.portaKey - key;

    if (m_ParserTrackParam.portaTime != 0) {
        int32_t sweepTime{m_ParserTrackParam.portaTime};
        sweepTime *= m_ParserTrackParam.portaTime;
        sweepTime = sweepTime * (sweepPitch >= 0.0f ? sweepPitch : -sweepPitch);
        sweepTime >>= 5;
        sweepTime *= 5;

        channel->SetSweepParam(sweepPitch, sweepTime, true);
    } else {
        channel->SetSweepParam(sweepPitch, length, false);
    }

    m_ParserTrackParam.portaKey = key;

    channel->SetSilence(m_ParserTrackParam.silenceFlag, 0);
    channel->SetReleasePriorityFix(m_pSequenceSoundPlayer->IsReleasePriorityFix());
    channel->SetPanMode(m_pSequenceSoundPlayer->GetPanMode());
    channel->SetPanCurve(m_pSequenceSoundPlayer->GetPanCurve());

    return channel;
}

}  // namespace nn::atk::detail::driver
