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

}  // namespace nn::atk::detail::fnd
