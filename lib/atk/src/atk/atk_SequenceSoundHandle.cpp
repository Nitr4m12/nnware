#include <nn/atk/atk_SequenceSoundHandle.h>

namespace nn::atk {

SequenceSoundHandle::SequenceSoundHandle(SoundHandle* handle) {
    if (handle == nullptr)
        return;

    if (handle->IsAttachedSound()) {
        detail::SequenceSound* sound;
        sound = detail::fnd::DynamicCast<detail::SequenceSound*>(handle->detail_GetAttachedSound());
        if (sound != nullptr) {
            m_pSound = sound;
            detail_AttachSoundAsTempHandle(sound);
        }
    }
}

void SequenceSoundHandle::detail_AttachSoundAsTempHandle(detail::SequenceSound* sound) {
    m_pSound = sound;

    if (m_pSound->IsAttachedTempSpecialHandle())
        m_pSound->DetachTempSpecialHandle();

    m_pSound->m_pTempSpecialHandle = this;
}

}  // namespace nn::atk
