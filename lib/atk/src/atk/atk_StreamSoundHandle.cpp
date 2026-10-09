#include <nn/atk/atk_StreamSoundHandle.h>

namespace nn::atk {

StreamSoundHandle::StreamSoundHandle(SoundHandle* handle) {
    if (handle == nullptr)
        return;

    if (handle->IsAttachedSound()) {
        detail::StreamSound* sound;
        sound = detail::fnd::DynamicCast<detail::StreamSound*>(handle->detail_GetAttachedSound());
        if (sound != nullptr) {
            m_pSound = sound;
            detail_AttachSoundAsTempHandle(sound);
        }
    }
}

void StreamSoundHandle::detail_AttachSoundAsTempHandle(detail::StreamSound* sound) {
    m_pSound = sound;

    if (m_pSound->IsAttachedTempSpecialHandle())
        m_pSound->DetachTempSpecialHandle();

    m_pSound->m_pTempSpecialHandle = this;
}

void StreamSoundHandle::DetachSound() {
    if (!IsAttachedSound())
        return;

    if (m_pSound->m_pTempSpecialHandle == this)
        m_pSound->m_pTempSpecialHandle = nullptr;

    if (m_pSound != nullptr)
        m_pSound = nullptr;
}

}  // namespace nn::atk
