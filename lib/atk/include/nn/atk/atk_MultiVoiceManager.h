#pragma once

#include <nn/atk/atk_MultiVoice.h>

namespace nn::atk::detail::driver {

class MultiVoiceManager {
public:
    using VoiceList = util::IntrusiveList<
        MultiVoice, util::IntrusiveListMemberNodeTraits<MultiVoice, &MultiVoice::m_LinkNode>>;

    MultiVoiceManager();

    size_t GetObjectSize(const SoundInstanceConfig& config);
    size_t GetRequiredMemSize(int32_t voiceCount, const SoundInstanceConfig& config);

    void Initialize(void* mem, size_t memSize, const SoundInstanceConfig& config);
    void Finalize();

    void StopAllVoices();

    MultiVoice* AllocVoice(int32_t voiceChannelCount, int32_t priority,
                           MultiVoice::VoiceCallback callback, void* callbackData);

    bool DropLowestPriorityVoice(int32_t);

    void AppendVoiceList(MultiVoice* voice);

    void FreeVoice(MultiVoice* voice);

    void RemoveVoiceList(MultiVoice* voice);

    void UpdateAllVoiceStatus();
    void UpdateAudioFrameVoiceStatus();
    void UpdateAllVoices();
    void UpdateAudioFrameVoices();

    void ChangeVoicePriority(MultiVoice* voice);

    void UpdateAllVoicesSync(uint32_t syncFlag);

    int32_t GetVoiceCount() const;
    int32_t GetActiveCount() const;
    int32_t GetFreeCount() const;

    VoiceList* GetVoiceList() const;

    static MultiVoiceManager* GetInstance();

private:
    bool m_Initialized;
    VoiceList m_PrioVoiceList;
    VoiceList m_FreeVoiceList;
};
static_assert(sizeof(MultiVoiceManager) == 0x28);

}  // namespace nn::atk::detail::driver
