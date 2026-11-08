#include <nn/atk/atk_FrameHeap.h>

#include <cstring>

#include <nn/util/util_BytePtr.h>

#include <nn/atk/fnd/basis/atkfnd_Inlines.h>

namespace nn::atk::detail {

FrameHeap::FrameHeap() = default;

FrameHeap::~FrameHeap() {
    Destroy();
}

bool FrameHeap::Create(void* startAddress, size_t size) {
    if (IsValid())
        Destroy();

    void* endAddress{util::BytePtr(startAddress, size).Get()};
    startAddress = util::BytePtr(startAddress).AlignUp(fnd::HeapBase::DefaultAlignment).Get();

    if (startAddress <= endAddress) {
        m_pHeap = fnd::FrameHeapImpl::Create(startAddress,
                                             fnd::GetOffsetFromPtr(startAddress, endAddress), 0);

        if (m_pHeap != nullptr)
            return NewSection();
    }

    return false;
}

void FrameHeap::Destroy() {
    if (IsValid()) {
        ClearSection();
        m_pHeap->Free(fnd::FrameHeapImpl::FreeAllMode);
        m_pHeap->Destroy();
        m_pHeap = nullptr;
    }
}

void* FrameHeap::Alloc(size_t size, DisposeCallback callback, void* callbackArg,
                       HeapCallback heapCallback, void* heapCallbackArg) {
    const size_t blockSize{sizeof(Block)};
    const size_t allocSize{(size + 127) & ~63};

    void* mem{m_pHeap->Alloc(allocSize, blockSize)};

    if (mem == nullptr)
        return nullptr;

    void* buffer{util::BytePtr(mem, blockSize).Get()};

    Block* block{
        new (mem) Block(buffer, size, callback, callbackArg, heapCallback, heapCallbackArg),
    };
    m_SectionList.back().AppendBlock(block);

    return buffer;
}

void FrameHeap::Clear() {
    ClearSection();
    m_pHeap->Free(fnd::FrameHeapImpl::FreeAllMode);
    [[maybe_unused]] bool result = NewSection();
}

int FrameHeap::SaveState() {
    if (!m_pHeap->RecordState(m_SectionList.size()))
        return -1;

    if (!NewSection()) {
        m_pHeap->FreeByState(0);
        return -1;
    }

    int result{m_SectionList.size() - 1};
    return result;
}

void FrameHeap::LoadState(int level) {
    if (level == 0) {
        Clear();
        return;
    }

    bool alreadyUseCallback{ProcessCallback(level)};
    while (m_SectionList.size() > level) {
        auto& section{m_SectionList.back()};

        if (alreadyUseCallback)
            section.SetUseCallback(false);

        section.~Section();
        m_SectionList.erase(m_SectionList.iterator_to(section));
    }

    [[maybe_unused]] int32_t result{m_pHeap->FreeByState(level)};
    m_pHeap->RecordState(m_SectionList.size());
    [[maybe_unused]] bool result2{NewSection()};
}

bool FrameHeap::NewSection() {
    void* buffer{m_pHeap->Alloc(sizeof(Section), fnd::HeapBase::DefaultAlignment)};

    if (buffer != nullptr) {
        std::memset(buffer, 0, sizeof(Section));
        auto* section{new (buffer) Section};
        m_SectionList.push_back(*section);

        return true;
    }

    return false;
}

int32_t FrameHeap::GetCurrentLevel() const {
    return m_SectionList.size() - 1;
}

size_t FrameHeap::GetSize() const {
    return fnd::GetOffsetFromPtr(m_pHeap, m_pHeap->GetHeapEndAddress());
}

size_t FrameHeap::GetFreeSize() const {
    size_t size{m_pHeap->GetAllocatableSize(HeapAlign)};
    size = size < HeapAlign ? 0 : size - HeapAlign & ~(HeapAlign - 1);

    return size;
}

void FrameHeap::ClearSection() {
    bool alreadyUseCallback{ProcessCallback(0)};

    while (!m_SectionList.empty()) {
        auto& section{m_SectionList.back()};

        if (alreadyUseCallback) {
            section.SetUseCallback(false);
        }

        section.~Section();
        m_SectionList.erase(m_SectionList.iterator_to(section));
    }
}

void FrameHeap::Dump([[maybe_unused]] const SoundDataManager& mgr,
                     [[maybe_unused]] const SoundArchive& arc) const {}

FrameHeap::Section::Section() = default;

FrameHeap::Section::~Section() {
    if (m_UseCallback) {
        for (auto itr{m_BlockList.end()}; itr != m_BlockList.begin();) {
            --itr;
            itr->~Block();
        }
    }
    m_BlockList.clear();
}

void FrameHeap::Section::AppendBlock(Block* block) {
    m_BlockList.push_back(*block);
}

void FrameHeap::Section::Dump([[maybe_unused]] const SoundDataManager& mgr,
                              [[maybe_unused]] const SoundArchive& arc) const {}

}  // namespace nn::atk::detail
