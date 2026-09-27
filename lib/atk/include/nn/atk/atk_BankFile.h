#pragma once

#include <nn/atk/atk_Util.h>

namespace nn::atk::detail {

struct BankFile {
    struct InfoBlock;
    struct FileHeader : Util::SoundFileHeader {
        const InfoBlock* GetInfoBlock() const;
    };

    struct Instrument;
    struct InfoBlockBody {
        Util::Reference toWaveIdTable;
        Util::Reference toInstrumentReferenceTable;

        const Util::WaveIdTable& GetWaveIdTable() const;
        const Util::ReferenceTable& GetInstrumentReferenceTable() const;

        uint32_t GetWaveIdCount() const { return GetWaveIdTable().table.count; }

        int32_t GetInstrumentCount() const {
            return static_cast<int32_t>(GetInstrumentReferenceTable().count);
        }

        const Util::WaveId* GetWaveId(uint32_t index) const {
            return GetWaveIdTable().GetWaveId(index);
        }

        const Instrument* GetInstrument(int programNo) const;
    };
    static_assert(sizeof(InfoBlockBody) == 0x10);

    struct InfoBlock {
        BinaryBlockHeader header;
        InfoBlockBody body;
    };
    static_assert(sizeof(InfoBlock) == 0x18);

    struct KeyRegion;
    struct Instrument {
        Util::Reference toKeyRegionChunk;

        const KeyRegion* GetKeyRegion(uint32_t key) const;
    };
    static_assert(sizeof(Instrument) == 0x8);

    struct VelocityRegion;
    struct KeyRegion {
        Util::Reference toVelocityRegionChunk;

        const VelocityRegion* GetVelocityRegion(uint32_t velocity) const;
    };
    static_assert(sizeof(KeyRegion) == 0x8);

    struct RegionParameter;
    struct VelocityRegion {
        uint32_t waveIdTableIndex;
        Util::BitFlag optionParameter;

        uint8_t GetOriginalKey() const;
        uint8_t GetVolume() const;
        uint8_t GetPan() const;
        float GetPitch() const;
        bool IsIgnoreNoteOff() const;
        uint8_t GetKeyGroup() const;
        uint8_t GetInterpolationType() const;
        const AdshrCurve& GetAdshrCurve() const;
        const RegionParameter* GetRegionParameter() const;
    };
    static_assert(sizeof(VelocityRegion) == 0x8);

    struct RegionParameter {
        uint8_t originalKey;
        uint8_t padding1[3];
        uint8_t volume;
        uint8_t padding2[3];
        uint8_t pan;
        int8_t surroundPan;
        uint8_t padding3[2];
        float pitch;
        bool isIgnoreNoteOff;
        uint8_t keyGroup;
        uint8_t interpolationType;
        uint8_t padding4[1];
        uint32_t offset;
        Util::Reference refToAdshrCurve;
        AdshrCurve adshrCurve;
    };
    static_assert(sizeof(RegionParameter) == 0x28);
};

}  // namespace nn::atk::detail
