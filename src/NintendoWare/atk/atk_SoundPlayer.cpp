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

void SoundPlayer::Update() {
    DoFreePlayerHeap();

    for (auto itr{m_SoundList.begin()}; itr != m_SoundList.end();) {
        auto curItr{itr++};
        curItr->Update();
    }

    detail_SortPriorityList(false);
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

void SoundPlayer::detail_SortPriorityList(bool reverse) {
    if (m_PriorityList.size() <= 1)
        return;

    const int TmpCount{PlayerPriorityMax + 1};
    static PriorityList tmplist[TmpCount];

    while (!m_PriorityList.empty()) {
        detail::BasicSound& front{m_PriorityList.front()};
        m_PriorityList.pop_front();
        tmplist[front.CalcCurrentPlayerPriority()].push_back(front);
    }

    for (int i{0}; i < TmpCount; ++i) {
        while (!tmplist[i].empty()) {
            if (reverse) {
                detail::BasicSound& back{tmplist[i].back()};
                tmplist[i].pop_back();
                m_PriorityList.push_back(back);
            } else {
                detail::BasicSound& front{tmplist[i].front()};
                tmplist[i].pop_front();
                m_PriorityList.push_back(front);
            }
        }
    }
}

void SoundPlayer::PauseAllSound(bool flag, int fadeFrames) {
    for (auto itr{m_SoundList.begin()}; itr != m_SoundList.end();) {
        auto curItr{itr++};
        curItr->Pause(flag, fadeFrames);
    }
}

void SoundPlayer::PauseAllSound(bool flag, int fadeFrames, PauseMode pauseMode) {
    for (auto itr{m_SoundList.begin()}; itr != m_SoundList.end();) {
        auto curItr{itr++};
        curItr->Pause(flag, fadeFrames, pauseMode);
    }
}

void SoundPlayer::SetVolume(float volume) {
    m_Volume = volume < 0.0f ? 0.0f : volume;
}

void SoundPlayer::SetLowPassFilterFrequency(float lpfFreq) {
    m_LpfFreq = lpfFreq;
}

void SoundPlayer::SetBiquadFilter(int type, float value) {
    m_BiquadType = type;
    m_BiquadValue = value;
}

void SoundPlayer::SetDefaultOutputLine(u32 outputLineFlag) {
    m_OutputLineFlag = outputLineFlag;
}

}  // namespace nn::atk
