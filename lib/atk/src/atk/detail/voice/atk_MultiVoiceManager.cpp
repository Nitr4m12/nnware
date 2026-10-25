#include <nn/atk/atk_MultiVoiceManager.h>

namespace nn::atk::detail::driver {

MultiVoiceManager& MultiVoiceManager::GetInstance() {
    static MultiVoiceManager instance;
    return instance;
}

MultiVoiceManager::MultiVoiceManager() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount) {
    return sizeof(MultiVoice) * voiceCount;
}
#else
size_t MultiVoiceManager::GetObjectSize(const SoundInstanceConfig& config) {
    size_t result{sizeof(MultiVoice)};

    const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
    if (AdditionalParamBufferSize != 0) {
        result += sizeof(OutputAdditionalParam);
        result += AdditionalParamBufferSize;
    }

    return result;
}

size_t MultiVoiceManager::GetRequiredMemSize(int32_t voiceCount,
                                             const SoundInstanceConfig& config) {
    return GetObjectSize(config) * voiceCount;
}
#endif

void MultiVoiceManager::Initialize(void* mem, size_t memSize, const SoundInstanceConfig& config) {
    if (m_Initialized)
        return;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    const size_t TotalObjectSize{sizeof(MultiVoice)};
#else
    const size_t TotalObjectSize{GetObjectSize(config)};
#endif

    const int VoiceCount{static_cast<int>(memSize / TotalObjectSize)};

    uint8_t* ptr{static_cast<uint8_t*>(mem)};

    for (int i{0}; i < VoiceCount; ++i) {
        uint8_t* voicePtr{ptr};

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        new (voicePtr) MultiVoice();
#else
        ptr += sizeof(MultiVoice);
        OutputAdditionalParam* additionalParamPtr{};
        const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};

        if (AdditionalParamBufferSize != 0) {
            additionalParamPtr = new (ptr) OutputAdditionalParam;
            ptr += sizeof(OutputAdditionalParam);

            void* additionalParamBufferPtr{ptr};

            additionalParamPtr->Initialize(additionalParamBufferPtr, AdditionalParamBufferSize,
                                           config);
            ptr += AdditionalParamBufferSize;
        }

        new (voicePtr) MultiVoice(additionalParamPtr);
#endif

        m_FreeVoiceList.push_back(*reinterpret_cast<MultiVoice*>(voicePtr));
    }

    m_Initialized = true;
}

void MultiVoiceManager::Finalize() {
    if (!m_Initialized)
        return;

    StopAllVoices();

    while (!m_FreeVoiceList.empty()) {
        MultiVoice& voice{m_FreeVoiceList.front()};
        m_FreeVoiceList.pop_front();
        voice.~MultiVoice();
    }

    m_Initialized = false;
}

void MultiVoiceManager::StopAllVoices() {
    while (!m_PrioVoiceList.empty()) {
        MultiVoice& voice{m_PrioVoiceList.front()};
        voice.Stop();
        if (voice.m_Callback != nullptr)
            voice.m_Callback(&voice, MultiVoice::VoiceCallbackStatus_Cancel, voice.m_pCallbackData);
        voice.Free();
    }
}

MultiVoice* MultiVoiceManager::AllocVoice(int32_t voiceChannelCount, int32_t priority,
                                          MultiVoice::VoiceCallback callback, void* callbackData) {
    if (m_FreeVoiceList.empty()) {
        if (DropLowestPriorityVoice(priority) == 0)
            return nullptr;
    }

    MultiVoice& voice{m_FreeVoiceList.front()};
    if (!voice.Alloc(voiceChannelCount, priority, callback, callbackData))
        return nullptr;

    voice.m_Priority = priority & MultiVoice::PriorityNoDrop;

    m_FreeVoiceList.erase(m_FreeVoiceList.iterator_to(voice));

    AppendVoiceList(&voice);
    return &voice;
}

void MultiVoiceManager::FreeVoice(MultiVoice* voice) {
    RemoveVoiceList(voice);
}

void MultiVoiceManager::UpdateAllVoiceStatus() {
    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};
        curItr->UpdateVoiceStatus();
    }
}

void MultiVoiceManager::UpdateAudioFrameVoiceStatus() {
    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};

        if (curItr->GetUpdateType() == UpdateType_AudioFrame)
            curItr->UpdateVoiceStatus();
    }
}

void MultiVoiceManager::UpdateAllVoices() {
    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};
        curItr->Calc();
    }

    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};
        curItr->Update();
    }
}

void MultiVoiceManager::UpdateAudioFrameVoices() {
    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};

        if (curItr->GetUpdateType() == UpdateType_AudioFrame)
            curItr->Calc();
    }

    for (auto itr{m_PrioVoiceList.begin()}; itr != m_PrioVoiceList.end();) {
        auto curItr{itr++};

        if (curItr->GetUpdateType() == UpdateType_AudioFrame)
            curItr->Update();
    }
}

void MultiVoiceManager::AppendVoiceList(MultiVoice* voice) {
    auto itr{m_PrioVoiceList.rbegin()};
    while (itr != m_PrioVoiceList.rend()) {
        if (itr->GetPriority() <= voice->GetPriority())
            break;

        ++itr;
    }

    m_PrioVoiceList.insert(itr.base(), *voice);
}

void MultiVoiceManager::RemoveVoiceList(MultiVoice* voice) {
    m_PrioVoiceList.erase(m_PrioVoiceList.iterator_to(*voice));
    m_FreeVoiceList.push_back(*voice);
}

void MultiVoiceManager::ChangeVoicePriority(MultiVoice* voice) {
    m_PrioVoiceList.erase(m_PrioVoiceList.iterator_to(*voice));
    AppendVoiceList(voice);
}

int32_t MultiVoiceManager::DropLowestPriorityVoice(int32_t priority) {
    if (m_PrioVoiceList.empty())
        return 0;

    MultiVoice* dropVoice{&m_PrioVoiceList.front()};
    if (dropVoice->GetPriority() == MultiVoice::PriorityNoDrop ||
        priority < dropVoice->GetPriority())
        return 0;

    int32_t dropCount{dropVoice->GetSdkVoiceCount()};

    MultiVoice::VoiceCallback callbackFunc{dropVoice->m_Callback};
    void* callbackData{dropVoice->m_pCallbackData};

    dropVoice->Stop();
    dropVoice->Free();
    if (callbackFunc != nullptr)
        callbackFunc(dropVoice, MultiVoice::VoiceCallbackStatus_DropVoice, callbackData);

    return dropCount;
}

}  // namespace nn::atk::detail::driver
