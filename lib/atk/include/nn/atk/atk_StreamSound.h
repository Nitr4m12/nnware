#pragma once

#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_SoundInstanceManager.h>
#include <nn/atk/atk_StreamSoundPlayer.h>

namespace nn::atk {

class StreamSoundHandle;

namespace detail {

class StreamSound;
using StreamSoundInstanceManager = SoundInstanceManager<StreamSound>;

class StreamSound : public BasicSound {
    NN_ATK_RTTI_OVERRIDE(StreamSound, BasicSound)

public:
    explicit StreamSound(const StreamSoundInstanceManager& manager);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    void Setup(const driver::StreamSoundPlayer::SetupArg& arg);

    void Prepare(const driver::StreamSoundPlayer::PrepareBaseArg& arg);
    void PreparePrefetch(const void* strmPrefetchFile,
                         const driver::StreamSoundPlayer::PrepareBaseArg& arg);

    void UpdateMoveValue() override;

    void OnUpdateParam() override;

    void SetTrackVolume(uint32_t trackBitFlag, float volume, int32_t);
    void SetTrackInitialVolume(uint32_t trackBitFlag, uint32_t volume);

    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine);
    void ResetTrackOutputLine(uint32_t trackBitFlag);

    void SetTrackMainOutVolume(uint32_t trackBitFlag, float volume);
    void SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                     const MixParameter& param);
    void SetTrackPan(uint32_t trackBitFlag, float pan);
    void SetTrackSurroundPan(uint32_t trackBitFlag, float span);
    void SetTrackMainSend(uint32_t trackBitFlag, float send);
    void SetTrackFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

    void OnUpdatePlayerPriority() override;

    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;

    bool ReadStreamDataInfo(StreamDataInfo*) const;

    int32_t GetPlayLoopCount() const;
    position_t GetPlaySamplePosition(bool) const;
    float GetFilledBufferPercentage() const;
    int32_t GetBufferBlockCount(WaveBuffer::Status waveBufferStatus) const;
    int32_t GetTotalBufferBlockCount() const;

    bool IsPrepared() const override;
    bool IsSuspendByLoadingDelay() const;
    bool IsLoadingDelayState() const;

    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override;

private:
    friend StreamSoundInstanceManager;

    util::IntrusiveListNode m_PriorityLink;
    StreamSoundHandle* m_pTempSpecialHandle;
    StreamSoundInstanceManager* m_Manager;
    MoveValue<float, int32_t> m_TrackVolume[8];
    uint16_t m_AllocTrackFlag;
    bool m_InitializeFlag;
    uint8_t m_Padding[1];
    uint32_t m_AvailableTrackBitFlag[2];
    void* m_pCacheBuffer;
    size_t m_CacheSize;
    driver::StreamSoundPlayer m_PlayerInstance;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(StreamSound) == 0x11a00);
#else
static_assert(sizeof(StreamSound) == 0x11a40);
#endif

}  // namespace detail
}  // namespace nn::atk
