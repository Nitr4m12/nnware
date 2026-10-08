#include <nn/atk/atk_WaveSound.h>

namespace nn::atk::detail {

WaveSound::WaveSound(WaveSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool WaveSound::Initialize()
#else
bool WaveSound::Initialize(OutputReceiver* pOutputReceiver)
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
    m_pWaveFile = nullptr;
    m_WaveType = WaveType_Invalid;
    m_InitializeFlag = true;
    m_IsCalledPrepare = false;
    m_ChannelCount = 0;
    return true;
}

}  // namespace nn::atk::detail
