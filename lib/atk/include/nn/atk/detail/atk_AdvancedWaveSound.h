#pragma once

#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_Debug.h>
#include <nn/atk/atk_SoundInstanceManager.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundHandle.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundPlayer.h>

namespace nn::atk::detail {

class AdvancedWaveSound;

using AdvancedWaveSoundInstanceManager = SoundInstanceManager<AdvancedWaveSound>;

class AdvancedWaveSound : public BasicSound {
public:
    explicit AdvancedWaveSound(AdvancedWaveSoundInstanceManager& manager);
    ~AdvancedWaveSound() override;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool Initialize() override;
#else
    bool Initialize(OutputReceiver* pOutputReceiver) override;
#endif
    void Finalize() override;

    bool IsPrepared() const override;

    void Prepare(const driver::AdvancedWaveSoundPlayer::PrepareParameter& parameter);

    DebugSoundType GetSoundType() const { return DebugSoundType_Wavesound; }

private:
    bool IsAttachedTempSpecialHandle() override;
    void DetachTempSpecialHandle() override;

    void OnUpdatePlayerPriority() override;

    driver::BasicSoundPlayer* GetBasicSoundPlayerHandle() override;

public:
    util::IntrusiveListNode m_PriorityLink;

private:
    AdvancedWaveSoundHandle* m_pTempSpecialHandle;
    AdvancedWaveSoundInstanceManager& m_InstanceManager;
    driver::AdvancedWaveSoundPlayer m_PlayerInstance;
    bool m_IsInitialized{false};
    uint8_t m_Padding[3];
};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
static_assert(sizeof(AdvancedWaveSound) == 0x980);
#else
static_assert(sizeof(AdvancedWaveSound) == 0x9b0);
#endif

}  // namespace nn::atk::detail
