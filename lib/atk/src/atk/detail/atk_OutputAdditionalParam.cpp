#include <nn/atk/atk_OutputAdditionalParam.h>

#include <nn/atk/atk_Util.h>

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

void OutputAdditionalParam::Finalize() {
    if (m_pAdditionalSend != nullptr) {
        m_pAdditionalSend->Finalize();
        m_pAdditionalSend = nullptr;
    }

    if (m_pBusMixVolumePacket != nullptr)
        m_pBusMixVolumePacket = nullptr;

    if (m_pVolumeThroughModePacket != nullptr) {
        m_pVolumeThroughModePacket->Finalize();
        m_pVolumeThroughModePacket = nullptr;
    }
}

void OutputAdditionalParam::Reset() {
    if (m_pAdditionalSend != nullptr)
        m_pAdditionalSend->Reset();

    if (m_pBusMixVolumePacket != nullptr)
        m_pBusMixVolumePacket->Reset();

    if (m_pVolumeThroughModePacket)
        m_pVolumeThroughModePacket->Reset();
}

void* OutputAdditionalParam::GetBufferAddr() {
    if (m_pAdditionalSend != nullptr)
        return m_pAdditionalSend;

    if (m_pBusMixVolumePacket != nullptr)
        return m_pBusMixVolumePacket;

    return m_pVolumeThroughModePacket;
}

SendArray* OutputAdditionalParam::GetAdditionalSendAddr() {
    return m_pAdditionalSend;
}

const SendArray* const OutputAdditionalParam::GetAdditionalSendAddr() const {
    return m_pAdditionalSend;
}

float OutputAdditionalParam::TryGetAdditionalSend(int32_t bus) const {
    if (bus < 4)
        return 0.0f;

    return m_pAdditionalSend->TryGetValue(Util::GetAdditionalSendIndex(bus));
}

bool OutputAdditionalParam::IsAdditionalSendEnabled() const {
    return m_pAdditionalSend != nullptr;
}

void OutputAdditionalParam::TrySetAdditionalSend(int32_t bus, float send) {
    if (bus < 4)
        return;

    m_pAdditionalSend->TrySetValue(Util::GetAdditionalSendIndex(bus), send);
}

BusMixVolumePacket* OutputAdditionalParam::GetBusMixVolumePacketAddr() {
    return m_pBusMixVolumePacket;
}

const BusMixVolumePacket* const OutputAdditionalParam::GetBusMixVolumePacketAddr() const {
    return m_pBusMixVolumePacket;
}

}  // namespace nn::atk::detail
