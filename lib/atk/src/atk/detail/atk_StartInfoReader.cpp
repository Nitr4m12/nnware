#include <nn/atk/atk_StartInfoReader.h>

namespace nn::atk::detail {

StartInfoReader::StartInfoReader(const SoundArchive::SoundInfo& soundInfo)
    : m_PlayerPriority{soundInfo.playerPriority}, m_PlayerId{soundInfo.playerId},
      m_ActorPlayerId{soundInfo.actorPlayerId} {}

}  // namespace nn::atk::detail
