#pragma once

#include <nn/util.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_OutputReceiver.h>

namespace nn::atk::detail {

template <typename Sound>
class SoundInstanceManager {
public:
    using PriorityList = util::IntrusiveList<
        Sound, util::IntrusiveListMemberNodeTraits<Sound, &Sound::m_PriorityLink>>;
    using Iterator = typename PriorityList::iterator;

    SoundInstanceManager() = default;
    ~SoundInstanceManager() = default;

    // UNCHECKED
    static size_t GetObjectSize(const SoundInstanceConfig& config) {
        size_t result{sizeof(Sound)};

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
        if (AdditionalParamBufferSize != 0) {
            result = sizeof(Sound) + sizeof(OutputAdditionalParam) + 8;
            result *= AdditionalParamBufferSize * 2;
        }
#endif

        return result;
    }

    // UNCHECKED
    static size_t GetRequiredMemSize(int instanceCount, const SoundInstanceConfig& config) {
        return GetObjectSize(config) * instanceCount;
    }

    // TODO
    int32_t Create(void* buffer, size_t size, const SoundInstanceConfig& config) {
        const size_t TotalObjectSize{GetObjectSize(config)};
        const int ObjectCount{size / TotalObjectSize};

        uint8_t* ptr{reinterpret_cast<uint8_t*>(buffer)};
        uint8_t* soundPtr{ptr};
        uint8_t* additionalParamPtr{ptr};

        for (int i{0}; i < ObjectCount; ++i) {
            Sound* sound{reinterpret_cast<Sound*>(soundPtr)};
            m_PriorityList.push_back(*sound);
            soundPtr += sizeof(Sound);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
            const size_t AdditionalParamBufferSize{
                OutputAdditionalParam::GetRequiredMemSize(config),
            };

            if (AdditionalParamBufferSize != 0) {
                [[maybe_unused]] const size_t AdditionalParamSize{sizeof(OutputAdditionalParam)};

                OutputAdditionalParam* pAdditionalParam{
                    reinterpret_cast<OutputAdditionalParam*>(additionalParamPtr),
                };
                new (pAdditionalParam) OutputAdditionalParam;

                [[maybe_unused]] void* pAdditionalParamBuffer;
                OutputAdditionalParam* pAdditionalParamForPlayer;
                new (pAdditionalParamForPlayer) OutputAdditionalParam;
                [[maybe_unused]] const void* pAdditionalParamBufferForPlayer;
            }
#endif
        }

        return ObjectCount;
    }

    // UNCHECKED
    void Destroy() {
        char* ptr{reinterpret_cast<char*>(m_pBuffer)};
        const size_t TotalObjectSize{GetObjectSize(m_SoundInstanceConfig)};
        const int objectCount{m_BufferSize / TotalObjectSize};

        if (m_FreeList.empty())
            return;

        for (int i{0}; i < objectCount; ++i) {
            Sound* sound{reinterpret_cast<Sound*>(ptr)};
            m_FreeList.iterator_to(*sound)->Finalize();

            ptr += i * sizeof(sound);
        }

        m_FreeList.clear();
    }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    Sound* Alloc(int32_t priority, int32_t ambientPriority);
#else
    Sound* Alloc(int32_t priority, int32_t ambientPriority, OutputReceiver* pOutputReceiver);
#endif

    void Free(Sound* sound) {
        RemovePriorityList(sound);
        sound->Finalize();
        m_FreeList.push_back(*sound);
    }

    void UpdatePriority(Sound* sound, int priority) {
        RemovePriorityList(sound);
        InsertPriorityList(sound, priority);
    }
    void SortPriorityList();

    Sound* GetLowestPrioritySound();

    int GetActiveCount() const { m_PriorityList.size(); }
    int GetFreeCount() const { m_FreeList.size(); }

    const PriorityList& GetSoundList() const { return m_PriorityList; }
    PriorityList& GetFreeList() { return m_FreeList; }

    void InsertPriorityList(Sound* sound, int priority) {
        Iterator itr{m_PriorityList.begin()};
        while (itr != m_PriorityList.end()) {
            if (itr->CalcCurrentPlayerPriority() > priority)
                break;
            ++itr;
        }

        m_PriorityList.insert(itr, *sound);
    }

    void RemovePriorityList(Sound* sound) {
        m_PriorityList.erase(m_PriorityList.iterator_to(*sound));
    }

private:
    void* m_pBuffer;
    size_t m_BufferSize;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    SoundInstanceConfig m_SoundInstanceConfig;
#endif
    PriorityList m_PriorityList;
    PriorityList m_FreeList;
};

}  // namespace nn::atk::detail
