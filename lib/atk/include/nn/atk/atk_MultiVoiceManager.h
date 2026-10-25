#pragma once

#include <nn/atk/atk_MultiVoice.h>

namespace nn::atk::detail::driver {

class MultiVoiceManager {
public:
    using VoiceList = util::IntrusiveList<
        MultiVoice, util::IntrusiveListMemberNodeTraits<MultiVoice, &MultiVoice::m_LinkNode>>;

    static MultiVoiceManager& GetInstance();

    size_t GetObjectSize(const SoundInstanceConfig& config);
    size_t GetRequiredMemSize(int32_t voiceCount, const SoundInstanceConfig& config);

    void Initialize(void* mem, size_t memSize, const SoundInstanceConfig& config);
    void Finalize();

    MultiVoice* AllocVoice(int32_t voiceChannelCount, int32_t priority,
                           MultiVoice::VoiceCallback callback, void* callbackData);
    void FreeVoice(MultiVoice* voice);

    void StopAllVoices();

    void UpdateAllVoices();
    void UpdateAllVoiceStatus();

    void UpdateAudioFrameVoices();
    void UpdateAudioFrameVoiceStatus();

    void UpdateAllVoicesSync(uint32_t syncFlag);

    int32_t GetVoiceCount() const;
    size_t GetActiveCount() const;
    size_t GetFreeCount() const;

    VoiceList* GetVoiceList() const;

private:
    MultiVoiceManager();

    void ChangeVoicePriority(MultiVoice* voice);
    bool DropLowestPriorityVoice(int32_t priority);

    void UpdateEachVoicePriority(const VoiceList::iterator&, const VoiceList::iterator&);

    void AppendVoiceList(MultiVoice* voice);
    void RemoveVoiceList(MultiVoice* voice);

    bool m_Initialized{false};
    VoiceList m_PrioVoiceList;
    VoiceList m_FreeVoiceList;
};
static_assert(sizeof(MultiVoiceManager) == 0x28);

}  // namespace nn::atk::detail::driver
