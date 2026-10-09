#pragma once

#include <nn/util/util_BitFlagSet.h>

#include <nn/atk/atk_SequenceSound.h>
#include <nn/atk/atk_SoundHandle.h>

namespace nn::atk {

class SequenceSoundHandle {
public:
    using TrackBitFlagSet = util::BitFlagSet<16, void>;

    constexpr static uint32_t BankIndexMin = 0;
    constexpr static uint32_t BankIndexMax = 3;

    constexpr static uint8_t TransposeMin = 192;
    constexpr static uint8_t TransposeMax = 63;

    constexpr static uint8_t VelocityRangeMin = 0;
    constexpr static uint8_t VelocityRangeMax = 127;

    constexpr static uint32_t VariableIndexMax = 15;
    constexpr static uint32_t TrackIndexMax = 15;

    explicit SequenceSoundHandle(SoundHandle* pSoundHandle);

    void detail_AttachSoundAsTempHandle(detail::SequenceSound* pSound);
    void DetachSound();

private:
    detail::SequenceSound* m_pSound;
};

}  // namespace nn::atk
