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
    SendArray* GetAdditionalSendAddr() const;

    float TryGetAdditionalSend(int32_t bus) const;

    bool IsAdditionalSendEnabled() const;

    void TrySetAdditionalSend(int32_t bus, float send);

    BusMixVolumePacket* GetBusMixVolumePacketAddr();
    BusMixVolumePacket* GetBusMixVolumePacketAddr() const;

    float GetBusMixVolume(int32_t waveChannel, int32_t mixChannel) const;
    OutputBusMixVolume* GetBusMixVolume() const;

    void SetBusMixVolume(int32_t waveChannel, int32_t mixChannel, float volume);
    void SetBusMixVolume(const OutputBusMixVolume& param);

    bool IsBusMixVolumeUsed() const;
    void SetBusMixVolumeUsed(bool isUsed);

    bool IsBusMixVolumeEnabledForBus(int32_t bus) const;
    void SetBusMixVolumeEnabledForBus(int32_t bus, bool isEnabled);

    bool IsBusMixVolumeEnabled() const;

    VolumeThroughModePacket* GetVolumeThroughModePacketAddr();
    VolumeThroughModePacket* GetVolumeThroughModePacketAddr() const;

    float GetBinaryVolume() const;
    void SetBinaryVolume(float volume);

    uint8_t TryGetVolumeThroughMode(int32_t bus) const;
    void TrySetVolumeThroughMode(int32_t bus, uint8_t volumeThroughMode);
    bool IsVolumeThroughModeEnabled() const;

    bool IsVolumeThroughModeUsed();
    void SetVolumeThroughModeUsed(bool isUsed);

    OutputAdditionalParam& operator=(const OutputAdditionalParam& rhs);

private:
    SendArray* m_pAdditionalSend{};
    BusMixVolumePacket* m_pBusMixVolumePacket{};
    VolumeThroughModePacket* VolumeThroughModePacket{};
};
static_assert(sizeof(OutputAdditionalParam) == 0x18);

}  // namespace nn::atk::detail
