#pragma once

#include <nn/atk/atk_MmlSequenceTrack.h>

namespace nn::atk::detail::driver {

class MmlParser {
public:
    static const int PanCenter{64};
    static const int SurroundPanCenter{PanCenter};

    static const int TempoMin{0};
    static const int TempoMax{1023};

    enum SeqArgType {
        SeqArgType_None,
        SeqArgType_U8,
        SeqArgType_S16,
        SeqArgType_Vmidi,
        SeqArgType_Random,
        SeqArgType_Variable,
    };

    virtual ~MmlParser() = default;

    SequenceTrack::ParseResult Parse(MmlSequenceTrack* track, bool doNoteOn) const;

    static uint32_t ParseAllocTrack(const void* baseAddress, uint32_t seqOffset,
                                    uint32_t* allocTrack);

    static bool EnablePrintVar() { return *mPrintVarEnabledFlag = true; }
    static bool IsEnabledPrintVar() { return *mPrintVarEnabledFlag; }

protected:
    virtual void CommandProc(MmlSequenceTrack* track, uint32_t command, int32_t commandArg1,
                             int32_t commandArg2) const;

    virtual void NoteOnCommandProc(MmlSequenceTrack* track, int32_t key, int32_t velocity,
                                   int32_t length, bool tieFlag) const;

private:
    uint8_t ReadByte(const uint8_t** ptr) const { return *(*ptr)++; }
    void UnreadByte(const uint8_t** ptr) const { (*ptr)--; }

    uint16_t Read16(const uint8_t** ptr) const;
    uint32_t Read24(const uint8_t** ptr) const;
    int32_t ReadVar(const uint8_t** ptr) const;

    int32_t ReadArg(const uint8_t** ptr, SequenceSoundPlayer* player, SequenceTrack* track,
                    SeqArgType argType) const;

    volatile int16_t* GetVariablePtr(SequenceSoundPlayer* player, SequenceTrack* track,
                                     int32_t varNo) const;

    static bool* mPrintVarEnabledFlag;
};
static_assert(sizeof(MmlParser) == 0x8);

}  // namespace nn::atk::detail::driver
