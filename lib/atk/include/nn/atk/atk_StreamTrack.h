#pragma once

#include <nn/os.h>

#include <nn/atk/atk_MultiVoice.h>
#include <nn/atk/atk_ProfileReader.h>

namespace nn::atk::detail::driver {

struct StreamChannel {
    void AppendWaveBuffer(WaveBuffer* pBuffer, bool lastFlag);
    os::Tick GetProcessTick(const SoundProfile& profile);

    void* m_pBufferAddress;
    MultiVoice* m_pVoice;
    WaveBuffer m_WaveBuffer[StreamDataLoadTaskMax];
    AdpcmContext m_AdpcmContext[StreamDataLoadTaskMax];
    UpdateType m_UpdateType;
};
static_assert(sizeof(StreamChannel) == 0x1080);

struct StreamTrack {
    bool m_ActiveFlag;
    StreamChannel* m_pChannels[2];
    uint8_t channelCount;
    uint8_t volume;
    uint8_t pan;
    uint8_t span;
    uint8_t mainSend;
    uint8_t fxSend[3];
    uint8_t lpfFreq;
    int8_t biquadType;
    uint8_t biquadValue;
    uint8_t flags;
    float m_Volume;
    int32_t m_OutputLine;
    OutputParam m_TvParam;

    StreamTrack() = default;
    ~StreamTrack() = default;
};
static_assert(sizeof(StreamTrack) == 0x80);

}  // namespace nn::atk::detail::driver
