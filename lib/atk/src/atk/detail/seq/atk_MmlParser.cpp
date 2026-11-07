#include <nn/atk/atk_MmlParser.h>

#include <nn/atk/atk_MmlCommand.h>
#include <nn/atk/atk_SequenceSoundPlayer.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail::driver {

namespace {

// const float ModSpeedBase{1 / 2.56};

}  // anonymous namespace

SequenceTrack::ParseResult MmlParser::Parse(MmlSequenceTrack* track, bool doNoteOn) const {
    SequenceSoundPlayer* player{track->GetSequenceSoundPlayer()};

    SequenceTrack::ParserTrackParam& trackParam{track->GetParserTrackParam()};
    SeqArgType argType{SeqArgType_None};
    SeqArgType argType2{SeqArgType_None};
    bool useArgType{false};
    bool doExecCommand{true};

    uint32_t cmd{ReadByte(&trackParam.currentAddr)};
    if (cmd == MmlCommand::Mml_If) {
        cmd = ReadByte(&trackParam.currentAddr);
        doExecCommand = trackParam.cmpFlag;
    }

    switch (cmd) {
    case MmlCommand::Mml_Time:
        cmd = ReadByte(&trackParam.currentAddr);
        argType2 = SeqArgType_S16;
        break;
    case MmlCommand::Mml_TimeRandom:
        cmd = ReadByte(&trackParam.currentAddr);
        argType2 = SeqArgType_Random;
        break;
    case MmlCommand::Mml_TimeVariable:
        cmd = ReadByte(&trackParam.currentAddr);
        argType2 = SeqArgType_Variable;
        break;
    default:
        break;
    }

    switch (cmd) {
    case MmlCommand::Mml_Random:
        cmd = ReadByte(&trackParam.currentAddr);
        argType = SeqArgType_Random;
        useArgType = true;
        break;
    case MmlCommand::Mml_Variable:
        cmd = ReadByte(&trackParam.currentAddr);
        argType = SeqArgType_Variable;
        useArgType = true;
        break;
    default:
        break;
    }

    if ((cmd & 0b10000000) == 0) {
        const uint8_t velocity{ReadByte(&trackParam.currentAddr)};
        const int32_t length{
            ReadArg(&trackParam.currentAddr, player, track,
                    useArgType ? argType : SeqArgType_Vmidi),
        };
        int32_t key{static_cast<int32_t>(cmd) + trackParam.transpose};

        if (doExecCommand) {
            key = fnd::Clamp(key, 0, 127);
            if (!trackParam.muteFlag && doNoteOn)
                NoteOnCommandProc(track, key, velocity, length > 0 ? length : -1,
                                  trackParam.tieFlag);

            if (trackParam.noteWaitFlag) {
                trackParam.wait = length;
                if (trackParam.wait == 0)
                    trackParam.noteFinishWait = true;
            }
        }
    } else {
        int32_t commandArg1{0};
        int32_t commandArg2{0};
        switch (cmd & 0b11110000) {
        case 0b10000000: {
            switch (cmd) {
            case MmlCommand::Mml_Wait: {
                int32_t arg{
                    ReadArg(&trackParam.currentAddr, player, track,
                            useArgType ? argType : SeqArgType_Vmidi),
                };
                if (doExecCommand)
                    trackParam.wait = arg;
                break;
            }
            case MmlCommand::Mml_Prg: {
                commandArg1 = ReadArg(&trackParam.currentAddr, player, track,
                                      useArgType ? argType : SeqArgType_Vmidi);
                if (doExecCommand)
                    CommandProc(track, cmd, commandArg1, commandArg2);
                break;
            }
            case MmlCommand::Mml_OpenTrack: {
                uint8_t trackNo{ReadByte(&trackParam.currentAddr)};
                uint32_t offset{Read24(&trackParam.currentAddr)};
                if (doExecCommand) {
                    commandArg1 = trackNo;
                    commandArg2 = static_cast<position_t>(offset);
                    CommandProc(track, cmd, commandArg1, commandArg2);
                }
                break;
            }
            case MmlCommand::Mml_Jump: {
                uint32_t offset{Read24(&trackParam.currentAddr)};
                if (doExecCommand) {
                    commandArg1 = static_cast<position_t>(offset);
                    CommandProc(track, cmd, commandArg1, commandArg2);
                }
                break;
            }
            case MmlCommand::Mml_Call: {
                uint32_t offset{Read24(&trackParam.currentAddr)};
                if (doExecCommand) {
                    commandArg1 = static_cast<position_t>(offset);
                    CommandProc(track, cmd, commandArg1, commandArg2);
                }
                break;
            }
            default:
                break;
            }
            break;
        }
        case 0b10110000:
        case 0b11000000:
        case 0b11010000: {
            uint8_t arg{
                static_cast<uint8_t>(ReadArg(&trackParam.currentAddr, player, track,
                                             useArgType ? argType : SeqArgType_U8)),
            };
            if (argType2 != SeqArgType_None)
                commandArg2 = ReadArg(&trackParam.currentAddr, player, track, argType2);

            if (doExecCommand) {
                switch (cmd) {
                case MmlCommand::Mml_Transpose:
                case MmlCommand::Mml_PitchBend:
                    commandArg1 = *reinterpret_cast<int8_t*>(&arg);
                    break;
                default:
                    commandArg1 = arg;
                    break;
                }

                CommandProc(track, cmd, commandArg1, commandArg2);
            }
            break;
        }

        case 0b10010000: {
            if (doExecCommand)
                CommandProc(track, cmd, commandArg1, commandArg2);
            break;
        }
        case 0b11100000: {
            commandArg1 = static_cast<int16_t>(ReadArg(&trackParam.currentAddr, player, track,
                                                       useArgType ? argType : SeqArgType_S16));
            if (doExecCommand)
                CommandProc(track, cmd, commandArg1, commandArg2);
            break;
        }
        case 0b11110000: {
            switch (cmd) {
            case MmlCommand::Mml_AllocTrack: {
                Read16(&trackParam.currentAddr);
                break;
            }
            case MmlCommand::Mml_Fin: {
                if (doExecCommand)
                    return SequenceTrack::ParseResult_Finish;
                break;
            }
            case MmlCommand::Mml_ExCommand: {
                uint32_t cmdex{ReadByte(&trackParam.currentAddr)};
                switch (cmdex & 0b11110000) {
                case 0b10100000:
                case 0b10110000: {
                    commandArg1 = ReadByte(&trackParam.currentAddr);
                    if (doExecCommand)
                        CommandProc(track, (cmd << 8) + cmdex, commandArg1, commandArg2);
                    break;
                }
                case 0b11100000: {
                    commandArg1 =
                        static_cast<uint16_t>(ReadArg(&trackParam.currentAddr, player, track,
                                                      useArgType ? argType : SeqArgType_S16));
                    if (doExecCommand)
                        CommandProc(track, (cmd << 8) + cmdex, commandArg1, commandArg2);
                    break;
                }
                case 0b10010000:
                case 0b10000000: {
                    commandArg1 = ReadByte(&trackParam.currentAddr);
                    commandArg2 =
                        static_cast<int16_t>(ReadArg(&trackParam.currentAddr, player, track,
                                                     useArgType ? argType : SeqArgType_S16));

                    if (doExecCommand)
                        CommandProc(track, (cmd << 8) + cmdex, commandArg1, commandArg2);
                    break;
                }
                default:
                    break;
                }
                break;
            }
            default: {
                if (doExecCommand)
                    CommandProc(track, cmd, commandArg1, commandArg2);
                break;
            }
            }
            break;
        }
        default:
            break;
        }
    }

    return SequenceTrack::ParseResult_Continue;
}

// TODO: MmlParser::CommandProc

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
