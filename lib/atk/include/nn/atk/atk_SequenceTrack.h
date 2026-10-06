#pragma once

#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_ProfileReader.h>
#include <nn/os/os_TickTypes.h>

namespace nn::atk::detail::driver {

class SequenceSoundPlayer;

class SequenceTrack {
public:
    static const int CallStackDepth{10};

    struct ParserTrackParam {
        struct CallStack {
            uint8_t loopFlag;
            uint8_t loopCount;
            uint8_t padding[2];
            const uint8_t* address;
        };
        static_assert(sizeof(CallStack) == 0x10);

        const uint8_t* baseAddr;
        const uint8_t* currentAddr;
        bool cmpFlag;
        bool noteWaitFlag;
        bool tieFlag;
        bool monophonicFlag;
        CallStack callStack[CallStackDepth];
        uint8_t callStackDepth;
        bool frontBypassFlag;
        bool muteFlag;
        bool silenceFlag;
        int32_t wait;
        bool noteFinishWait;
        bool portaFlag;
        bool damperFlag;
        uint8_t bankIndex;
        int prgNo;
        float sweepPitch;
        MoveValue<uint8_t, int16_t> volume;
        MoveValue<uint8_t, int16_t> volume2;
        MoveValue<int8_t, int16_t> pan;
        MoveValue<int8_t, int16_t> surroundPan;
        MoveValue<int8_t, int16_t> pitchBend;
        CurveLfoParam lfoParam[Channel::ModCount]{};
        uint8_t lfoTarget[Channel::ModCount];
        uint8_t velocityRange;
        uint8_t bendRange;
        int8_t initPan;
        uint8_t padding1[1];
        int8_t transpose;
        uint8_t priority;
        uint8_t portaKey;
        uint8_t portaTime;
        uint8_t attack;
        uint8_t decay;
        uint8_t sustain;
        uint8_t release;
        int16_t envHold;
        int8_t biquadType;
        uint8_t mainSend;
        uint8_t fxSend[AuxBus_Count];
        uint8_t padding2[1];
        float lpfFreq;
        float biquadValue;
        int32_t outputLine;

        ParserTrackParam() = default;
    };
    static_assert(sizeof(ParserTrackParam) == 0x150);

    enum ParseResult {
        ParseResult_Continue,
        ParseResult_Finish,
    };

    static const int DefaultPriority{64};
    static const int DefaultBendRange{2};
    static const int DefaultPortaKey{60};
    static const int InvalidEnvelope{255};
    static const int MaxEnvelopeValue{127};
    static const int ParserParamSize{32};
    static const int TrackVariableCount{16};

    static const int PauseReleaseValue{127};
    static const int MuteReleaseValue{127};

    static void ChannelCallbackFunc(Channel* dropChannel, Channel::ChannelCallbackStatus status,
                                    void* userData);

    SequenceTrack();
    virtual ~SequenceTrack();

    void InitParam();

    void SetSeqData(const void* seqBase, int seqOffset);

    void Open();
    void Close();

    bool IsOpened() const { return m_OpenFlag; }

    int ParseNextTick(bool doNoteOn);

    void UpdateChannelLength();
    void UpdateChannelParam();

    Channel* NoteOn(int key, int velocity, int length, bool tieFlag);

    void StopAllChannel();
    void ReleaseAllChannel(int release);
    void FreeAllChannel();
    void PauseAllChannel(bool flag);

    int GetChannelCount() const;

    const ParserTrackParam& GetParserTrackParam() const { return m_ParserTrackParam; }
    ParserTrackParam& GetParserTrackParam() { return m_ParserTrackParam; }

    void SetMute(SequenceMute mute);
    void SetSilence(bool silenceFlag, int fadeTimes);

    void SetVolume(float volume) { m_ExtVolume = volume; }
    void SetPitch(float pitch) { m_ExtPitch = pitch; }
    void SetPanRange(float panRange) { m_PanRange = panRange; }

    void SetLpfFreq(float lpfFreq) { m_ParserTrackParam.lpfFreq = lpfFreq; }
    void SetBiquadFilter(int type, float value);
    void SetBankIndex(int bankIndex);
    void SetTranspose(int8_t transpose);
    void SetVelocityRange(uint8_t range);
    void SetOutputLine(int outputLine);

    void SetTvVolume(float volume) { m_TvParam.volume = volume; }
    void SetTvMixParameter(uint32_t srcChNo, int mixChNo, float param);
    void SetTvPan(float pan) { m_TvParam.pan = pan; }
    void SetTvSurroundPan(float span) { m_TvParam.span = span; }
    void SetTvMainSend(float mainSend) { m_TvParam.send[0] = mainSend; }
    void SetTvFxSend(AuxBus bus, float send) { m_TvParam.send[bus + 1] = send; }

    float GetVolume() const { return m_ExtVolume; }
    float GetPitch() const { return m_ExtPitch; }
    float GetPanRange() const { return m_PanRange; }

    float GetLpfFreq() const { return m_ParserTrackParam.lpfFreq; }
    int GetBiquadType() const { return m_ParserTrackParam.biquadType; }
    float GetBiquadValue() const { return m_ParserTrackParam.biquadValue; }

    int16_t GetTrackVariable(int varNo) const;
    void SetTrackVariable(int varNo, int16_t var);
    volatile int16_t* GetVariablePtr(int varNo);

    void SetSequenceSoundPlayer(SequenceSoundPlayer* player) { m_pSequenceSoundPlayer = player; }
    const SequenceSoundPlayer* GetSequenceSoundPlayer() const { return m_pSequenceSoundPlayer; }
    SequenceSoundPlayer* GetSequenceSoundPlayer() { return m_pSequenceSoundPlayer; }

    void SetPlayerTrackNo(int playerTrackNo);
    uint8_t GetPlayerTrackNo() const { return m_PlayerTrackNo; }

    void UpdateChannelRelease(Channel* channel);

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsOpened())
            return 0;

        os::Tick totalTick{0};

        for (Channel* channel{m_pChannelList}; channel != nullptr;
             channel = channel->GetNextTrackChannel())
            totalTick += channel->GetProcessTick(profile);

        return totalTick;
    }

    void ForceMute();

protected:
    virtual ParseResult Parse(bool doNoteOn) = 0;

private:
    Channel* GetLastChannel() const { return m_pChannelList; }

    void AddChannel(Channel* channel);

    uint8_t m_PlayerTrackNo;
    bool m_OpenFlag;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    bool m_ForceMute;
#endif
    float m_ExtVolume;
    float m_ExtPitch;
    float m_PanRange;
    OutputParam m_TvParam;
    ParserTrackParam m_ParserTrackParam;
    volatile int16_t m_TrackVariable[TrackVariableCount];
    SequenceSoundPlayer* m_pSequenceSoundPlayer;
    Channel* m_pChannelList;
};
static_assert(sizeof(SequenceTrack) == 0x1e8);

}  // namespace nn::atk::detail::driver
