#include <nn/atk/detail/atk_AdvancedWaveSoundFile.h>

namespace nn::atk::detail {

const AdvancedWaveSoundFile::InfoBlock* AdvancedWaveSoundFile::GetBlock() const {
    return reinterpret_cast<const InfoBlock*>(fileHeader.GetFirstBlock());
}

}  // namespace nn::atk::detail
