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

SoundPlayer::~SoundPlayer() {
    StopAllSound(0);
}

void SoundPlayer::StopAllSound(int fadeFrames) {
    for (auto itr{m_SoundList.begin()}; itr != m_SoundList.end();) {
        auto curItr{itr++};
        curItr->Stop(fadeFrames);
    }
}

void SoundPlayer::DoFreePlayerHeap() {
    for (auto itr{m_PlayerHeapFreeReqList.begin()}; itr != m_PlayerHeapFreeReqList.end();) {
        auto curItr{itr++};

        if (curItr->GetState() == detail::PlayerHeap::State_TaskFinished) {
            m_PlayerHeapFreeReqList.erase(curItr);
            m_PlayerHeapFreeList.push_back(*curItr);
        }
    }
}

}  // namespace nn::atk
