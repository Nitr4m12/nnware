#pragma once

#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_WaveSound.h>

namespace nn::atk {

class WaveSoundHandle {
public:
    using TrackBitFlagSet = util::BitFlagSet<8, void>;

    WaveSoundHandle() = default;
    explicit WaveSoundHandle(SoundHandle* handle);

    ~WaveSoundHandle() = default;

    void StartPrepared() {
        if (IsAttachedSound())
            m_pSound->StartPrepared();
    }

    void Stop(int fadeFrames) {
        if (IsAttachedSound())
            m_pSound->Stop(fadeFrames);
    }

    void Pause(bool flag, int fadeFrames) {
        if (IsAttachedSound())
            m_pSound->Pause(flag, fadeFrames);
    }

    void Pause(bool flag, int fadeFrames, PauseMode pauseMode) {
        if (IsAttachedSound())
            m_pSound->Pause(flag, fadeFrames, pauseMode);
    }

    bool IsPrepared() const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->IsPrepared();
    }

    bool IsPause() const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->IsPause();
    }

    void FadeIn(int frames) {
        if (IsAttachedSound())
            m_pSound->FadeIn(frames);
    }

    void SetVolume(float volume, int frames) {
        if (IsAttachedSound())
            m_pSound->SetVolume(volume, frames);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetVolumeThroughMode(int bus, uint8_t modeBitFlag) {
        if (IsAttachedSound())
            m_pSound->SetVolumeThroughMode(bus, modeBitFlag);
    }
#endif

    void SetPitch(float pitch) {
        if (IsAttachedSound())
            m_pSound->SetPitch(pitch);
    }

    void SetLowPassFilterFrequency(float lpfFreq) {
        if (IsAttachedSound())
            m_pSound->SetLpfFreq(lpfFreq);
    }

    void SetPlayerPriority(int priority) {
        if (IsAttachedSound())
            m_pSound->SetPlayerPriority(priority);
    }

    void SetChannelPriority(int32_t priority) {
        if (IsAttachedSound())
            m_pSound->SetChannelPriority(priority);
    }

    void SetPan(float pan) {
        if (IsAttachedSound())
            m_pSound->SetPan(pan);
    }

    void SetSurroundPan(float span) {
        if (IsAttachedSound())
            m_pSound->SetSurroundPan(span);
    }

    void SetMixMode(MixMode mixMode) {
        if (IsAttachedSound())
            m_pSound->SetMixMode(mixMode);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetMixVolume(int32_t subMixBus, const MixVolume& mixVolume) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, subMixBus,
                                            {mixVolume.channel, 6});
    }

    void SetBusMixVolumeEnabled(int32_t subMixBus, bool isEnabled) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolumeEnabled(OutputDevice_Main, subMixBus, isEnabled);
    }

    void SetBusMixVolume(int32_t srcChNo, int32_t subMixBus, const ChannelMixVolume& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, srcChNo, subMixBus, param);
    }

    void SetMixVolume(const MixVolume& mixVolume) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, 0, {mixVolume.channel, 6});
    }

    void SetBusMixVolume(int32_t subMixBus, const ChannelMixVolume& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, subMixBus, param);
    }
#endif

    void SetMainSend(float send) {
        if (IsAttachedSound())
            m_pSound->SetMainSend(send);
    }

    void SetFxSend(AuxBus bus, float send) {
        if (IsAttachedSound())
            m_pSound->SetFxSend(bus, send);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetSend(int bus, float send) {
        if (IsAttachedSound())
            m_pSound->SetSend(bus, send);
    }
#endif

    bool IsAttachedSound() const { return m_pSound != nullptr; }

    void DetachSound();

    void SetId(uint32_t id) {
        if (IsAttachedSound())
            m_pSound->SetId(id);
    }

    uint32_t GetId() const {
        if (!IsAttachedSound())
            return InvalidSoundId;

        return m_pSound->GetId();
    }

    const SoundParam* GetAmbientParam() const {
        if (!IsAttachedSound())
            return nullptr;

        return &m_pSound->GetAmbientParam();
    }

    bool ReadWaveSoundDataInfo(WaveSoundDataInfo* info) {
        if (!IsAttachedSound())
            return false;

        return ReadWaveSoundDataInfo(info);
    }

    position_t GetPlaySamplePosition(bool isOriginalSamplePosition) const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetPlaySamplePosition(isOriginalSamplePosition);
    }

    uint32_t GetChannelCount() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetChannelCount();
    }

    void SetChannelMixParameter(uint32_t srcChNo, const MixParameter& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputChannelMixParameter(OutputDevice_Main, srcChNo, param);
    }

    void SetChannelMixParameter(const MixParameter& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputChannelMixParameter(OutputDevice_Main, 0, param);
    }

    void SetOutputChannelMixParameter(OutputDevice device, uint32_t srcChNo,
                                      const MixParameter& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputChannelMixParameter(device, srcChNo, param);
    }

    void SetOutputChannelMixParameter(OutputDevice device, const MixParameter& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputChannelMixParameter(device, 0, param);
    }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetProcessTick(profile);
    }

    void detail_AttachSoundAsTempHandle(detail::WaveSound* pSound);

    detail::WaveSound* detail_GetAttachedSound() { return m_pSound; }
    const detail::WaveSound* detail_GetAttachedSound() const { return m_pSound; }

    void ForceStop();

private:
    NN_NO_COPY(WaveSoundHandle);

    detail::WaveSound* m_pSound{};
};

}  // namespace nn::atk
