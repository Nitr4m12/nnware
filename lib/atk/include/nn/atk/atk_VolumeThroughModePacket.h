#pragma once

#include <cstdint>

#include <nn/atk/atk_ValueArray.h>

namespace nn::atk::detail {

class VolumeThroughModePacket {
public:
    using VolumeThroughModeArray = ValueArray<uint8_t>;

    VolumeThroughModePacket() = default;

    static size_t GetRequiredMemSize(int busCount);

    bool Initialize(void* buffer, size_t size, int busCount);
    void Finalize();
    void Reset();

    int GetBusCount() const { return m_VolumeThroughMode.GetCount(); }

    uint8_t GetVolumeThroughMode(int bus) const { return m_VolumeThroughMode.GetValue(bus); }

    void GetVolumeThroughMode(int bus, uint8_t volumeThroughMode) {
        m_VolumeThroughMode.SetValue(bus, volumeThroughMode);
    }

    uint8_t TryGetVolumeThroughMode(int bus) const { return m_VolumeThroughMode.TryGetValue(bus); }

    bool TrySetVolumeThroughMode(int bus, uint8_t volumeThroughMode) {
        return m_VolumeThroughMode.TrySetValue(bus, volumeThroughMode);
    }

    bool IsVolumeThroughModeUsed() const { return m_IsVolumeThroughModeUsed; }

    void SetVolumeThroughModeUsed(bool isUsed) { m_IsVolumeThroughModeUsed = isUsed; }

    float GetBinaryVolume() const { return m_BinaryVolume; }

    void SetBinaryVolume(float volume) { m_BinaryVolume = volume; }

    VolumeThroughModePacket& operator=(const VolumeThroughModePacket& rhs) {
        m_VolumeThroughMode = rhs.m_VolumeThroughMode;
        m_IsVolumeThroughModeUsed = rhs.m_IsVolumeThroughModeUsed;
        m_BinaryVolume = rhs.m_BinaryVolume;
        return *this;
    }

private:
    VolumeThroughModeArray m_VolumeThroughMode;
    bool m_IsVolumeThroughModeUsed{false};
    float m_BinaryVolume{1.0f};
};
static_assert(sizeof(VolumeThroughModePacket) == 0x18);

}  // namespace nn::atk::detail
