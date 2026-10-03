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
    m_ParserTrackParam.bendRange = 2;
    m_ParserTrackParam.initPan = 0;

    m_ParserTrackParam.transpose = 0;
    m_ParserTrackParam.priority = 64;

    m_ParserTrackParam.portaKey = 60;
    m_ParserTrackParam.portaTime = 0;

    m_ParserTrackParam.attack = 255;
    m_ParserTrackParam.decay = 255;
    m_ParserTrackParam.sustain = 255;
    m_ParserTrackParam.release = 255;
    m_ParserTrackParam.envHold = 255;

    m_ParserTrackParam.mainSend = 127;
    for (int i{0}; i < AuxBus_Count; ++i)
        m_ParserTrackParam.fxSend[i] = 0;

    m_ParserTrackParam.lpfFreq = 0.0f;

    m_ParserTrackParam.biquadType = BiquadFilterType_None;
    m_ParserTrackParam.biquadValue = 0.0f;

    m_ParserTrackParam.outputLine = -1;

    for (int varNo{0}; varNo < TrackVariableCount; ++varNo)
        m_TrackVariable[varNo] = -1;

    m_ForceMute = false;
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

}  // namespace nn::atk::detail::driver
