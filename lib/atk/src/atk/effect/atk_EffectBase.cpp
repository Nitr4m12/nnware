#include <nn/atk/atk_EffectBase.h>

namespace nn::atk {

EffectBase::EffectBase() = default;

EffectBase::~EffectBase() = default;

#if NN_WARE_VER < NN_MAKE_VER(4, 0, 0)
bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] audio::FinalMixType* pFinalMix) {
    return true;
}

bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] audio::SubMixType* pSubMix) {
    return true;
}
#else
bool EffectBase::AddEffect([[maybe_unused]] audio::AudioRendererConfig* pConfig,
                           [[maybe_unused]] OutputMixer* pOutputMixer) {
    return true;
}
#endif

void EffectBase::SetEffectInputOutput([[maybe_unused]] const int8_t* input,
                                      [[maybe_unused]] const int8_t* output,
                                      [[maybe_unused]] int inputCount,
                                      [[maybe_unused]] int outputCount) {}

}  // namespace nn::atk
