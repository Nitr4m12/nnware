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

    // UNCHECKED
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

    // UNCHECKED
    void Destroy() {
        char* ptr{reinterpret_cast<char*>(m_pBuffer)};
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
        const size_t TotalObjectSize{GetObjectSize()};
#else
        const size_t TotalObjectSize{GetObjectSize(m_SoundInstanceConfig)};
#endif
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
