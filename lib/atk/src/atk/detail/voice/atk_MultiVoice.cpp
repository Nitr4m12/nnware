#include <nn/atk/atk_MultiVoice.h>

namespace nn::atk::detail::driver {

namespace {

size_t DspAdpcmFrameToNibbleAddress(position_t frame, size_t nibbleAddress) {}

void PanCurveToPanInfo(Util::PanInfo& panInfo, PanCurve curve) {}

void CalcBarancePanForStereo(float* left, float* right, const float& pan, int32_t channelIndex,
                             const Util::PanInfo& panInfo, OutputMode mode) {}

void CalcSurroundPanForSurround(float* front, float* rear, const float& span,
                                const Util::PanInfo& panInfo) {}

void CalcPanForMono(float* left, float* right, const Util::PanInfo& panInfo) {}

void CalcDualPanForStereo(float* left, float* right, const float& pan, const Util::PanInfo& panInfo,
                          OutputMode mode) {}

void CalcSurroundPanForMono(float* front, float* rear, const Util::PanInfo& panInfo) {}

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
int32_t GetOutputReceiverMixBufferIndex(const OutputReceiver* pOutputReceiver, int32_t channel,
                                        int32_t bus) {}
#endif

}  // anonymous namespace

const float MultiVoice::VolumeMin{0.0};
const float MultiVoice::VolumeDefault{1.0};
const float MultiVoice::VolumeMax{2.0};

const float MultiVoice::PanLeft{-1.0};
const float MultiVoice::PanCenter{-0.0};
const float MultiVoice::PanRight{1.0};

const float MultiVoice::SpanFront{0.0};
const float MultiVoice::SpanCenter{1.0};
const float MultiVoice::SpanRear{2.0};

const float MultiVoice::CutoffFreqMin{0.0};
const float MultiVoice::CutoffFreqMax{1.0};

const float MultiVoice::BiquadValueMin{0.0};
const float MultiVoice::BiquadValueMax{1.0};

const float MultiVoice::SendMin{0.0};
const float MultiVoice::SendMax{1.0};

MultiVoice::MultiVoice() = default;

#if NN_WARE_VER >= NN_MAKE_VER(4, 0, 0)
MultiVoice::MultiVoice(OutputAdditionalParam* pAdditionalParam)
    : m_pTvAdditionalParam(pAdditionalParam) {}
#endif

MultiVoice::~MultiVoice() {
    for (int channelIndex{0}; channelIndex < m_ChannelCount; ++channelIndex)
        m_Voice[channelIndex].Free();
}

}  // namespace nn::atk::detail::driver
