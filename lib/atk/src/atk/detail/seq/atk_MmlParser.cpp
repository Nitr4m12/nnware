#include <nn/atk/atk_MmlParser.h>

#include <nn/atk/atk_SequenceSoundPlayer.h>

namespace nn::atk::detail::driver {

namespace {

const float ModSpeedBase{1 / 2.56};

}  // anonymous namespace

// TODO: SequenceTrack::ParseResult MmlParser::Parse

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

}  // namespace nn::atk::detail::driver
