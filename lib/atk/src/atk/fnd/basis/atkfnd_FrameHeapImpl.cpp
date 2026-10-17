#include <nn/atk/fnd/basis/atkfnd_FrameHeapImpl.h>

#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail::fnd {

FrameHeapImpl* FrameHeapImpl::Create(void* startAddress, size_t size, uint16_t optFlag) {
    void* endAddress{startAddress};

    endAddress = RoundDown(AddOffsetToPtr(endAddress, size), HeapBase::DefaultAlignment);
    startAddress = RoundUp(startAddress, HeapBase::DefaultAlignment);

    if (startAddress <= endAddress &&
        GetOffsetFromPtr(startAddress, endAddress) >= sizeof(FrameHeapImpl)) {
        FrameHeapImpl* pHeap{new (startAddress) FrameHeapImpl};
        pHeap->Initialize(HeapBase::FrameHeapSignature,
                          AddOffsetToPtr(pHeap, sizeof(FrameHeapImpl)), endAddress, optFlag);
        pHeap->m_pHeadAllocator = pHeap->GetHeapStart();
        pHeap->m_pTailAllocator = pHeap->GetHeapEnd();
        pHeap->m_pState = nullptr;
        return pHeap;
    }

    return nullptr;
}

void* FrameHeapImpl::Destroy() {
    HeapBase::Finalize();
    return this;
}

// NON_MATCHING: inlining of AllocFromHead and AllocFromTail
void* FrameHeapImpl::Alloc(size_t size, int alignment) {
    void* memory{};

    if (size == 0)
        size = HeapBase::DefaultAlignment;
    else
        size = RoundUp(size, HeapBase::DefaultAlignment);

    LockHeap();
    if (alignment >= 0)
        memory = AllocFromHead(size, alignment);
    else
        memory = AllocFromTail(size, alignment);
    UnlockHeap();

    return memory;
}

void* FrameHeapImpl::AllocFromHead(size_t size, int alignment) {
    void* newBlock{RoundUp(m_pHeadAllocator, alignment)};
    void* endAddress{AddOffsetToPtr(newBlock, size)};

    if (endAddress > m_pTailAllocator)
        return nullptr;

    size = static_cast<uint32_t>(GetOffsetFromPtr(m_pHeadAllocator, endAddress));
    FillAllocMemory(m_pHeadAllocator, size);
    m_pHeadAllocator = endAddress;
    return newBlock;
}

void* FrameHeapImpl::AllocFromTail(size_t size, int alignment) {
    void* newBlock{RoundDown(AddOffsetToPtr(m_pTailAllocator, -static_cast<int32_t>(size)), alignment)};
    if (newBlock < m_pHeadAllocator)
        return nullptr;

    size = static_cast<uint32_t>(GetOffsetFromPtr(newBlock, m_pTailAllocator));
    FillAllocMemory(newBlock, size);
    m_pTailAllocator = newBlock;
    return newBlock;
}

size_t FrameHeapImpl::ResizeForMBlock(void *memBlock, size_t newSize) {
    if (newSize == 0)
        newSize = HeapBase::DefaultAlignment;
    else
        newSize = RoundUp(newSize, HeapBase::DefaultAlignment);

    {
        HeapBase::LockHeap();

        const ptrdiff_t oldSize {GetOffsetFromPtr(memBlock, m_pHeadAllocator)};
        void* endAddress{AddOffsetToPtr(memBlock, newSize)};
        if (static_cast<int32_t>(newSize) != oldSize) {
            if (newSize > static_cast<uint32_t>(oldSize)) {
                if (endAddress <= m_pTailAllocator)
                    FillAllocMemory(m_pHeadAllocator, newSize - oldSize);
                else
                    newSize = 0;
            } else {
                FillFreeMemory(endAddress, oldSize - newSize);
            }
            m_pHeadAllocator = endAddress;
        }
        HeapBase::UnlockHeap();
    }

    return newSize;
}

}  // namespace nn::atk::detail::fnd
