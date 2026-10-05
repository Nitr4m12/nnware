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

SequenceSoundPlayer::SequenceSoundPlayer() {
    for (int varNo{0}; varNo < PlayerVariableCount; ++varNo)
        m_LocalVariable[varNo] = VariableDefaultValue;

    for (int trackNo{0}; trackNo < TrackCountPerPlayer; ++trackNo)
        m_pTracks[trackNo] = nullptr;
}

SequenceSoundPlayer::~SequenceSoundPlayer() {
    Finalize();
}

}  // namespace nn::atk::detail::driver
