#include <nn/atk/atk_StartInfoReader.h>

namespace nn::atk::detail {

StartInfoReader::StartInfoReader(const SoundArchive::SoundInfo& soundInfo)
    : m_PlayerPriority{soundInfo.playerPriority}, m_PlayerId{soundInfo.playerId},
      m_ActorPlayerId{soundInfo.actorPlayerId} {}

void StartInfoReader::Read(const SoundStartable::StartInfo* startInfo) {
    if (startInfo == nullptr)
        return;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_StartOffset) {
        m_StartOffsetType = startInfo->startOffsetType;
        m_StartOffset = startInfo->startOffset;
    }

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_PlayerPriority)
        m_PlayerPriority = startInfo->playerPriority;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_PlayerId)
        m_PlayerId = startInfo->playerId;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_ActorPlayerId)
        m_ActorPlayerId = startInfo->actorPlayerId;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_SequenceSoundInfo)
        m_pSeqInfo = &startInfo->sequenceSoundInfo;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_StreamSoundInfo)
        m_pStrmInfo = &startInfo->streamSoundInfo;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_StreamSoundMetaInfo)
        m_pStrmMetaInfo = &startInfo->streamSoundMetaInfo;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_StreamSoundMetaInfo2)
        m_pStrmMetaInfo2 = &startInfo->streamSoundMetaInfo2;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_WaveSoundInfo)
        m_pWsdInfo = &startInfo->waveSoundInfo;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_DelayTime)
        m_DelayTime = startInfo->delayTime;

    if ((startInfo->enableFlag & (SoundStartable::StartInfo::EnableFlagBit_DelayCount |
                                  SoundStartable::StartInfo::EnableFlagBit_DelayTime)) ==
        SoundStartable::StartInfo::EnableFlagBit_DelayCount)
        m_DelayCount = startInfo->delayCount;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_UpdateType)
        m_UpdateType = startInfo->updateType;

    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_SubMixIndex)
        m_SubMixIndex = startInfo->subMixIndex;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    if (startInfo->enableFlag & SoundStartable::StartInfo::EnableFlagBit_OutputReceiver)
        m_pOutputReceiver = startInfo->pOutputReceiver;
#endif
}

}  // namespace nn::atk::detail
