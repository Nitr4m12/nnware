#pragma once

#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_StreamSound.h>

namespace nn::atk {

class StreamSoundHandle {
public:
    using TrackBitFlagSet = util::BitFlagSet<8, void>;

    StreamSoundHandle() = default;
    explicit StreamSoundHandle(SoundHandle* handle);

    ~StreamSoundHandle() = default;

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

    bool IsSuspendByLoadingDelay() const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->IsSuspendByLoadingDelay();
    }

    bool IsLoadingDelayState() const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->IsLoadingDelayState();
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
    void SetMixVolume(int subMixBus, const MixVolume& mixVolume) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, subMixBus,
                                            {mixVolume.channel, 6});
    }

    void SetBusMixVolumeEnabled(int subMixBus, bool isEnabled) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolumeEnabled(OutputDevice_Main, subMixBus, isEnabled);
    }

    void SetBusMixVolume(int srcChNo, int subMixBus, const ChannelMixVolume& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, srcChNo, subMixBus, param);
    }

    void SetMixVolume(const MixVolume& mixVolume) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, 0, {mixVolume.channel, 6});
    }

    void SetBusMixVolume(int subMixBus, const ChannelMixVolume& param) {
        if (IsAttachedSound())
            m_pSound->SetOutputBusMixVolume(OutputDevice_Main, 0, subMixBus, param);
    }

    void SetTrackMixVolume(TrackBitFlagSet, int32_t, const MixVolume&);  // TODO
    void SetTrackMixVolume(const MixVolume&);                            // TODO
#endif

    void SetMainSend(float send) {
        if (IsAttachedSound())
            m_pSound->SetMainSend(send);
    }

    void SetEffectSend(AuxBus bus, float send) {
        if (IsAttachedSound())
            m_pSound->SetFxSend(bus, send);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetSend(int bus, float send) {
        if (IsAttachedSound())
            m_pSound->SetSend(bus, send);
    }
#endif

    void SetTrackVolume(uint32_t trackBitFlag, float volume, int32_t frames) {
        if (IsAttachedSound())
            m_pSound->SetTrackVolume(trackBitFlag, volume, frames);
    }

    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t lineFlag) {
        if (IsAttachedSound())
            m_pSound->SetTrackOutputLine(trackBitFlag, lineFlag);
    }

    void ResetTrackOutputLine(uint32_t trackBitFlag) {
        if (IsAttachedSound())
            m_pSound->ResetTrackOutputLine(trackBitFlag);
    }

    void SetTrackOutputVolume(OutputDevice, uint32_t, float);              // TODO
    void SetTrackOutputPan(OutputDevice, uint32_t, float);                 // TODO
    void SetTrackOutputSurroundPan(OutputDevice, uint32_t, float);         // TODO
    void SetTrackOutputMainSend(OutputDevice, uint32_t, float);            // TODO
    void SetTrackOutputEffectSend(OutputDevice, uint32_t, AuxBus, float);  // TODO

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

#if NN_WARE_VER < NN_MAKE_VER(3, 0, 0)
    bool ReadStreamDataInfo(StreamDataInfo* info) const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->ReadStreamDataInfo(info);
    }
#else
    bool ReadStreamSoundDataInfo(StreamSoundDataInfo* info) const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->ReadStreamSoundDataInfo(info);
    }

    bool ReadStreamDataInfo(StreamSoundDataInfo* info) const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->ReadStreamSoundDataInfo(info);
    }
#endif

    int32_t GetPlayLoopCount() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetPlayLoopCount();
    }

    int64_t GetPlaySamplePosition() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetPlaySamplePosition(false);
    }

    float GetFilledBufferPercentage() const {
        if (!IsAttachedSound())
            return 0.0f;

        return m_pSound->GetFilledBufferPercentage();
    }

    int32_t GetBufferBlockCount(WaveBuffer::Status status) const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetBufferBlockCount(status);
    }

    int32_t GetTotalBufferBlockCount() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetTotalBufferBlockCount();
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

    void SetTrackChannelMixParameter(uint32_t trackBitFlag, uint32_t srcChNo,
                                     const MixParameter& mixParam) {
        if (IsAttachedSound())
            m_pSound->SetTrackChannelMixParameter(trackBitFlag, srcChNo, mixParam);
    }

    void SetTrackChannelMixParameter(const MixParameter& mixParam) {
        if (IsAttachedSound())
            m_pSound->SetTrackChannelMixParameter(1, 0, mixParam);
    }

    void SetTrackChannelOutputMixParameter(OutputDevice, uint32_t, uint32_t,
                                           const MixParameter&);                // TODO
    void SetTrackChannelOutputMixParameter(OutputDevice, const MixParameter&);  // TODO

    uint32_t GetAvailableTrackBitFlag(uint32_t channel) const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetAvailableTrackBitFlag(channel);
    }

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetProcessTick(profile);
    }

    void detail_AttachSoundAsTempHandle(detail::StreamSound* pSound);

    detail::StreamSound* detail_GetAttachedSound() { return m_pSound; }
    const detail::StreamSound* detail_GetAttachedSound() const { return m_pSound; }

    void* detail_SetFsAccessLog(detail::fnd::FsAccessLog* fsAccessLog) {
        if (!IsAttachedSound())
            return nullptr;

        return m_pSound->detail_SetFsAccessLog(fsAccessLog);
    }

    bool HasStreamSoundPlayer(const detail::driver::StreamSoundPlayer* player) const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->GetBasicSoundPlayerHandle() == player;
    }

private:
    NN_NO_COPY(StreamSoundHandle);

    detail::StreamSound* m_pSound{};
};

}  // namespace nn::atk
