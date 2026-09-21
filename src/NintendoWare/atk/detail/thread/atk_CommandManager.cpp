#include <nn/atk/atk_CommandManager.h>

namespace nn::atk::detail {

CommandBuffer::CommandBuffer() = default;

CommandBuffer::~CommandBuffer() = default;

void CommandBuffer::Finalize() {}

void CommandBuffer::Initialize(void* commandBuffer, size_t commandBufferSize) {
    m_CommandMemoryAreaBegin = 0;
    m_CommandMemoryAreaEnd = 0;
    m_CommandMemoryAreaZeroFlag = false;

    m_CommandMemoryArea = static_cast<u32*>(commandBuffer);
    m_CommandMemoryAreaSize = commandBufferSize / sizeof(u32);
}

// NON_MATCHING
void* CommandBuffer::AllocMemory(size_t size) {
    if (m_CommandMemoryAreaZeroFlag)
        return nullptr;

    void* ptr{};

    const size_t count{(size + 3) / 4};

    volatile uintptr_t curEnd{m_CommandMemoryAreaEnd};

    if (count + m_CommandMemoryAreaBegin > curEnd)
        return nullptr;

    if (m_CommandMemoryAreaBegin < curEnd) {
        ptr = count + m_CommandMemoryArea;
        m_CommandMemoryAreaBegin = count + m_CommandMemoryAreaBegin;
    } else if (m_CommandMemoryAreaSize < m_CommandMemoryAreaBegin + count) {
        if (count > curEnd)
            return nullptr;

        ptr = m_CommandMemoryArea;
        m_CommandMemoryAreaBegin = count;
    }

    Command* command{static_cast<Command*>(ptr)};
    if (command != nullptr) {
        if (count == curEnd) {
            m_CommandMemoryAreaZeroFlag = true;
        }
        command->memory_next = count;
    }

    return command;
}

void CommandBuffer::FreeMemory(Command* lastCommand) {
    m_CommandMemoryAreaEnd = lastCommand->memory_next;
    m_CommandMemoryAreaZeroFlag = false;
}

size_t CommandBuffer::GetCommandBufferSize() const {
    return m_CommandMemoryAreaSize * sizeof(u32);
}

// NON_MATCHING
size_t CommandBuffer::GetAllocatableCommandSize() const {
    if (m_CommandMemoryAreaZeroFlag)
        return 0;

    size_t count{m_CommandMemoryAreaSize};

    volatile uintptr_t curBegin{m_CommandMemoryAreaBegin};
    volatile uintptr_t curEnd{m_CommandMemoryAreaEnd};

    if (curBegin < curEnd)
        return (curEnd - curBegin) * 4;

    if (curEnd > m_CommandMemoryAreaSize - curBegin)
        return 0;

    return (m_CommandMemoryAreaSize - curBegin) * 4;
}

}  // namespace nn::atk::detail
