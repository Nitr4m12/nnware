#include <nn/atk/atk_SequenceSound.h>

namespace nn::atk::detail {

SequenceSound::SequenceSound(SequenceSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool SequenceSound::Initialize()
#else
bool SequenceSound::Initialize(OutputReceiver* pOutputReceiver)
#endif
{
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    bool result{BasicSound::Initialize()};
#else
    bool result{BasicSound::Initialize(pOutputReceiver)};
#endif

    if (!result)
        return false;

    m_pTempSpecialHandle = nullptr;
    m_IsCalledPrepare = false;
    m_InitializeFlag = true;
    return true;
}

}  // namespace nn::atk::detail
