#include <nn/atk/atk_WaveSoundFile.h>

#include <nn/atk/atk_ElementType.h>

namespace nn::atk::detail {
namespace {

const uint8_t WsdDefaultPan{64};
const int8_t WsdDefaultSurroundPan{0};
const float WsdDefaultPitch{1.0};
const uint8_t WsdDefaultMainSend{127};
const uint8_t WsdDefaultFxSend{0};
const AdshrCurve WsdDefaultAdshrCurve{127, 127, 127, 127, 127};
const uint8_t WsdDefaultLpfFreq{64};
const uint8_t WsdDefaultBiquadType{0};
const uint8_t WsdDefaultBiquadValue{0};
const uint8_t WsdDefaultKey{64};
const uint8_t WsdDefaultVolume{96};

enum WaveSoundInfoBitFlagWsd {
    WaveSoundInfoBitFlagWsd_Pan = 0,
    WaveSoundInfoBitFlagWsd_Pitch = 1,
    WaveSoundInfoBitFlagWsd_Filter = 2,
    WaveSoundInfoBitFlagWsd_Send = 8,
    WaveSoundInfoBitFlagWsd_Envelope = 9,
    WaveSoundInfoBitFlagWsd_Randomizer = 10,
};

enum NoteInfoBitFlag {
    NoteInfoBitFlag_Key = 0,
    NoteInfoBitFlag_Volume = 1,
    NoteInfoBitFlag_Pan = 2,
    NoteInfoBitFlag_Pitch = 3,
    NoteInfoBitFlag_Send = 8,
    NoteInfoBitFlag_Envelope = 9,
    NoteInfoBitFlag_Randomizer = 10,
    NoteInfoBitFlag_Lfo = 11,
};

struct SendValueWsd {
    uint8_t mainSend;
    Util::Table<uint8_t, uint8_t> fxSend;
};

}  // anonymous namespace

const WaveSoundFile::InfoBlock* WaveSoundFile::FileHeader::GetInfoBlock() const {
    return util::ConstBytePtr(GetBlock(ElementType_WaveSoundFile_InfoBlock))
        .Get<WaveSoundFile::InfoBlock>();
}

const WaveSoundFile::WaveSoundData&
WaveSoundFile::InfoBlockBody::GetWaveSoundData(uint32_t index) const {
    return *util::ConstBytePtr(GetWaveSoundDataReferenceTable().GetReferedItem(
                                   index, ElementType_WaveSoundFile_WaveSoundMetaData))
                .Get<WaveSoundData>();
}

const Util::ReferenceTable& WaveSoundFile::InfoBlockBody::GetWaveSoundDataReferenceTable() const {
    return *util::ConstBytePtr(this, toWaveSoundDataReferenceTable.offset)
                .Get<Util::ReferenceTable>();
}

const Util::WaveIdTable& WaveSoundFile::InfoBlockBody::GetWaveIdTable() const {
    return *util::ConstBytePtr(this, toWaveIdTable.offset).Get<Util::WaveIdTable>();
}

const WaveSoundFile::WaveSoundInfo& WaveSoundFile::WaveSoundData::GetWaveSoundInfo() const {
    return *util::ConstBytePtr(this, toWaveSoundInfo.offset).Get<WaveSoundInfo>();
}

const Util::ReferenceTable& WaveSoundFile::WaveSoundData::GetTrackInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toTrackInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const Util::ReferenceTable& WaveSoundFile::WaveSoundData::GetNoteInfoReferenceTable() const {
    return *util::ConstBytePtr(this, toNoteInfoReferenceTable.offset).Get<Util::ReferenceTable>();
}

const WaveSoundFile::TrackInfo& WaveSoundFile::WaveSoundData::GetTrackInfo(uint32_t index) const {
    const void* pTrackInfo;
    pTrackInfo =
        GetTrackInfoReferenceTable().GetReferedItem(index, ElementType_WaveSoundFile_TrackInfo);

    return *util::ConstBytePtr(pTrackInfo).Get<TrackInfo>();
}

const WaveSoundFile::NoteInfo& WaveSoundFile::WaveSoundData::GetNoteInfo(uint32_t index) const {
    const void* pNoteInfo;
    pNoteInfo =
        GetNoteInfoReferenceTable().GetReferedItem(index, ElementType_WaveSoundFile_NoteInfo);

    return *util::ConstBytePtr(pNoteInfo).Get<NoteInfo>();
}

uint8_t WaveSoundFile::WaveSoundInfo::GetPan() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Pan)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return WsdDefaultPan;
}

int8_t WaveSoundFile::WaveSoundInfo::GetSurroundPan() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Pan)};
    if (result)
        return static_cast<int8_t>(Util::DivideBy8bit(value, 1));

    return WsdDefaultSurroundPan;
}

float WaveSoundFile::WaveSoundInfo::GetPitch() const {
    float value;
    bool result{optionParameter.GetValuefloat(&value, WaveSoundInfoBitFlagWsd_Pitch)};
    if (result)
        return value;

    return WsdDefaultPitch;
}

