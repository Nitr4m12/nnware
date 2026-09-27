#include <nn/atk/atk_Debug.h>

#include <cstdint>

namespace nn::atk {
namespace {

const uint32_t NotEnoughSeqSound{0b00001};
const uint32_t NotEnoughStrmSound{0b00010};
const uint32_t NotEnoughWaveSound{0b00100};
const uint32_t NotEnoughSeqTrack{0b01000};
const uint32_t NotEnoughStrmChannel{0b10000};
const uint32_t NotEnoughInstance{0b11111};

uint32_t gWarningFlag = NotEnoughInstance;

uint32_t GetWarningBitFlag(DebugWarningFlag warning) {
    uint32_t bitFlag{0};

    switch (warning) {
    case DebugWarningFlag_NotEnoughInstance:
        bitFlag = NotEnoughInstance;
        break;

    case DebugWarningFlag_NotEnoughSeqsound:
        bitFlag = NotEnoughSeqSound;
        break;

    case DebugWarningFlag_NotEnoughStrmsound:
        bitFlag = NotEnoughStrmSound;
        break;

    case DebugWarningFlag_NotEnoughWavesound:
        bitFlag = NotEnoughWaveSound;
        break;

    case DebugWarningFlag_NotEnoughSeqtrack:
        bitFlag = NotEnoughSeqTrack;
        break;

    case DebugWarningFlag_NotEnoughStrmchannel:
        bitFlag = NotEnoughStrmChannel;
        break;
    }

    return bitFlag;
}

}  // anonymous namespace

void Debug_SetWarningFlag(DebugWarningFlag warning, bool enable) {
    uint32_t bitFlag{GetWarningBitFlag(warning)};

    if (enable)
        gWarningFlag |= bitFlag;
    else
        gWarningFlag &= ~bitFlag;
}

namespace detail {

DebugLogFunc g_DebugLogHookFunc;

bool Debug_GetWarningFlag(DebugWarningFlag warning) {
    uint32_t bitFlag{GetWarningBitFlag(warning)};

    return (bitFlag & ~gWarningFlag) == 0;
}

DebugWarningFlag Debug_GetDebugWarningFlagFromSoundType(DebugSoundType type) {
    switch (type) {
    case DebugSoundType_Seqsound:
        return DebugWarningFlag_NotEnoughSeqsound;

    case DebugSoundType_Strmsound:
        return DebugWarningFlag_NotEnoughStrmsound;

    case DebugSoundType_Wavesound:
        return DebugWarningFlag_NotEnoughWavesound;

    default:
        return DebugWarningFlag_NotEnoughSeqsound;
    }
}

const char* Debug_GetSoundTypeString(DebugSoundType type) {
    switch (type) {
    case DebugSoundType_Seqsound:
        return "seq";

    case DebugSoundType_Strmsound:
        return "strm";

    case DebugSoundType_Wavesound:
        return "wave";

    default:
        return "";
    }
}

}  // namespace detail
}  // namespace nn::atk
