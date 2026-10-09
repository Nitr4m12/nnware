#pragma once

#include <nn/atk/atk_Debug.h>
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
    explicit StreamSound(StreamSoundInstanceManager& manager);

    void SetCacheBuffer(void* cacheBuffer, size_t cacheSize) {
        m_pCacheBuffer = cacheBuffer;
        m_CacheSize = cacheSize;
    }

    bool IsCacheAvailable() const { return m_pCacheBuffer != nullptr; }

    void* GetCacheBuffer() { return m_pCacheBuffer; }
    size_t GetCacheSize() { return m_CacheSize; }

    void Setup(const driver::StreamSoundPlayer::SetupArg& arg);
    void Prepare(const driver::StreamSoundPlayer::PrepareBaseArg& arg);
    void PreparePrefetch(const void* strmPrefetchFile,
                         const driver::StreamSoundPlayer::PrepareBaseArg& arg);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    bool IsPrepared() const override;
    bool IsSuspendByLoadingDelay() const;
    bool IsLoadingDelayState() const;

    void SetTrackVolume(uint32_t trackBitFlag, float volume, int32_t frames);
    void SetTrackInitialVolume(uint32_t trackBitFlag, uint32_t volume);
    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t lineFlag);
    void ResetTrackOutputLine(uint32_t trackBitFlag);
    void SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                     const MixParameter& mixParam);
    void SetTrackMainOutVolume(uint32_t trackBitFlag, float volume);
    void SetTrackPan(uint32_t trackBitFlag, float pan);
    void SetTrackSurroundPan(uint32_t trackBitFlag, float span);
    void SetTrackMainSend(uint32_t trackBitFlag, float send);
    void SetTrackFxSend(uint32_t trackBitFlag, AuxBus bus, float send);

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    bool ReadStreamDataInfo(StreamDataInfo* info) const;
#else
    bool ReadStreamSoundDataInfo(StreamSoundDataInfo* info) const;
#endif

    int64_t GetPlayLoopCount() const;
    position_t GetPlaySamplePosition(bool isOriginalSamplePosition) const;

    uint32_t GetAvailableTrackBitFlag(uint32_t channel) { return m_AvailableTrackBitFlag[channel]; }

    float GetFilledBufferPercentage() const;
    int32_t GetBufferBlockCount(WaveBuffer::Status status) const;
    int32_t GetTotalBufferBlockCount() const;

    int32_t GetActiveChannelCount() const { return m_PlayerInstance.GetActiveChannelCount(); }
    int32_t GetActiveTrackCount() const { return m_PlayerInstance.GetActiveTrackCount(); }

    DebugSoundType GetSoundType() const { return DebugSoundType_Strmsound; }

    void SetLoaderManager(driver::StreamSoundLoaderManager& manager) {
        if (IsPlayerAvailable())
            m_PlayerInstance.SetLoaderManager(&manager);
    }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsPlayerAvailable())
            return 0;

        return m_PlayerInstance.GetProcessTick(profile);
    }

    void* detail_SetFsAccessLog(fnd::FsAccessLog* fsAccessLog) {
        if (!IsPlayerAvailable())
            return nullptr;

        return m_PlayerInstance.detail_SetFsAccessLog(fsAccessLog);
    }

    util::IntrusiveListNode m_PriorityLink;

protected:
    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;

    void UpdateMoveValue() override;
    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override { return &m_PlayerInstance; }

    void OnUpdatePlayerPriority() override;

private:
    friend StreamSoundHandle;

    void OnUpdateParam() override;

    StreamSoundHandle* m_pTempSpecialHandle;
    StreamSoundInstanceManager& m_Manager;
    MoveValue<float, int32_t> m_TrackVolume[StreamTrackCount];
    uint16_t m_AllocTrackFlag;
    bool m_InitializeFlag{false};
    uint8_t m_Padding[1];
    uint32_t m_AvailableTrackBitFlag[WaveChannelMax];
    void* m_pCacheBuffer{};
    size_t m_CacheSize{0};
    driver::StreamSoundPlayer m_PlayerInstance;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(StreamSound) == 0x11a00);
#else
static_assert(sizeof(StreamSound) == 0x11a40);
#endif

}  // namespace detail
}  // namespace nn::atk
