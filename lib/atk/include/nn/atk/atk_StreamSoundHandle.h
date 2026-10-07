#pragma once

#include <nn/util/util_BitFlagSet.h>

#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_StreamSound.h>

namespace nn::atk {

class StreamSoundHandle {
public:
    using TrackBitFlagSet = util::BitFlagSet<8, void>;

    explicit StreamSoundHandle(SoundHandle* pSoundHandle);

    void StartPrepared() { m_pSound->StartPrepared(); }

    void Stop(int fadeFrames) { m_pSound->Stop(fadeFrames); }

    void Pause(bool flag, int fadeFrames) { m_pSound->Pause(flag, fadeFrames); }
    void Pause(bool flag, int fadeFrames, PauseMode pauseMode) { m_pSound->Pause(flag, fadeFrames, pauseMode); }

    bool IsPrepared() const { return m_pSound->IsPrepared(); }
    bool IsPause() const { return m_pSound->IsPause(); }
    bool IsSuspendByLoadingDelay() const { return m_pSound->IsSuspendByLoadingDelay(); }
    bool IsLoadingDelayState() const { return m_pSound->IsLoadingDelayState(); }

    void FadeIn(int frames) { return m_pSound->FadeIn(frames); }

    void SetVolume(float volume, int frames) { m_pSound->SetVolume(volume, frames); }
    void SetVolumeThroughMode(int bus, uint8_t modeBitFlag) { m_pSound->SetVolumeThroughMode(bus, modeBitFlag); }
    void SetPitch(float pitch) { m_pSound->SetPitch(pitch); }
    void SetLowPassFilterFrequency(float lpfFreq) { m_pSound->SetLpfFreq(lpfFreq); }
    void SetPlayerPriority(int priority) { m_pSound->SetPlayerPriority(priority); }
    void SetPan(float pan) { m_pSound->SetPan(pan); }
    void SetSurroundPan(float span) { m_pSound->SetSurroundPan(span); }
    void SetMixMode(MixMode mixMode) { m_pSound->SetMixMode(mixMode); }

    void detail_AttachSoundAsTempHandle(detail::StreamSound* pSound);
    void DetachSound();

private:
    NN_NO_COPY(StreamSoundHandle);

    detail::StreamSound* m_pSound;
};

}  // namespace nn::atk
