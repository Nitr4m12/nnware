#include <nn/atk/atk_BusMixVolumePacket.h>

#include <nn/util/util_BitUtil.h>

namespace nn::atk::detail {

size_t BusMixVolumePacket::GetRequiredMemSize(int busCount) {
    size_t result{util::align_up<uint64_t>(busCount, 8)};
    return result;
}

bool BusMixVolumePacket::Initialize(void* buffer, [[maybe_unused]] size_t size, int busCount) {
    m_pIsEnabledTable = static_cast<bool*>(buffer);
    for (int i{0}; i < busCount; ++i)
        m_pIsEnabledTable[i] = false;

    m_BusCount = busCount;
    return true;
}

void BusMixVolumePacket::Finalize() {
    m_pIsEnabledTable = nullptr;
    m_IsUsed = false;
    m_BusCount = 0;
}

}  // namespace nn::atk::detail
