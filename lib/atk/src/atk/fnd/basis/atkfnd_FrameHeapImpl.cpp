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

void* FrameHeapImpl::AllocFromHead(size_t size, int alignment) {
    void* newBlock{RoundUp(m_pHeadAllocator, alignment)};
    void* endAddress{AddOffsetToPtr(newBlock, size)};

    if (endAddress > m_pTailAllocator)
        return nullptr;

    FillAllocMemory(m_pHeadAllocator,
                    static_cast<uint32_t>(GetOffsetFromPtr(m_pHeadAllocator, endAddress)));
    m_pHeadAllocator = endAddress;
    return newBlock;
}

void* FrameHeapImpl::AllocFromTail(size_t size, int alignment) {
    void* newBlock{
        RoundDown(AddOffsetToPtr(m_pTailAllocator, static_cast<int32_t>(-size)), alignment),
    };
    if (newBlock < m_pHeadAllocator)
        return nullptr;

    size = static_cast<uint32_t>(GetOffsetFromPtr(newBlock, m_pTailAllocator));
    FillAllocMemory(newBlock, size);
    m_pTailAllocator = newBlock;
    return newBlock;
}

}  // namespace nn::atk::detail::fnd
