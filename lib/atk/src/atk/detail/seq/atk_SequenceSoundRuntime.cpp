#include <nn/atk/atk_SequenceSoundRuntime.h>

namespace nn::atk::detail {

SequenceSoundRuntime::SequenceSoundRuntime() {
    m_SequenceCallback.m_pSequenceSoundRuntime = this;
}

}  // namespace nn::atk::detail
