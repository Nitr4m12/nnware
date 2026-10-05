#include <nn/atk/atk_SequenceSoundPlayer.h>

namespace {

const uint32_t IntervalMsecNumerator{5 << 16};
const uint32_t IntervalMsecDenominator{1 << 16};

}  // anonymous namespace

namespace nn::atk::detail::driver {

void SequenceSoundPlayer::InitSequenceSoundPlayer() {
    for (int variableNo{0}; variableNo < GlobalVariableCount; ++variableNo)
        m_GlobalVariable[variableNo] = VariableDefaultValue;
}

}  // namespace nn::atk::detail::driver
