#pragma once

#include <nn/atk/atk_ExternalSoundPlayer.h>

namespace nn::atk {

class SoundActor {
public:
    using ActorPlayer = detail::ExternalSoundPlayer;

    ActorPlayer* detail_GetActorPlayer(int actorPlayerId) {
        if (static_cast<uint32_t>(actorPlayerId) > 3)
            return nullptr;

        return m_ActorPlayerList[actorPlayerId];
    }

private:
    uint8_t _0[0x90];
    ActorPlayer* m_ActorPlayerList[4];
    float m_Volume;
    float m_Pitch;
    float m_TvVolume;
    float m_TvPan;
    float m_Lpf;
    int32_t m_BiquadFilterType;
    float m_BiquadFilterValue;
};

}  // namespace nn::atk
