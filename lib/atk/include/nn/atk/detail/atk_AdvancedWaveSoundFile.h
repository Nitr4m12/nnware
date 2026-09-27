#pragma once

#include <nn/util/util_BinaryFormat.h>

#include <nn/atk/detail/atk_BinaryTypes.h>

namespace nn::atk::detail {

struct AdvancedWaveSoundFile {
    struct WaveSoundTrack;
    struct InfoBlockBody {
        BinaryTypes::ReferenceTable* GetTrackReferenceTable() const;
        WaveSoundTrack* GetWaveSoundTrack(int32_t index) const;

        uint32_t offsetToTrackTableReference;
        BinaryTypes::Reference toTrackTable;
    };
    static_assert(sizeof(InfoBlockBody) == 0x8);

    struct InfoBlock {
        util::BinaryBlockHeader blockHeader;
        InfoBlockBody body;
    };
    static_assert(sizeof(InfoBlock) == 0x18);

    struct WaveSoundClip;
    struct WaveSoundTrack {
        BinaryTypes::ReferenceTable* GetClipReferenceTable() const;
        WaveSoundClip* GetWaveSoundClip(int32_t index) const;

        uint32_t offsetToCurveTableReference;
        uint32_t offsetToClipTableReference;
        BinaryTypes::Reference toClipTable;
    };

    struct WaveSoundClip {
        uint32_t waveIndex;
        uint32_t position;
        uint32_t duration;
        uint32_t startOffset;
        float pitch;
        uint8_t volume;
        uint8_t pan;
        uint8_t padding[2];
    };
    static_assert(sizeof(WaveSoundClip) == 0x18);

    InfoBlock* GetBlock() const;

    util::BinaryFileHeader fileHeader;
};
static_assert(sizeof(AdvancedWaveSoundFile) == 0x20);

}  // namespace nn::atk::detail
