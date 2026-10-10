#pragma once

#include <nn/util.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_OutputAdditionalParam.h>
#include <nn/atk/atk_OutputReceiver.h>
#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail {

template <typename Sound>
class SoundInstanceManager {
public:
    using PriorityList =
        util::IntrusiveList<Sound,
                            util::IntrusiveListMemberNodeTraits<Sound, &Sound::m_PriorityLink>>;
    using Iterator = typename PriorityList::iterator;

    SoundInstanceManager() = default;
    ~SoundInstanceManager() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static size_t GetObjectSize() {
        size_t result{sizeof(Sound)};
        return result;
    }
#else
    static size_t GetObjectSize(const SoundInstanceConfig& config) {
        size_t result{sizeof(Sound)};

        const size_t AdditionalParamBufferSize{OutputAdditionalParam::GetRequiredMemSize(config)};
        if (AdditionalParamBufferSize != 0) {
            result += sizeof(OutputAdditionalParam) * 2;
            result += AdditionalParamBufferSize * 2;
        }

        return result;
    }
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    static size_t GetRequiredMemSize(int instanceCount) { return GetObjectSize() * instanceCount; }
#else
    static size_t GetRequiredMemSize(int instanceCount, const SoundInstanceConfig& config) {
        return GetObjectSize(config) * instanceCount;
    }
#endif

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    int32_t Create(void* buffer, size_t size)
#else
    int32_t Create(void* buffer, size_t size, const SoundInstanceConfig& config)
#endif
    {
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        const size_t TotalObjectSize{GetObjectSize()};
#else
        const size_t TotalObjectSize{GetObjectSize(config)};
#endif
        const int ObjectCount{static_cast<int>(size / TotalObjectSize)};

        uint8_t* ptr{reinterpret_cast<uint8_t*>(buffer)};
        uint8_t* soundPtr{ptr};
        [[maybe_unused]] uint8_t* additionalParamPtr{ptr + ObjectCount * sizeof(Sound)};

        for (int i{0}; i < ObjectCount; ++i) {
            Sound* sound{new (soundPtr) Sound(*this)};
            m_FreeList.push_back(*sound);

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
            const size_t AdditionalParamBufferSize{
                OutputAdditionalParam::GetRequiredMemSize(config),
            };

            if (AdditionalParamBufferSize != 0) {
                const size_t AdditionalParamSize{sizeof(OutputAdditionalParam)};

                OutputAdditionalParam* pAdditionalParam;
                pAdditionalParam = new (additionalParamPtr) OutputAdditionalParam;
                additionalParamPtr += AdditionalParamSize;
                void* pAdditionalParamBuffer{additionalParamPtr};
                pAdditionalParam->Initialize(pAdditionalParamBuffer, AdditionalParamBufferSize,
                                             config);
                additionalParamPtr += AdditionalParamBufferSize;

                OutputAdditionalParam* pAdditionalParamForPlayer;
                pAdditionalParamForPlayer = new (additionalParamPtr) OutputAdditionalParam;
                additionalParamPtr += AdditionalParamSize;
                void* pAdditionalParamBufferForPlayer{additionalParamPtr};
                pAdditionalParamForPlayer->Initialize(pAdditionalParamBufferForPlayer,
                                                      AdditionalParamBufferSize, config);
                additionalParamPtr += AdditionalParamBufferSize;

                sound->SetOutputAdditionalParamAddr(OutputDevice_Main, pAdditionalParam,
                                                    pAdditionalParamForPlayer);
            }
#endif

            soundPtr += sizeof(Sound);
        }

        m_pBuffer = buffer;
        m_BufferSize = size;
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
        m_SoundInstanceConfig = config;
#endif

        return ObjectCount;
    }

    void Destroy() {
        char* ptr{static_cast<char*>(m_pBuffer)};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        const size_t TotalObjectSize{GetObjectSize()};
#else
        const size_t TotalObjectSize{GetObjectSize(m_SoundInstanceConfig)};
#endif
        const int objectCount{static_cast<int>(m_BufferSize / TotalObjectSize)};

        for (int i{0}; i < objectCount; ++i) {
            Sound* sound{reinterpret_cast<Sound*>(ptr)};
            sound->~Sound();

            ptr += sizeof(Sound);
        }

        m_FreeList.clear();
        m_PriorityList.clear();
    }

    // NON_MATCHING
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    Sound* Alloc(int32_t priority, int32_t ambientPriority)
#else
    Sound* Alloc(int32_t priority, int32_t ambientPriority, OutputReceiver* pOutputReceiver)
#endif
    {
        int allocPriority{priority + ambientPriority};
        allocPriority = fnd::Clamp(allocPriority, PlayerPriorityMin, PlayerPriorityMax);

        Sound* sound;

        while (!m_FreeList.empty()) {
            sound = &m_FreeList.front();
            m_FreeList.pop_front();

            if (!m_FreeList.empty())
                continue;

            Sound* lowPrioSound{GetLowestPrioritySound()};
            if (lowPrioSound == nullptr)
                return nullptr;

            if (allocPriority < lowPrioSound->CalcCurrentPlayerPriority())
                return nullptr;

            Debug_GetWarningFlag(
                Debug_GetDebugWarningFlagFromSoundType(lowPrioSound->GetSoundType()));
            lowPrioSound->Stop(0);
        }

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        bool result{sound->Initialize()};
#else
        bool result{sound->Initialize(pOutputReceiver)};
#endif

        if (!result) {
            m_FreeList.push_back(*sound);
            return nullptr;
        }

        sound->SetPriority(priority, ambientPriority);
        InsertPriorityList(sound, allocPriority);

        return sound;
    }

    void Free(Sound* sound) {
        RemovePriorityList(sound);
        sound->Finalize();
        m_FreeList.push_back(*sound);
    }

    void UpdatePriority(Sound* sound, int priority) {
        RemovePriorityList(sound);
        InsertPriorityList(sound, priority);
    }

    void SortPriorityList() {
        if (m_PriorityList.size() <= 1)
            return;

        const int TmpCount{PlayerPriorityMax + 1};
        PriorityList tmplist[TmpCount];

        while (!m_PriorityList.empty()) {
            Sound& front{m_PriorityList.front()};
            m_PriorityList.pop_front();
            tmplist[front.CalcCurrentPlayerPriority()].push_back(front);
        }

        for (int i{0}; i < TmpCount; ++i) {
            while (!tmplist[i].empty()) {
                Sound& front{tmplist[i].front()};
                tmplist[i].pop_front();
                m_PriorityList.push_back(front);
            }
        }
    }

    Sound* GetLowestPrioritySound() {
        if (m_PriorityList.empty())
            return nullptr;

        return &m_PriorityList.front();
    }

    int GetActiveCount() const { return m_PriorityList.size(); }
    int GetFreeCount() const { return m_FreeList.size(); }

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
    void* m_pBuffer{};
    size_t m_BufferSize{0};
#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
    SoundInstanceConfig m_SoundInstanceConfig;
#endif
    PriorityList m_PriorityList;
    PriorityList m_FreeList;
};

}  // namespace nn::atk::detail
