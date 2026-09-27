#pragma once

#include <nn/atk/detail/atk_AdvancedWaveSoundFile.h>

namespace nn::atk::detail {

struct AdvancedWaveSoundClipInfo {
    uint32_t waveIndex;
    uint32_t position;
    uint32_t duration;
    uint32_t startOffset;
    float pitch;
    uint8_t volume;
    uint8_t pan;
};
static_assert(sizeof(AdvancedWaveSoundClipInfo) == 0x18);

struct AdvancedWaveSoundTrackInfo {
    constexpr static uint32_t AdvancedWaveSoundClipInfoCountMax = 10;

    int32_t waveSoundClipCount;
    AdvancedWaveSoundClipInfo waveSoundClipInfo[AdvancedWaveSoundClipInfoCountMax];
};
static_assert(sizeof(AdvancedWaveSoundTrackInfo) == 0xf4);

struct AdvancedWaveSoundTrackInfoSet {
    constexpr static uint32_t AdvancedWaveSoundTrackInfoCountMax = 4;

    int32_t waveSoundTrackCount;
    AdvancedWaveSoundTrackInfo waveSoundTrackInfo[AdvancedWaveSoundTrackInfoCountMax];
};
static_assert(sizeof(AdvancedWaveSoundTrackInfoSet) == 0x3d4);

class AdvancedWaveSoundFileReader {
public:
    explicit AdvancedWaveSoundFileReader(const void* pFile);

    int32_t GetWaveSoundTrackCount();
    int32_t GetWaveSoundClipCount(int32_t index);

    bool ReadWaveSoundTrackInfoSet(AdvancedWaveSoundTrackInfoSet* pTrackInfoSet);

private:
    AdvancedWaveSoundFile::InfoBlockBody* m_pInfoBlockBody;
};
static_assert(sizeof(AdvancedWaveSoundFileReader) == 0x8);

}  // namespace nn::atk::detail
