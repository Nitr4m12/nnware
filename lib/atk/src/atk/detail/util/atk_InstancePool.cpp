#include <nn/atk/atk_InstancePool.h>

#include <nn/util/util_BytePtr.h>

#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail {

int PoolImpl::CreateImpl(void* buffer, size_t size, size_t objSize) {
    return CreateImpl(buffer, size, objSize, DefaultAlignment);
}

int PoolImpl::CreateImpl(void* buffer, size_t size, size_t objSize, size_t alignment) {
    char* ptr{util::BytePtr(buffer).AlignUp(alignment).Get<char>()};
    objSize = util::align_up(objSize, alignment);
    int objectCount = (fnd::GetOffsetFromPtr(ptr, buffer) + size) / objSize;

    for (int i{0}; i < objectCount; ++i) {
        PoolImpl* head{reinterpret_cast<PoolImpl*>(ptr)};
        head->m_pNext = m_pNext;
        m_pNext = head;

        ptr += objSize;
    }

    m_pBuffer = buffer;
    m_BufferSize = size;

    return objectCount;
}

void PoolImpl::DestroyImpl() {
    void* begin{util::BytePtr(m_pBuffer).Get()};
    void* end{util::BytePtr(m_pBuffer, m_BufferSize).Get()};

    PoolImpl* ptr{m_pNext};
    PoolImpl* prev{this};

    for (; ptr != nullptr; ptr = ptr->m_pNext) {
        if (ptr >= begin && ptr < end)
            prev->m_pNext = ptr->m_pNext;
        else
            prev = ptr;
    }
}

int PoolImpl::CountImpl() const {
    int count{0};

    for (PoolImpl* ptr{m_pNext}; ptr != nullptr; ptr = ptr->m_pNext)
        ++count;

    return count;
}

void* PoolImpl::AllocImpl() {
    PoolImpl* head{m_pNext};

    if (head != nullptr)
        m_pNext = head->m_pNext;

    return head;
}

void PoolImpl::FreeImpl(void* ptr) {
    PoolImpl* head{reinterpret_cast<PoolImpl*>(ptr)};

    head->m_pNext = m_pNext;
    m_pNext = head;
}

}  // namespace nn::atk::detail
