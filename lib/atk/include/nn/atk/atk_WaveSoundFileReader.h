#pragma once

#include <nn/atk/atk_WaveSoundFile.h>

namespace nn::atk::detail {

struct WaveSoundInfo {
    float pitch;
    AdshrCurve adshr;
    uint8_t pan;
    uint8_t surroundPan;
    uint8_t mainSend;
    uint8_t fxSend[AuxBus_Count];
    uint8_t lpfFreq;
    uint8_t biquadType;
    uint8_t biquadValue;
};
static_assert(sizeof(WaveSoundInfo) == 0x14);

struct WaveSoundNoteInfo {
    uint32_t waveArchiveId;
    int32_t waveIndex;
    AdshrCurve adshr;
    uint8_t originalKey;
    uint8_t pan;
    uint8_t surroundPan;
    uint8_t volume;
    float pitch;

    WaveSoundNoteInfo() : adshr(0, 0, 0, 0, 0) {}
};
static_assert(sizeof(WaveSoundNoteInfo) == 0x18);

class WaveSoundFileReader {
public:
    static const uint32_t SignatureFile{0x44535746};  // FWSD

    explicit WaveSoundFileReader(const void* waveSoundFile);

    bool IsAvailable() const { return m_pHeader != nullptr; }

    uint32_t GetWaveSoundCount() const;
    uint32_t GetNoteInfoCount(uint32_t index) const;
    uint32_t GetTrackInfoCount(uint32_t index) const;

    bool ReadWaveSoundInfo(WaveSoundInfo* dst, uint32_t index) const;

    bool ReadNoteInfo(WaveSoundNoteInfo* dst, uint32_t index, uint32_t noteIndex) const;

    bool IsFilterSupportedVersion() const;

private:
    const WaveSoundFile::FileHeader* m_pHeader{};
    const WaveSoundFile::InfoBlockBody* m_pInfoBlockBody{};
};
static_assert(sizeof(WaveSoundFileReader) == 0x10);

}  // namespace nn::atk::detail
