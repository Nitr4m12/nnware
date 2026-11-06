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

    const detail::SoundActorParam& GetActorParam() const { return m_ActorParam; }

private:
    uint8_t _0[0x90];
    ActorPlayer* m_ActorPlayerList[4];
    detail::SoundActorParam m_ActorParam;
};

}  // namespace nn::atk
