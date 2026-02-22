/**
 * @brief Basic sound player from a sound archive.
 */

#pragma once

#include <nn/atk/detail/atk_SoundArchiveManager.h>
#include <nn/atk/detail/atk_AdvancedWaveSoundRuntime.h>
#include <nn/atk/detail/seq/atk_SequenceSoundRuntime.h>
#include <nn/atk/detail/strm/atk_StreamSoundRuntime.h>
#include <nn/atk/detail/wsd/atk_WaveSoundRuntime.h>

namespace nn::atk {
class SoundArchivePlayer {
public:
    SoundArchivePlayer();

    virtual ~SoundArchivePlayer();

    bool IsAvailable() const;
    void Finalize();
    void StopAllSound(int32_t, bool);
    void DisposeInstances();

    nn::atk::detail::SoundArchiveManager mArchiveManager;          // _8
    nn::atk::detail::SequenceSoundRuntime mSeqSoundRuntime;        // _50
    nn::atk::detail::WaveSoundRuntime mWaveSoundRuntime;           // _130
    nn::atk::detail::AdvancedWaveSoundRuntime mAdvancedWaveSound;  // _1B0
    nn::atk::detail::StreamSoundRuntime mStreamSoundRuntime;       // _1E0
    uint64_t _290;
    uint32_t _298;
    uint8_t _29C[0x2E8 - 0x29C];
};

}  // namespace nn::atk
