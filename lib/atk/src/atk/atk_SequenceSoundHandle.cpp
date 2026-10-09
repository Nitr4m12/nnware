#include <nn/atk/atk_SequenceSoundHandle.h>

namespace nn::atk {

void SequenceSoundHandle::detail_AttachSoundAsTempHandle(detail::SequenceSound* sound) {
    m_pSound = sound;

    if (m_pSound->IsAttachedTempSpecialHandle())
        m_pSound->DetachTempSpecialHandle();

    m_pSound->m_pTempSpecialHandle = this;
}

}  // namespace nn::atk
