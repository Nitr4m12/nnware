#include <nn/atk/atk_OutputAdditionalParam.h>

namespace {

int32_t GetAdditionalBusCount(int32_t busCount) {
    return busCount - 4;
}

}  // anonymous namespace

namespace nn::atk::detail {

size_t OutputAdditionalParam::GetRequiredMemSize(const SoundInstanceConfig& config) {
    size_t result{0};

    if (config.busCount > 4)
        result = SendArray::GetRequiredMemSize(GetAdditionalBusCount(config.busCount)) + 16;

    if (config.isBusMixVolumeEnabled) {
        result += BusMixVolumePacket::GetRequiredMemSize(config.busCount);
        result += sizeof(BusMixVolumePacket);
    }

    if (config.isVolumeThroughModeEnabled) {
        result += VolumeThroughModePacket::GetRequiredMemSize(config.busCount);
        result += sizeof(VolumeThroughModePacket);
    }

    return result;
}

void OutputAdditionalParam::Initialize(void* buffer, [[maybe_unused]] size_t bufferSize,
                                       const SoundInstanceConfig& config) {
    uint8_t* ptr{static_cast<uint8_t*>(buffer)};

    if (config.busCount > 4) {
        m_pAdditionalSend = reinterpret_cast<SendArray*>(ptr);
        new (m_pAdditionalSend) SendArray;
        ptr += sizeof(SendArray);

        const int AdditionalSendCount{GetAdditionalBusCount(config.busCount)};
        const size_t BufferSize{SendArray::GetRequiredMemSize(AdditionalSendCount)};
        m_pAdditionalSend->Initialize(ptr, BufferSize, AdditionalSendCount);

        ptr += BufferSize;
    }

    if (config.isBusMixVolumeEnabled) {
        m_pBusMixVolumePacket = reinterpret_cast<BusMixVolumePacket*>(ptr);
        new (m_pBusMixVolumePacket) BusMixVolumePacket;
        ptr += sizeof(BusMixVolumePacket);

        const size_t BufferSize{BusMixVolumePacket::GetRequiredMemSize(config.busCount)};
        m_pBusMixVolumePacket->Initialize(ptr, BufferSize, config.busCount);

        ptr += BufferSize;
    }

    if (config.isVolumeThroughModeEnabled) {
        m_pVolumeThroughModePacket = reinterpret_cast<VolumeThroughModePacket*>(ptr);
        new (m_pVolumeThroughModePacket) VolumeThroughModePacket;
        ptr += sizeof(VolumeThroughModePacket);

        const size_t BufferSize{VolumeThroughModePacket::GetRequiredMemSize(config.busCount)};
        m_pVolumeThroughModePacket->Initialize(ptr, BufferSize, config.busCount);
    }
}

}  // namespace nn::atk::detail
