#pragma once

#include <nn/atk/atk_Channel.h>

namespace nn::atk::detail::driver {

class SequenceSoundPlayer;

struct NoteOnInfo {
    int prgNo;
    int key;
    int velocity;
    int length;
    int initPan;
    int priority;
    Channel::ChannelCallback channelCallback;
    void* channelCallbackData;
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    UpdateType updateType;
    int subMixIndex;
#else
    OutputReceiver* pOutputReceiver;
    UpdateType updateType;
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(NoteOnInfo) == 0x30);
#else
static_assert(sizeof(NoteOnInfo) == 0x38);
#endif

class NoteOnCallback {
public:
    virtual ~NoteOnCallback() = default;

    virtual Channel* NoteOn(SequenceSoundPlayer* seqPlayer, uint8_t bankIndex,
                            const NoteOnInfo& noteOnInfo) = 0;
};
static_assert(sizeof(NoteOnCallback) == 0x8);

}  // namespace nn::atk::detail::driver
