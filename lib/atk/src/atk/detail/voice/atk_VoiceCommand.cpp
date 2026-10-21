#include <nn/atk/atk_VoiceCommand.h>

namespace nn::atk::detail {

LowLevelVoiceCommand& LowLevelVoiceCommand::GetInstance() {
    static LowLevelVoiceCommand instance;
    return instance;
}

void LowLevelVoiceCommand::Initialize(void* commandBuffer, size_t commandBufferSize,
                                      void* waveBuffer, [[maybe_unused]] size_t waveBufferSize,
                                      int32_t waveBufferPacketCount) {
    m_WaveBufferPacketCount = waveBufferPacketCount;
    m_pWaveBufferPacket = static_cast<WaveBufferPacket*>(waveBuffer);

    for (int i{0}; i < m_WaveBufferPacketCount; ++i) {
        m_pWaveBufferPacket[i].adpcmContext = {};
        m_pWaveBufferPacket[i].waveBuffer.Initialize();
    }

    CommandManager::Initialize(commandBuffer, commandBufferSize, ProcessCommandList);
}

}  // namespace nn::atk::detail
