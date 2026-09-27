#include <nn/atk/atk_BankFile.h>

#include <nn/atk/atk_ElementType.h>

namespace nn::atk::detail {
namespace {

const uint8_t DefaultOriginalKey{60};
const uint8_t DefaultVolume{127};
const uint8_t DefaultPan{64};
const float DefaultPitch{1.0};
const bool DefaultIgnoreNoteOff{false};
const uint8_t DefaultKeyGroup{0};
const uint8_t DefaultInterpolationType{0};
const AdshrCurve DefaultAdshrCurve{127, 127, 127, 127, 127};

enum VelocityRegionBitFlag {
    VelocityRegionBitFlag_Key = 0,
    VelocityRegionBitFlag_Volume = 1,
    VelocityRegionBitFlag_Pan = 2,
    VelocityRegionBitFlag_Pitch = 3,
    VelocityRegionBitFlag_InstrumentNoteParam = 4,
    VelocityRegionBitFlag_Sends = 8,
    VelocityRegionBitFlag_Envelope = 9,
    VelocityRegionBitFlag_Randomizer = 10,
    VelocityRegionBitFlag_Lfo = 11,
    VelocityRegionBitFlag_BasicParamFlag = 0b01000011111
};

struct DirectChunk {
    Util::Reference toRegion;

    const void* GetRegion() const { return util::ConstBytePtr(this, toRegion.offset).Get(); }
};
static_assert(sizeof(DirectChunk) == 0x8);

struct RangeChunk {
    Util::Table<char> borderTable;

    const Util::Reference& GetRegionTableAddress(int index) const {
        return *util::ConstBytePtr(this, sizeof(borderTable.count) +
                                             util::align_up(borderTable.count, 4) +
                                             sizeof(Util::Reference) * index)
                    .Get<Util::Reference>();
    }

    const void* GetRegion(uint32_t index) const {
        bool isFoundRangeChunkIndex{false};
        uint32_t regionTableIndex{0};
        for (uint32_t i{0}; i < borderTable.count; ++i) {
            if (index <= borderTable.item[i]) {
                regionTableIndex = i;
                isFoundRangeChunkIndex = true;
                break;
            }
        }

        if (!isFoundRangeChunkIndex)
            return nullptr;

        const Util::Reference& ref{GetRegionTableAddress(regionTableIndex)};
        return util::ConstBytePtr(this, ref.offset).Get();
    }
};
static_assert(sizeof(RangeChunk) == 0x8);

struct IndexChunk {
    uint8_t min;
    uint8_t max;
    uint8_t reserved[2];
    Util::Reference toRegion[1];

