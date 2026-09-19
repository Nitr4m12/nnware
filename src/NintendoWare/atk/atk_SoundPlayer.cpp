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

#if NN_SDK_VER >= NN_MAKE_VER(4, 0, 0)
void SoundPlayer::SetMainSend(float send) {
    m_TvParam.mainSend = send;
}

float SoundPlayer::GetMainSend() const {
    return m_TvParam.mainSend;
}

void SoundPlayer::SetEffectSend(AuxBus bus, float send) {
    m_TvParam.fxSend[bus] = send;
}

float SoundPlayer::GetEffectSend(AuxBus bus) const {
    return m_TvParam.fxSend[bus];
}

float SoundPlayer::GetSend(int subMixBus) {
    if (subMixBus == 0)
        return m_TvParam.mainSend;

    if (subMixBus < 4)
        return m_TvParam.fxSend[subMixBus - 1u];

    return m_pOutputAdditionalParam->TryGetAdditionalSend(subMixBus);
}
#endif

void SoundPlayer::SetOutputVolume(OutputDevice device, float volume) {
    if (device != OutputDevice_Main)
        return;

    m_TvParam.volume = volume < 0.0f ? 0.0f : volume;
}

void SoundPlayer::RemoveSoundList(detail::BasicSound* pSound) {
    m_SoundList.erase(m_SoundList.iterator_to(*pSound));
    pSound->DetachSoundPlayer(this);
}

// NON_MATCHING: bad branching
void SoundPlayer::InsertPriorityList(detail::BasicSound* pSound) {
    auto itr{m_SoundList.begin()};

    while (itr != m_SoundList.end() && itr->CalcCurrentPlayerPriority() <= pSound->CalcCurrentPlayerPriority()) {
        if (m_IsFirstComeBased && itr->CalcCurrentPlayerPriority() == pSound->CalcCurrentPlayerPriority())
            break;

        ++itr;
    }

    m_SoundList.insert(itr, *pSound);
}

void SoundPlayer::RemovePriorityList(detail::BasicSound* pSound) {
    m_PriorityList.erase(m_PriorityList.iterator_to(*pSound));
}

}  // namespace nn::atk
