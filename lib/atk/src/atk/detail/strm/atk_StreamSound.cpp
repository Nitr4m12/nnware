#include <nn/atk/atk_StreamSound.h>

namespace nn::atk::detail {

StreamSound::StreamSound(StreamSoundInstanceManager& manager) : m_Manager{manager} {}

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool StreamSound::Initialize()
#else
bool StreamSound::Initialize(OutputReceiver* pOutputReceiver)
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

    for (int i{0}; i < static_cast<int>(StreamTrackCount); ++i) {
        m_TrackVolume[i].InitValue(0);
        m_TrackVolume[i].SetTarget(1.0f, 1);
    }

    for (int i{0}; i < static_cast<int>(WaveChannelMax); ++i)
        m_AvailableTrackBitFlag[i] = 0;

    m_InitializeFlag = true;
    return true;
}

void StreamSound::Finalize() {
    if (m_InitializeFlag) {
        m_InitializeFlag = false;
        BasicSound::Finalize();
        m_Manager.Free(this);
    }
}

}  // namespace nn::atk::detail
