#include <nn/atk/atk_DspadpcmReader.h>

#include <cstring>

#include <nn/util/util_BytePtr.h>

namespace nn::atk::detail {
namespace {

struct InternalDSPADPCMInfo {
    uint32_t sampleCount;
    uint32_t adpcmNibbleCount;
    uint32_t sampleRate;
    uint16_t loopFlag;
    uint16_t format;
    uint32_t sa;
    uint32_t ea;
    uint32_t ca;
    uint16_t coef[16];
    uint16_t gain;
    uint16_t ps;
    uint16_t yn1;
    uint16_t yn2;
    uint16_t lps;
    uint16_t lyn1;
    uint16_t lyn2;
    uint16_t pad[11];
};
static_assert(sizeof(InternalDSPADPCMInfo) == 0x60);

}  // anonymous namespace

DspadpcmReader::DspadpcmReader() = default;

bool DspadpcmReader::ReadWaveInfo(WaveInfo* info) const {
    const InternalDSPADPCMInfo& data{
        *util::ConstBytePtr(m_pDspadpcmData).Get<InternalDSPADPCMInfo>()};

    info->sampleFormat = SampleFormat_DspAdpcm;
    info->loopFlag = false;
    info->channelCount = 1;
    info->sampleRate = data.sampleRate;
    info->loopStartFrame = 0;
    info->loopEndFrame = data.sampleCount;

    info->channelParam[0].dataAddress =
        util::ConstBytePtr(m_pDspadpcmData, sizeof(InternalDSPADPCMInfo)).Get();
    std::memcpy(info->channelParam[0].adpcmParam.coef, data.coef, sizeof(data.coef));
    int yn1 = data.yn1;
    info->channelParam[0].adpcmParam.predScale = data.ps;
    info->channelParam[0].adpcmParam.yn1 = yn1;
    info->channelParam[0].adpcmParam.yn2 = data.yn2;
    return true;
}

}  // namespace nn::atk::detail
