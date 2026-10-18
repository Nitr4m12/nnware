#include <nn/atk/atk_DeviceOutRecorder.h>

namespace nn::atk {

DeviceOutRecorder::RecorderBuffer::RecorderBuffer(const char* deviceName) {
    m_DeviceName = deviceName == nullptr ? "" : deviceName;
}

void DeviceOutRecorder::RecorderBuffer::UpdateMaxSamples() {
    if (m_ReadBlockSamples != 0)
        m_MaxSamples = (m_MaxBufferSamples / m_ReadBlockSamples) * m_ReadBlockSamples;
    else
        m_MaxSamples = m_MaxBufferSamples;
}

}  // namespace nn::atk
