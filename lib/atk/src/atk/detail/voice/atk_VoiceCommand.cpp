#include <nn/atk/atk_VoiceCommand.h>

#include <nn/util/util_BitUtil.h>

#include <nn/atk/atk_Voice.h>

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

size_t LowLevelVoiceCommand::GetRequiredWaveBufferMemSize(int32_t waveBufferPacketCount) {
    size_t result{waveBufferPacketCount * sizeof(WaveBufferPacket)};
    result = util::align_up(result, 32);
    return result;
}

void LowLevelVoiceCommand::LowLevelVoiceDisposeCallback([[maybe_unused]] LowLevelVoice* voice,
                                                        void* arg) {
    uint32_t voiceId{static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg))};
    VirtualVoiceManager::GetInstance().SetLowLevelVoice(voiceId, nullptr);
}

LowLevelVoiceCommand::WaveBufferPacket* LowLevelVoiceCommand::GetFreeWaveBuffer() {
    for (int i{0}; i < m_WaveBufferPacketCount; ++i) {
        WaveBufferPacket* packet{&m_pWaveBufferPacket[i]};
        switch (packet->waveBuffer.status) {
        case WaveBuffer::Status_Free:
        case WaveBuffer::Status_Done:
            return packet;
        case WaveBuffer::Status_Play:
        case WaveBuffer::Status_Wait:
            break;
        }
    }

    return nullptr;
}

void LowLevelVoiceCommand::ProcessCommandList(Command* commandList) {
    Command* command{commandList};
    while (command != nullptr) {
        switch (command->id) {
        case VoiceCommandId_Alloc: {
            auto* c{static_cast<VoiceCommandAlloc*>(command)};
            LowLevelVoice* lowLevelVoice{
                driver::HardwareManager::GetInstance().GetLowLevelVoiceAllocator().AllocVoice(),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->SetPriority(c->priority);
            lowLevelVoice->SetVoice(static_cast<Voice*>(c->userId));
            VirtualVoiceManager::GetInstance().SetLowLevelVoice(c->voiceId, lowLevelVoice);
            break;
        }
        case VoiceCommandId_Free: {
            auto* c{static_cast<VoiceCommandFree*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            driver::HardwareManager::GetInstance().GetLowLevelVoiceAllocator().FreeVoice(
                lowLevelVoice);
            VirtualVoiceManager::GetInstance().SetLowLevelVoice(c->voiceId, nullptr);
            break;
        }
        case VoiceCommandId_Priority: {
            auto* c{static_cast<VoiceCommandPriority*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->SetPriority(c->priority);
            break;
        }
        case VoiceCommandId_Play: {
            auto* c{static_cast<VoiceCommandPlay*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->SetSampleRate(c->sampleRate);
            lowLevelVoice->SetSampleFormat(c->sampleFormat);
            lowLevelVoice->SetAdpcmParam(c->adpcmParam);
#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
            lowLevelVoice->SetSubMixIndex(c->subMixIndex);
#else
            lowLevelVoice->SetOutputReceiver(c->pOutputReceiver);
#endif
            lowLevelVoice->SetState(VoiceState_Play);

            break;
        }
        case VoiceCommandId_Pause: {
            auto* c{static_cast<VoiceCommandPause*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->SetState(VoiceState_Pause);
            break;
        }
        case VoiceCommandId_Stop: {
            auto* c{static_cast<VoiceCommandFree*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->FreeAllWaveBuffer();
            lowLevelVoice->SetState(VoiceState_Stop);
            break;
        }
        case VoiceCommandId_AppendWaveBuffer: {
            auto* c{static_cast<VoiceCommandAppendWaveBuffer*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            WaveBufferPacket* packet{GetInstance().GetFreeWaveBuffer()};
            if (packet == nullptr)
                break;

            WaveBuffer* waveBuffer{&packet->waveBuffer};
            waveBuffer->bufferAddress = c->bufferAddress;
            waveBuffer->bufferSize = c->bufferSize;
            waveBuffer->sampleLength = c->sampleLength;
            waveBuffer->sampleOffset = c->sampleOffset;
            if (c->adpcmContextEnable) {
                packet->adpcmContext = c->adpcmContext;
                waveBuffer->pAdpcmContext = &packet->adpcmContext;
            } else {
                waveBuffer->pAdpcmContext = nullptr;
            }
            waveBuffer->loopFlag = c->loopFlag;
            waveBuffer->userParam = c->tag;
            lowLevelVoice->AppendWaveBuffer(waveBuffer);
            break;
        }
        case VoiceCommandId_Param: {
            auto* c{static_cast<VoiceCommandParam*>(command)};
            LowLevelVoice* lowLevelVoice{
                VirtualVoiceManager::GetInstance().GetLowLevelVoice(c->voiceId),
            };
            if (lowLevelVoice == nullptr)
                break;

            lowLevelVoice->SetVoiceParam(c->voiceParam);
            break;
        }
        default:
            break;
        }
        command = command->next;
    }
}

VoiceReplyCommand& VoiceReplyCommand::GetInstance() {
    static VoiceReplyCommand instance;
    return instance;
}

void VoiceReplyCommand::ProcessCommandList(Command* commandList) {
    for (Command* command{commandList}; command != nullptr; command = command->next) {
    }
}

}  // namespace nn::atk::detail
