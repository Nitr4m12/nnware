#pragma once

#include <cstddef>
#include <cstdint>

#include <nn/atk/atk_BusMixVolumePacket.h>
#include <nn/atk/atk_ValueArray.h>
#include <nn/atk/atk_VolumeThroughModePacket.h>

namespace nn::atk::detail {

using SendArray = ValueArray<float>;

class OutputAdditionalParam {
public:
    OutputAdditionalParam() = default;

    static size_t GetRequiredMemSize(const SoundInstanceConfig& config);

    void Initialize(void* buffer, size_t bufferSize, const SoundInstanceConfig& config);
    void Finalize();

    void Reset();

    void* GetBufferAddr();

    SendArray* GetAdditionalSendAddr();
    const SendArray* const GetAdditionalSendAddr() const;

    bool IsAdditionalSendEnabled() const;

    float TryGetAdditionalSend(int32_t bus) const;
    void TrySetAdditionalSend(int32_t bus, float send);

    BusMixVolumePacket* GetBusMixVolumePacketAddr();
    const BusMixVolumePacket* const GetBusMixVolumePacketAddr() const;

    bool IsBusMixVolumeEnabled() const;

    float GetBusMixVolume(int32_t waveChannel, int32_t mixChannel) const;
    const OutputBusMixVolume& GetBusMixVolume() const;

    void SetBusMixVolume(int32_t waveChannel, int32_t mixChannel, float volume);
    void SetBusMixVolume(const OutputBusMixVolume& param);

    bool IsBusMixVolumeUsed() const;
    void SetBusMixVolumeUsed(bool isUsed);

    bool IsBusMixVolumeEnabledForBus(int32_t bus) const;
    void SetBusMixVolumeEnabledForBus(int32_t bus, bool isEnabled);

    VolumeThroughModePacket* GetVolumeThroughModePacketAddr();
    const VolumeThroughModePacket* const GetVolumeThroughModePacketAddr() const;

    bool IsVolumeThroughModeEnabled() const;

    float GetBinaryVolume() const;
    void SetBinaryVolume(float volume);

    uint8_t TryGetVolumeThroughMode(int32_t bus) const;
    void TrySetVolumeThroughMode(int32_t bus, uint8_t mode);

    bool IsVolumeThroughModeUsed() const;
    void SetVolumeThroughModeUsed(bool isUsed);

    OutputAdditionalParam& operator=(const OutputAdditionalParam& rhs);

private:
    SendArray* m_pAdditionalSend{};
    BusMixVolumePacket* m_pBusMixVolumePacket{};
    VolumeThroughModePacket* m_pVolumeThroughModePacket{};
};
static_assert(sizeof(OutputAdditionalParam) == 0x18);

}  // namespace nn::atk::detail
