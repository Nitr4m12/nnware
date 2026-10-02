#include <nn/atk/atk_ProfileReader.h>

namespace nn::atk {

ProfileReader::ProfileReader() = default;

int32_t ProfileReader::Read(SoundProfile* profile, int32_t maxCount) {
    int count{0};

    while (m_ProfileBufferRead != m_ProfileBufferWrite && count < maxCount) {
        profile[count] = m_ProfileBuffer[m_ProfileBufferRead];

        if (m_ProfileBufferRead >= StreamDataLoadTaskMax - 1)
            m_ProfileBufferRead = 0;
        else
            ++m_ProfileBufferRead;
        ++count;
    }

    return count;
}

}  // namespace nn::atk