void WaveSoundFile::WaveSoundInfo::GetSendValue(uint8_t* mainSend, uint8_t* fxSend,
                                                uint8_t fxSendCount) const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Send)};

    if (result) {
        const SendValueWsd& sendValue = *util::ConstBytePtr(this, value).Get<SendValueWsd>();

        *mainSend = sendValue.mainSend;
        int countSize{sendValue.fxSend.count > AuxBus_Count ? AuxBus_Count :
                                                              sendValue.fxSend.count};

        for (int i{0}; i < countSize; ++i)
            fxSend[i] = sendValue.fxSend.item[i];
    } else {
        *mainSend = WsdDefaultMainSend;
        for (int i{0}; i < fxSendCount; ++i)
            fxSend[i] = WsdDefaultFxSend;
    }
}

const AdshrCurve& WaveSoundFile::WaveSoundInfo::GetAdshrCurve() const {
    uint32_t offsetToReference;
    bool result{optionParameter.GetValue(&offsetToReference, WaveSoundInfoBitFlagWsd_Envelope)};
    if (result) {
        const auto& ref{*util::ConstBytePtr(this, offsetToReference).Get<Util::Reference>()};
        return *util::ConstBytePtr(&ref, ref.offset).Get<AdshrCurve>();
    }

    return WsdDefaultAdshrCurve;
}

uint8_t WaveSoundFile::WaveSoundInfo::GetLpfFreq() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Filter)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return WsdDefaultLpfFreq;
}

uint8_t WaveSoundFile::WaveSoundInfo::GetBiquadType() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Filter)};
    if (result)
        return Util::DivideBy8bit(value, 1);

    return WsdDefaultBiquadType;
}

uint8_t WaveSoundFile::WaveSoundInfo::GetBiquadValue() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, WaveSoundInfoBitFlagWsd_Filter)};
    if (result)
        return Util::DivideBy8bit(value, 2);

    return WsdDefaultBiquadValue;
}

const Util::ReferenceTable& WaveSoundFile::TrackInfo::GetNoteEventReferenceTable() const {
    return *util::ConstBytePtr(this, toNoteEventReferenceTable.offset).Get<Util::ReferenceTable>();
}

const WaveSoundFile::NoteEvent& WaveSoundFile::TrackInfo::GetNoteEvent(uint32_t index) const {
    const void* pNoteEvent;
    pNoteEvent =
        GetNoteEventReferenceTable().GetReferedItem(index, ElementType_WaveSoundFile_NoteEvent);

    return *util::ConstBytePtr(pNoteEvent).Get<NoteEvent>();
}

uint8_t WaveSoundFile::NoteInfo::GetOriginalKey() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, NoteInfoBitFlag_Key)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return WsdDefaultKey;
}

uint8_t WaveSoundFile::NoteInfo::GetVolume() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, NoteInfoBitFlag_Volume)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return WsdDefaultVolume;
}

uint8_t WaveSoundFile::NoteInfo::GetPan() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, NoteInfoBitFlag_Pan)};
    if (result)
        return Util::DivideBy8bit(value, 0);

    return WsdDefaultPan;
}

uint8_t WaveSoundFile::NoteInfo::GetSurroundPan() const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, NoteInfoBitFlag_Pan)};
    if (result)
        return Util::DivideBy8bit(value, 1);

    return WsdDefaultSurroundPan;
}

float WaveSoundFile::NoteInfo::GetPitch() const {
    float value;
    bool result{optionParameter.GetValuefloat(&value, NoteInfoBitFlag_Pitch)};
    if (result)
        return value;

    return WsdDefaultPitch;
}

// NON_MATCHING
void WaveSoundFile::NoteInfo::GetSendValue(uint8_t* mainSend, uint8_t** fxSend,
                                           uint8_t fxSendCount) const {
    uint32_t value;
    bool result{optionParameter.GetValue(&value, NoteInfoBitFlag_Send)};

    if (!result) {
        *mainSend = WsdDefaultMainSend;
        for (int i{0}; i < fxSendCount; ++i)
            fxSend[i][0] = WsdDefaultFxSend;
    } else {
        const SendValueWsd& sendValue = *util::ConstBytePtr(this, value).Get<SendValueWsd>();

        *mainSend = sendValue.mainSend;
        int countSize{sendValue.fxSend.count > AuxBus_Count ? AuxBus_Count :
                                                              sendValue.fxSend.count};

        for (int i{0}; i < countSize; ++i)
            fxSend[i][0] = sendValue.fxSend.item[i];
    }
}

const AdshrCurve& WaveSoundFile::NoteInfo::GetAdshrCurve() const {
    uint32_t offsetToReference;
    bool result{optionParameter.GetValue(&offsetToReference, NoteInfoBitFlag_Envelope)};
    if (result) {
        const auto& ref{*util::ConstBytePtr(this, offsetToReference).Get<Util::Reference>()};
        return *util::ConstBytePtr(&ref, ref.offset).Get<AdshrCurve>();
    }

    return WsdDefaultAdshrCurve;
}

}  // namespace nn::atk::detail
