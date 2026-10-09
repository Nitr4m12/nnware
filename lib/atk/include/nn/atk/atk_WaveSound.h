#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_Debug.h>
#include <nn/atk/atk_SoundInstanceManager.h>
#include <nn/atk/atk_WaveSoundPlayer.h>

namespace nn::atk {

class WaveSoundHandle;

namespace detail {

class WaveSound;
using WaveSoundInstanceManager = SoundInstanceManager<WaveSound>;

class WaveSound : public BasicSound {
    NN_ATK_RTTI_OVERRIDE(WaveSound, BasicSound)

public:
    explicit WaveSound(WaveSoundInstanceManager& manager);

    void Prepare(const void* wsdFile, const void* waveFile,
                 const driver::WaveSoundPlayer::StartInfo& startInfo, int8_t waveType);

    void RegisterDataLoadTask(const driver::WaveSoundLoader::LoadInfo& loadInfo,
                              const driver::WaveSoundPlayer::StartInfo& startInfo);

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    bool IsPrepared() const override {
        if (m_IsCalledPrepare)
            return true;

        if (!IsPlayerAvailable())
            return false;

        return m_PlayerInstance.IsPrepared();
    }

    void InitializeChannelParam(int32_t priority, bool isReleasePriorityFix);
    void SetChannelPriority(int32_t priority);

    bool ReadWaveSoundDataInfo(WaveSoundDataInfo* info) const;

    position_t GetPlaySamplePosition(bool isOriginalSamplePosition) const;

    uint32_t GetChannelCount() const { return m_ChannelCount; }

    void SetLoaderManager(driver::WaveSoundLoaderManager& manager) {
        if (IsPlayerAvailable())
            m_PlayerInstance.SetLoaderManager(&manager);
    }

    DebugSoundType GetSoundType() const { return DebugSoundType_Wavesound; }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsPlayerAvailable())
            return 0;

        return m_PlayerInstance.GetProcessTick(profile);
    }

    util::IntrusiveListNode m_PriorityLink;

private:
    friend WaveSoundHandle;

    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;
    void OnUpdatePlayerPriority() override;

    void OnUpdateParam() override {}
    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override { return &m_PlayerInstance; }

    WaveSoundHandle* m_pTempSpecialHandle;
    WaveSoundInstanceManager& m_Manager;
    const void* m_pWaveFile;
    int8_t m_WaveType;
    bool m_InitializeFlag{false};
    bool m_IsCalledPrepare{false};
    uint8_t m_Padding[1];
    uint32_t m_ChannelCount{0};
    driver::WaveSoundPlayer m_PlayerInstance;
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(WaveSound) == 0x3b0);
#else
static_assert(sizeof(WaveSound) == 0x3e0);
#endif

}  // namespace detail
}  // namespace nn::atk
