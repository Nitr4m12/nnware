#pragma once

#include <nn/util.h>
#include <nn/util/util_IntrusiveList.h>

#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_OutputReceiver.h>

namespace nn::atk::detail {

template <typename T>
class SoundInstanceManager {
public:
    using PriorityList =
        util::IntrusiveList<T, util::IntrusiveListMemberNodeTraits<T, &T::m_PriorityLink>>;
    using Iterator = typename PriorityList::iterator;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
    T* Alloc(int32_t priority, int32_t ambientPriority);
#else
    T* Alloc(int32_t priority, int32_t ambientPriority, OutputReceiver* pOutputReceiver);
#endif
    int32_t Create(void* buffer, size_t size, const SoundInstanceConfig& config);

    void SortPriorityList();

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
