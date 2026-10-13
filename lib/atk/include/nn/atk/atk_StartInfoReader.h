#pragma once

#include <nn/atk/atk_SoundStartable.h>

namespace nn::atk::detail {

class StartInfoReader {
public:
    explicit StartInfoReader(const SoundArchive::SoundInfo& soundInfo);

    void Read(const SoundStartable::StartInfo* startInfo);

    auto GetStartOffsetType() const { return m_StartOffsetType; }
    int32_t GetStartOffset() const { return m_StartOffset; }
    int32_t GetDelayTime() const { return m_DelayTime; }
    int32_t GetDelayCount() const { return m_DelayCount; }
    UpdateType GetUpdateType() const { return m_UpdateType; }
    int32_t GetPlayerPriority() const { return m_PlayerPriority; }
    SoundArchive::ItemId GetPlayerId() const { return m_PlayerId; }
    int32_t GetActorPlayerId() const { return m_ActorPlayerId; }
    const auto* GetSeqInfo() const { return m_pSeqInfo; }
    const auto* GetStrmInfo() const { return m_pStrmInfo; }
    const auto* GetStrmMetaInfo() const { return m_pStrmMetaInfo; }
    const auto* GetStrmMetaInfo2() const { return m_pStrmMetaInfo2; }
    const auto* GetWsdInfo() const { return m_pWsdInfo; }
    int32_t GetSubMixIndex() const { return m_SubMixIndex; }
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* GetOutputReceiver() const { return m_pOutputReceiver; }
#endif

private:
    SoundStartable::StartInfo::StartOffsetType m_StartOffsetType;
    int32_t m_StartOffset;
    int32_t m_DelayTime;
    int32_t m_DelayCount;
    UpdateType m_UpdateType;
    int32_t m_PlayerPriority;
    SoundArchive::ItemId m_PlayerId;
    int32_t m_ActorPlayerId;
    SoundStartable::StartInfo::SequenceSoundInfo* m_pSeqInfo;
    SoundStartable::StartInfo::StreamSoundInfo* m_pStrmInfo;
    SoundArchive::StreamSoundInfo* m_pStrmMetaInfo;
    SoundArchive::StreamSoundInfo2* m_pStrmMetaInfo2;
    SoundStartable::StartInfo::WaveSoundInfo* m_pWsdInfo;
    int32_t m_SubMixIndex;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    OutputReceiver* m_pOutputReceiver;
#endif
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(StartInfoReader) == 0x50);
#else
static_assert(sizeof(StartInfoReader) == 0x58);
#endif

}  // namespace nn::atk::detail
