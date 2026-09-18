#include <nn/atk/atk_SoundPlayer.h>

namespace nn::atk {

SoundPlayer::SoundPlayer() {
    m_TvParam.Initialize();
}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
SoundPlayer::SoundPlayer(detail::OutputAdditionalParam* pParam) : m_pOutputAdditionalParam{pParam} {
    m_TvParam.Initialize();

    if (m_pOutputAdditionalParam != nullptr)
        m_pOutputAdditionalParam->Reset();
}
#endif

}  // namespace nn::atk
