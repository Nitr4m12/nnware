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

void DeviceOutRecorder::RecorderBuffer::Clear() {
    m_ValidSamples = 0;
    m_ReadPosition = 0;
    m_WritePosition = 0;
}

}  // namespace nn::atk
