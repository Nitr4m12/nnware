#include <nn/atk/atk_WaveSoundHandle.h>

namespace nn::atk {

WaveSoundHandle::WaveSoundHandle(SoundHandle* handle) {
    if (handle == nullptr)
        return;

    if (handle->IsAttachedSound()) {
        detail::WaveSound* sound;
        sound = detail::fnd::DynamicCast<detail::WaveSound*>(handle->detail_GetAttachedSound());
        if (sound != nullptr) {
            m_pSound = sound;
            detail_AttachSoundAsTempHandle(sound);
        }
    }
}

void WaveSoundHandle::detail_AttachSoundAsTempHandle(detail::WaveSound* sound) {
    m_pSound = sound;

    if (m_pSound->IsAttachedTempSpecialHandle())
        m_pSound->DetachTempSpecialHandle();

    m_pSound->m_pTempSpecialHandle = this;
}

void WaveSoundHandle::ForceStop() {
    if (IsAttachedSound())
        m_pSound->ForceStop();
}

void WaveSoundHandle::DetachSound() {
    if (!IsAttachedSound())
        return;

    if (m_pSound->m_pTempSpecialHandle == this)
        m_pSound->m_pTempSpecialHandle = nullptr;

    if (m_pSound != nullptr)
        m_pSound = nullptr;
}

}  // namespace nn::atk
