#pragma once

#include <nn/util/util_BitFlagSet.h>

#include <nn/atk/atk_SoundHandle.h>
#include <nn/atk/atk_WaveSound.h>

namespace nn::atk {

class WaveSoundHandle {
public:
    using TrackBitFlagSet = util::BitFlagSet<8, void>;

    explicit WaveSoundHandle(SoundHandle* pSoundHandle);

    void detail_AttachSoundAsTempHandle(detail::WaveSound* pSound);

    void ForceStop();

    void DetachSound();

private:
    detail::WaveSound* m_pSound;
};

}  // namespace nn::atk
