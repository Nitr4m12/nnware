#include <nn/atk/detail/atk_AdvancedWaveSoundFile.h>

namespace nn::atk::detail {

const AdvancedWaveSoundFile::InfoBlock* AdvancedWaveSoundFile::GetBlock() const {
    return reinterpret_cast<const InfoBlock*>(fileHeader.GetFirstBlock());
}

const BinaryTypes::ReferenceTable&
AdvancedWaveSoundFile::InfoBlockBody::GetTrackReferenceTable() const {
    util::ConstBytePtr bytePtr{util::ConstBytePtr(this, offsetToTrackTableReference)};

    return *bytePtr.Get<BinaryTypes::ReferenceTable>();
}

const AdvancedWaveSoundFile::WaveSoundTrack&
AdvancedWaveSoundFile::InfoBlockBody::GetWaveSoundTrack(int32_t index) const {
    const void* pReferedItem{GetTrackReferenceTable().GetReferedItem(index)};

    const WaveSoundTrack* pWaveSoundTrack{static_cast<const WaveSoundTrack*>(pReferedItem)};
    return *pWaveSoundTrack;
}

const BinaryTypes::ReferenceTable&
AdvancedWaveSoundFile::WaveSoundTrack::GetClipReferenceTable() const {
    util::ConstBytePtr bytePtr{util::ConstBytePtr(this, offsetToClipTableReference)};

    return *bytePtr.Get<BinaryTypes::ReferenceTable>();
}

}  // namespace nn::atk::detail
