#pragma once

#include <cstddef>
#include <cstdint>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk::detail::fnd {

class HeapBase : public util::IntrusiveListBaseNode<HeapBase> {
public:
    using HeapList = util::IntrusiveList<HeapBase, util::IntrusiveListBaseNodeTraits<HeapBase>>;

    enum HeapType {
        HeapType_Exp,
        HeapType_Frame,
        HeapType_Unit,
        HeapType_Unknown,
    };

    enum FillType {
        FillType_NoUse,
        FillType_Alloc,
        FillType_Free,
        FillType_Max,
    };

    static const int DefaultAlignment = 4;

    static const uint32_t ExpHeapSignature = 0x45585048;    // HPXE
    static const uint32_t FrameHeapSignature = 0x46524D48;  // HMRF
    static const uint32_t UnitHeapSignature = 0x554E5448;   // HTNU

    static const int OptionZeroClear = 1 << 0;
    static const int OptionDebugFill = 1 << 1;
    static const int OptionThreadSafe = 1 << 2;

    static const int ErrorPrint = 1;

    static const int MIN_ALIGNMENT = DefaultAlignment;

    static HeapBase* FindContainHeap(const void* memBlock);
    static HeapBase* FindParentHeap(const HeapBase* pChild);

    void* GetHeapStartAddress();
    void* GetHeapEndAddress();

    size_t GetTotalSize();
    size_t GetTotalUsableSize();

    uint32_t SetFillValue(FillType type, uint32_t val);
    uint32_t GetFillValue(FillType type);

    HeapType GetHeapType();

protected:
    void Initialize(uint32_t signature, void* heapStart, void* heapEnd, uint16_t optFlag);
    void Finalize();

    uint32_t GetSignature() const { return m_Signature; }

    void* GetHeapStart() const { return mHeapStart; }

    void* GetHeapEnd() const { return mHeapEnd; }

    void LockHeap();
    void UnlockHeap();

    void FillFreeMemory(void* address, size_t size);
    void FillNoUseMemory(void* address, size_t size);
    void FillAllocMemory(void* address, size_t size);

private:
    static HeapBase* FindContainHeap(HeapList* pList, const void* memBlock);
    static HeapList* FindListContainHeap(HeapBase* pHeapBase);

    uint16_t GetOptionFlag();
    void SetOptionFlag(uint16_t optFlag);

    void* mHeapStart;
    void* mHeapEnd;
    uint32_t m_Signature;
    HeapList m_ChildList;
    uint32_t m_Attribute;
};
static_assert(sizeof(HeapBase) == 0x40);

}  // namespace nn::atk::detail::fnd
