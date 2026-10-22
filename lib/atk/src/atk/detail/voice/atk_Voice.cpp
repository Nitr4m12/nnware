#include <nn/atk/atk_Voice.h>

namespace nn::atk::detail {

namespace {

const BiquadFilterCoefficients BiquadFilterCoefZero{.b0=0, .b1=0, .b2=0, .a1=0, .a2=0};

const OutputMix DefaultTvMix{
    {
        1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    },
};

}

Voice::Voice() = default;

Voice::~Voice() = default;

}  // namespace nn::atk::detail
