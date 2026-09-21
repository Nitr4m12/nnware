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

void CommandBuffer::FreeMemory(Command* lastCommand) {
    m_CommandMemoryAreaEnd = lastCommand->memory_next;
    m_CommandMemoryAreaZeroFlag = false;
}

}  // namespace nn::atk::detail
