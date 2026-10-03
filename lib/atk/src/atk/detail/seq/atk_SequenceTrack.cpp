#include <nn/atk/atk_SequenceTrack.h>

namespace nn::atk::detail::driver {

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

// NON_MATCHING
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
        if (m_ParserTrackParam.wait != 0)
            return 1;
    }

    if (m_ParserTrackParam.currentAddr != nullptr && m_ParserTrackParam.wait == 0) {
        int counter{0};
        const int CounterMax{10000};

        while (m_ParserTrackParam.wait == 0) {
            if (m_ParserTrackParam.noteFinishWait)
                return 1;

            if (counter > CounterMax - 1)
                return 1;

            ParseResult result{Parse(doNoteOn)};
            if (result == ParseResult_Finish)
                return -1;

            ++counter;
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

// TODO: SequenceTrack::UpdateChannelParam

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

}  // namespace nn::atk::detail::driver
