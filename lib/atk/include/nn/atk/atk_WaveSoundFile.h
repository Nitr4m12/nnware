#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct WaveSoundFile {
    struct InfoBlock;
    struct FileHeader : Util::SoundFileHeader {
        const InfoBlock* GetInfoBlock() const;
    };

    struct WaveSoundData;
    struct InfoBlockBody {
        Util::Reference toWaveIdTable;
        Util::Reference toWaveSoundDataReferenceTable;

        const Util::WaveIdTable& GetWaveIdTable() const;
        const Util::ReferenceTable& GetWaveSoundDataReferenceTable() const;

        uint32_t GetWaveIdCount() const { return GetWaveIdTable().GetCount(); }

        uint32_t GetWaveSoundCount() const { return GetWaveSoundDataReferenceTable().count; }

        const Util::WaveId* GetWaveId(uint32_t index) const {
            return GetWaveIdTable().GetWaveId(index);
        }

        const WaveSoundData& GetWaveSoundData(uint32_t index) const;
    };
    static_assert(sizeof(InfoBlockBody) == 0x10);

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };
    static_assert(sizeof(InfoBlock) == 0x18);

    struct WaveSoundInfo;
    struct TrackInfo;
    struct NoteInfo;
    struct WaveSoundData {
        Util::Reference toWaveSoundInfo;
        Util::Reference toTrackInfoReferenceTable;
        Util::Reference toNoteInfoReferenceTable;

        const WaveSoundInfo& GetWaveSoundInfo() const;
        const Util::ReferenceTable& GetTrackInfoReferenceTable() const;
        const Util::ReferenceTable& GetNoteInfoReferenceTable() const;

        uint32_t GetTrackCount() const { return GetTrackInfoReferenceTable().count; }

        uint32_t GetNoteCount() const { return GetNoteInfoReferenceTable().count; }

        const TrackInfo& GetTrackInfo(uint32_t index) const;
        const NoteInfo& GetNoteInfo(uint32_t index) const;
    };
    static_assert(sizeof(WaveSoundData) == 0x18);

    struct WaveSoundInfo {
        Util::BitFlag optionParameter;

        uint8_t GetPan() const;
        int8_t GetSurroundPan() const;
        float GetPitch() const;
        void GetSendValue(uint8_t* mainSend, uint8_t* fxSend, uint8_t fxSendCount) const;
        const AdshrCurve& GetAdshrCurve() const;
        uint8_t GetLpfFreq() const;
        uint8_t GetBiquadType() const;
        uint8_t GetBiquadValue() const;
    };
    static_assert(sizeof(WaveSoundInfo) == 0x4);

    struct NoteEvent;
    struct TrackInfo {
        Util::Reference toNoteEventReferenceTable;

        const Util::ReferenceTable& GetNoteEventReferenceTable() const;

        uint32_t GetNoteEventCount() const { return GetNoteEventReferenceTable().count; }

        const NoteEvent& GetNoteEvent(uint32_t index) const;
    };
    static_assert(sizeof(TrackInfo) == 0x8);

    struct NoteEvent {
        float position;
        float length;
        uint32_t noteIndex;
        uint32_t reserved;
    };
    static_assert(sizeof(NoteEvent) == 0x10);

    struct NoteInfo {
        uint32_t waveIdTableIndex;
        Util::BitFlag optionParameter;

        uint8_t GetOriginalKey() const;
        uint8_t GetVolume() const;
        uint8_t GetPan() const;
        uint8_t GetSurroundPan() const;
        float GetPitch() const;
        void GetSendValue(uint8_t* mainSend, uint8_t** fxSend, uint8_t fxSendCount) const;
        const AdshrCurve& GetAdshrCurve() const;
    };
    static_assert(sizeof(NoteInfo) == 0x8);
};

}  // namespace nn::atk::detail