    const void* GetRegion(uint32_t index) const {
        if (index >= min && index <= max)
            return util::ConstBytePtr(this, toRegion[index - min].offset).Get();

        return nullptr;
    }
};
static_assert(sizeof(IndexChunk) == 0xc);

enum RegionType { RegionType_Direct, RegionType_Range, RegionType_Index, RegionType_Unknown };

RegionType GetRegionType(uint16_t typeId) {
    switch (typeId) {
    case ElementType_BankFile_DirectReferenceTable:
        return RegionType_Direct;

    case ElementType_BankFile_RangeReferenceTable:
        return RegionType_Range;

    case ElementType_BankFile_IndexReferenceTable:
        return RegionType_Index;

    default:
        return RegionType_Unknown;
    }
}

const void* GetDirectChunk(const void* regionChunk) {
    const DirectChunk& directChunk{*reinterpret_cast<const DirectChunk*>(regionChunk)};

    return directChunk.GetRegion();
}

const void* GetRangeChunk(const void* regionChunk, uint32_t index) {
    const RangeChunk& rangeChunk{*reinterpret_cast<const RangeChunk*>(regionChunk)};

    return rangeChunk.GetRegion(index);
}

const void* GetIndexChunk(const void* regionChunk, uint32_t index) {
    const IndexChunk& indexChunk{*reinterpret_cast<const IndexChunk*>(regionChunk)};

    return indexChunk.GetRegion(index);
}

const void* GetRegion(const void* startPtr, uint16_t typeId, uint32_t offset, uint32_t index) {
    const void* regionChunk{util::ConstBytePtr(startPtr, offset).Get()};
    const void* region{nullptr};

    switch (GetRegionType(typeId)) {
    case RegionType_Direct:
        region = GetDirectChunk(regionChunk);
        break;

    case RegionType_Range:
        region = GetRangeChunk(regionChunk, index);
        break;

    case RegionType_Index:
        region = GetIndexChunk(regionChunk, index);
        break;

    case RegionType_Unknown:
    default:
        region = nullptr;
        break;
    }

    return region;
}

}  // anonymous namespace

const BankFile::InfoBlock* BankFile::FileHeader::GetInfoBlock() const {
    return util::ConstBytePtr(GetBlock(ElementType_BankFile_InfoBlock)).Get<InfoBlock>();
}

const Util::WaveIdTable& BankFile::InfoBlockBody::GetWaveIdTable() const {
    return *util::ConstBytePtr(this).Advance(toWaveIdTable.offset).Get<Util::WaveIdTable>();
}

const Util::ReferenceTable& BankFile::InfoBlockBody::GetInstrumentReferenceTable() const {
    return *util::ConstBytePtr(this)
                .Advance(toInstrumentReferenceTable.offset)
                .Get<Util::ReferenceTable>();
}

const BankFile::Instrument* BankFile::InfoBlockBody::GetInstrument(int programNo) const {
    auto& table{GetInstrumentReferenceTable()};
    auto& ref{table.item[programNo]};

    if (ref.IsValidTypeId(ElementType_BankFile_InstrumentInfo))
        return util::ConstBytePtr(table.GetReferedItem(programNo)).Get<BankFile::Instrument>();

    return nullptr;
}

const BankFile::KeyRegion* BankFile::Instrument::GetKeyRegion(uint32_t key) const {
    return util::ConstBytePtr(
               GetRegion(this, toKeyRegionChunk.typeId, toKeyRegionChunk.offset, key))
        .Get<KeyRegion>();
}

const BankFile::VelocityRegion* BankFile::KeyRegion::GetVelocityRegion(uint32_t velocity) const {
    return util::ConstBytePtr(GetRegion(this, toVelocityRegionChunk.typeId,
                                        toVelocityRegionChunk.offset, velocity))
        .Get<VelocityRegion>();
}

uint8_t BankFile::VelocityRegion::GetOriginalKey() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_Key)};
    if (result)
        return value;

    return DefaultOriginalKey;
}

uint8_t BankFile::VelocityRegion::GetVolume() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_Volume)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return DefaultVolume;
}

uint8_t BankFile::VelocityRegion::GetPan() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_Pan)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return DefaultPan;
}

float BankFile::VelocityRegion::GetPitch() const {
    float value;
    bool result{optionParameter.GetValuefloat(&value, VelocityRegionBitFlag_Pitch)};
    if (result)
        return value;

    return DefaultPitch;
}

bool BankFile::VelocityRegion::IsIgnoreNoteOff() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_InstrumentNoteParam)};
    if (result)
        return Util::DivideBy8bit(value, 0) != 0;

    return DefaultIgnoreNoteOff;
}

uint8_t BankFile::VelocityRegion::GetKeyGroup() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_InstrumentNoteParam)};
    if (result)
        return Util::DivideBy8bit(value, 1);

    return DefaultKeyGroup;
}

uint8_t BankFile::VelocityRegion::GetInterpolationType() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, VelocityRegionBitFlag_InstrumentNoteParam)};
    if (result)
        return Util::DivideBy8bit(value, 2);

    return DefaultInterpolationType;
}

const AdshrCurve& BankFile::VelocityRegion::GetAdshrCurve() const {
    uint32_t offsetToReference;
    bool result{optionParameter.GetValue(&offsetToReference, VelocityRegionBitFlag_Envelope)};
    if (result) {
        const auto& ref{*util::ConstBytePtr(this, offsetToReference).Get<Util::Reference>()};
        return *util::ConstBytePtr(&ref, ref.offset).Get<AdshrCurve>();
    }

    return DefaultAdshrCurve;
}

const BankFile::RegionParameter* BankFile::VelocityRegion::GetRegionParameter() const {
    if (optionParameter.bitFlag != VelocityRegionBitFlag_BasicParamFlag)
        return nullptr;

    return util::ConstBytePtr(this, sizeof(VelocityRegion)).Get<BankFile::RegionParameter>();
}

}  // namespace nn::atk::detail
