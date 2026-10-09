#pragma once

#include <nn/atk/atk_SequenceSound.h>
#include <nn/atk/atk_SoundHandle.h>

namespace nn::atk {

class SequenceSoundHandle {
public:
    static const int32_t BankIndexMin{detail::SequenceSound::BankIndexMin};
    static const uint32_t BankIndexMax{detail::SequenceSound::BankIndexMax};

    static const int8_t TransposeMin{detail::SequenceSound::TransposeMin};
    static const int8_t TransposeMax{detail::SequenceSound::TransposeMax};

    static const uint8_t VelocityRangeMin{detail::SequenceSound::VelocityRangeMin};
    static const uint8_t VelocityRangeMax{detail::SequenceSound::VelocityRangeMax};

    static const int32_t VariableIndexMax{15};
    static const int32_t TrackIndexMax{15};

    using TrackBitFlagSet = util::BitFlagSet<16, void>;

    SequenceSoundHandle() = default;
    explicit SequenceSoundHandle(SoundHandle* handle);

    ~SequenceSoundHandle();

    void StartPrepared() {
        if (IsAttachedSound())
            m_pSound->StartPrepared();
    }

    void Stop(int32_t fadeFrames) {
        if (IsAttachedSound())
            m_pSound->Stop(fadeFrames);
    }

    void Pause(bool flag, int32_t fadeFrames) {
        if (IsAttachedSound())
            m_pSound->Pause(flag, fadeFrames);
    }

    void Pause(bool flag, int32_t fadeFrames, PauseMode pauseMode) {
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

    void FadeIn(int32_t frames) {
        if (IsAttachedSound())
            m_pSound->FadeIn(frames);
    }

    void SetVolume(float volume, int32_t frames) {
        if (IsAttachedSound())
            m_pSound->SetVolume(volume, frames);
    }

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    void SetVolumeThroughMode(int32_t bus, uint8_t modeBitFlag) {
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

    void SetPlayerPriority(int32_t priority) {
        if (IsAttachedSound())
            m_pSound->SetPlayerPriority(priority);
    }

    void SetChannelPriority(int32_t priority) {
        if (IsAttachedSound())
            m_pSound->SetChannelPriority(priority);
    }

    void SetTempoRatio(float tempoRatio) {
        if (IsAttachedSound())
            m_pSound->SetTempoRatio(tempoRatio);
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

    void SetTrackMixVolume(TrackBitFlagSet, int32_t, const MixVolume&);  // TODO
    void SetTrackMixVolume(const MixVolume&);                            // TODO
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

    bool ReadVariable(int16_t* var, int32_t varNo) const {
        if (!IsAttachedSound())
            return false;

        return m_pSound->ReadVariable(varNo, var);
    }

    static bool ReadGlobalVariable(int16_t* var, int32_t varNo) {
        return detail::SequenceSound::ReadGlobalVariable(varNo, var);
    }

    bool ReadTrackVariable(int16_t* var, int32_t varNo, int32_t trackNo) {
        if (!IsAttachedSound())
            return false;

        return m_pSound->ReadTrackVariable(trackNo, varNo, var);
    }

    bool WriteVariable(int32_t varNo, int16_t var) {
        if (!IsAttachedSound())
            return false;

        m_pSound->WriteVariable(varNo, var);
        return true;
    }

    static bool WriteGlobalVariable(int32_t varNo, int16_t var) {
        detail::SequenceSound::WriteGlobalVariable(varNo, var);
        return true;
    }

    bool WriteTrackVariable(int32_t trackNo, int32_t varNo, int16_t var) {
        if (!IsAttachedSound())
            return false;

        m_pSound->WriteTrackVariable(trackNo, varNo, var);
        return true;
    }

    void SetTrackMute(uint32_t trackBitFlag, SequenceMute mute) {
        if (IsAttachedSound())
            m_pSound->SetTrackMute(trackBitFlag, mute);
    }

    void SetTrackMute(uint32_t trackBitFlag, bool muteFlag) {
        if (IsAttachedSound())
            m_pSound->SetTrackMute(trackBitFlag, muteFlag);
    }

    void SetTrackSilence(uint32_t trackBitFlag, bool silenceFlag, int32_t fadeFrames) {
        if (IsAttachedSound())
            m_pSound->SetTrackSilence(trackBitFlag, silenceFlag, fadeFrames);
    }

    void SetTrackVolume(uint32_t trackBitFlag, float volume) {
        if (IsAttachedSound())
            m_pSound->SetTrackVolume(trackBitFlag, volume);
    }

    void SetTrackPitch(uint32_t trackBitFlag, float pitch) {
        if (IsAttachedSound())
            m_pSound->SetTrackPitch(trackBitFlag, pitch);
    }

    bool SetTrackBankIndex(uint32_t trackBitFlag, int32_t bankIndex) {
        if (!IsAttachedSound())
            return false;

        m_pSound->SetTrackBankIndex(trackBitFlag, bankIndex);
        return true;
    }

    bool SetTrackTranspose(uint32_t trackBitFlag, int8_t transpose) {
        if (!IsAttachedSound())
            return false;

        m_pSound->SetTrackTranspose(trackBitFlag, transpose);
        return true;
    }

    bool SetTrackVelocityRange(uint32_t trackBitFlag, uint8_t velocityRange) {
        if (!IsAttachedSound())
            return false;

        m_pSound->SetTrackVelocityRange(trackBitFlag, velocityRange);
        return true;
    }

    void SetTrackOutputLine(uint32_t trackBitFlag, uint32_t outputLine) {
        if (IsAttachedSound())
            m_pSound->SetTrackOutputLine(trackBitFlag, outputLine);
    }

    void ResetTrackOutputLine(uint32_t trackBitFlag) {
        if (IsAttachedSound())
            m_pSound->ResetTrackOutputLine(trackBitFlag);
    }

    void SetTrackOutputVolume(OutputDevice, uint32_t, float);          // TODO
    void SetTrackOutputPan(OutputDevice, uint32_t, float);             // TODO
    void SetTrackOutputSurroundPan(OutputDevice, uint32_t, float);     // TODO
    void SetTrackOutputMainSend(OutputDevice, uint32_t, float);        // TODO
    void SetTrackOutputFxSend(OutputDevice, uint32_t, AuxBus, float);  // TODO

    bool IsAttachedSound() const { return m_pSound != nullptr; }

    void DetachSound();

    void SetId(uint32_t id) {
        if (IsAttachedSound())
            m_pSound->SetId(id);
    }

    uint32_t GetId() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetId();
    }

    const SoundParam* GetAmbientParam() const {
        if (!IsAttachedSound())
            return nullptr;

        return &m_pSound->GetAmbientParam();
    }

    uint32_t GetTick() const {
        if (!IsAttachedSound())
            return 0;

        return m_pSound->GetTick();
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

    os::Tick GetProcessTick(const SoundProfile& profile) {
        if (IsAttachedSound())
            return 0;

        return m_pSound->GetProcessTick(profile);
    }

    void detail_AttachSoundAsTempHandle(detail::SequenceSound* sound);

    detail::SequenceSound* detail_GetAttachedSound() { return m_pSound; }
    const detail::SequenceSound* detail_GetAttachedSound() const { return m_pSound; }

private:
    NN_NO_COPY(SequenceSoundHandle);

    detail::SequenceSound* m_pSound{};
};

}  // namespace nn::atk
