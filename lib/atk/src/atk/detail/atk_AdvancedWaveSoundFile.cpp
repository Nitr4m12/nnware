#include <nn/atk/detail/atk_AdvancedWaveSoundFile.h>
#include "nn/util/util_BytePtr.h"

namespace nn::atk::detail {

const AdvancedWaveSoundFile::InfoBlock* AdvancedWaveSoundFile::GetBlock() const {
    return reinterpret_cast<const InfoBlock*>(fileHeader.GetFirstBlock());
}

const BinaryTypes::ReferenceTable&
AdvancedWaveSoundFile::InfoBlockBody::GetTrackReferenceTable() const {
    util::ConstBytePtr bytePtr{util::ConstBytePtr(this, offsetToTrackTableReference)};

    return *bytePtr.Get<BinaryTypes::ReferenceTable>();
}

}  // namespace nn::atk::detail
