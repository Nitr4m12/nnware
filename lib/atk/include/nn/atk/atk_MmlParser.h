#pragma once

#include <nn/atk/atk_MmlSequenceTrack.h>

namespace nn::atk::detail::driver {

class MmlParser {
public:
    enum SeqArgType {
        SeqArgType_None,
        SeqArgType_U8,
        SeqArgType_S16,
        SeqArgType_Vmidi,
        SeqArgType_Random,
        SeqArgType_Variable,
    };

    constexpr static uint32_t PanCenter = 64;
    constexpr static uint32_t SurroundPanCenter = PanCenter;

    constexpr static uint32_t TempoMin = 0;
    constexpr static uint32_t TempoMax = 1023;

    MmlParser();
    virtual ~MmlParser();

    SequenceTrack::ParseResult Parse(MmlSequenceTrack* track, bool doNoteOn) const;

    int32_t ReadArg(const uint8_t** ptr, SequenceSoundPlayer* player, SequenceTrack* track,
                    SeqArgType argType) const;

    int32_t Read24(const uint8_t** ptr) const;
    int16_t Read16(const uint8_t** ptr) const;

    void CommandProc(MmlSequenceTrack* track, uint32_t command, int32_t commandArg1,
                     int32_t commandArg2) const;

    int16_t* GetVariablePtr(SequenceSoundPlayer* player, SequenceTrack* track, int32_t varNo) const;

    void NoteOnCommandProc(MmlSequenceTrack* track, int32_t key, int32_t velocity, int32_t length,
                           bool tieFlag) const;

    int16_t ReadVar(const uint8_t** ptr) const;

    static uint32_t ParseAllocTrack(const void* baseAddress, uint32_t seqOffset,
                                    uint32_t* allocTrack);

private:
    static bool* mPrintVarEnabledFlag;
};
static_assert(sizeof(MmlParser) == 0x8);

}  // namespace nn::atk::detail::driver
