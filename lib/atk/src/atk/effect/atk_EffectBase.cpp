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

}  // namespace nn::atk
