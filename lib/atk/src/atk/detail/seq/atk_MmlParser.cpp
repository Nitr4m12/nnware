#include <nn/atk/atk_MmlParser.h>

#include <nn/atk/atk_SequenceSoundPlayer.h>
#include "nn/util/util_BytePtr.h"

namespace nn::atk::detail::driver {

namespace {

// const float ModSpeedBase{1 / 2.56};

}  // anonymous namespace

// TODO: SequenceTrack::ParseResult MmlParser::Parse

int32_t MmlParser::ReadArg(const uint8_t** ptr, SequenceSoundPlayer* player, SequenceTrack* track,
                           SeqArgType argType) const {
    int32_t var{0};

    switch (argType) {
    case SeqArgType_U8:
        var = ReadByte(ptr);
        break;
    case SeqArgType_S16:
        var = Read16(ptr);
        break;
    case SeqArgType_Vmidi:
        var = ReadVar(ptr);
        break;
    case SeqArgType_Random: {
        int32_t rand;
        int16_t min;
        int16_t max;

        min = Read16(ptr);
        max = Read16(ptr);

        rand = Util::CalcRandom();

        var = min + ((((1 - min) + max) * rand) >> 0x10);
        break;
    }
    case SeqArgType_Variable: {
        uint8_t varNo{ReadByte(ptr)};
        const volatile int16_t* varPtr{GetVariablePtr(player, track, varNo)};

        if (varPtr != nullptr)
            var = *varPtr;
        break;
    }
    case SeqArgType_None:
        break;
    }
    return var;
}

uint32_t MmlParser::Read24(const uint8_t** ptr) const {
    uint32_t ret{ReadByte(ptr)};
    ret |= ReadByte(ptr) << 8;
    ret |= ReadByte(ptr) << 16;
    return ret;
}

uint16_t MmlParser::Read16(const uint8_t** ptr) const {
    uint16_t ret{ReadByte(ptr)};
    ret |= ReadByte(ptr) << 8;
    return ret;
}

// TODO: MmlParser::CommandProc

volatile int16_t* MmlParser::GetVariablePtr(SequenceSoundPlayer* player, SequenceTrack* track,
                                            int32_t varNo) const {
    if (varNo < 32)
        return player->GetVariablePtr(varNo);

    if (varNo < 48)
        return track->GetVariablePtr(varNo - 32);

    return nullptr;
}

void MmlParser::NoteOnCommandProc(MmlSequenceTrack* track, int32_t key, int32_t velocity,
                                  int32_t length, bool tieFlag) const {
    track->NoteOn(key, velocity, length, tieFlag);
}

int32_t MmlParser::ReadVar(const uint8_t** ptr) const {
    int32_t ret{0};
    uint8_t b;
    [[maybe_unused]] int i{0};

    while (true) {
        b = ReadByte(ptr);
        ret <<= 7;
        ret |= b & 0x7f;
        ++i;

        if (!(b & 0x80))
            break;
    }

    return ret;
}

uint32_t MmlParser::ParseAllocTrack(const void* baseAddress, uint32_t seqOffset,
                                    uint32_t* allocTrack) {
    const uint8_t* ptr{util::ConstBytePtr(baseAddress, seqOffset).Get<uint8_t>()};

    if (ptr[0] != 0xfe) {
        *allocTrack = 1;
        return seqOffset;
    }

    *allocTrack = (ptr[1] << 8) | ptr[2] | 1;
    return seqOffset + 3;
}

}  // namespace nn::atk::detail::driver
