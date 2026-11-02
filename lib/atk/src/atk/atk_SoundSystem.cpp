#include <nn/atk/atk_SoundSystem.h>

NN_MIDDLEWARE(g_MiddlewareInfo, "Nintendo", "NintendoWare_Atk" NN_SDK_BUILD_STR);

namespace {

size_t GetRequiredDriverCommandManagerMemSize(const nn::atk::SoundSystem::SoundSystemParam& param) {
    // TODO
}

void SetupHardwareManagerParameter(
    nn::atk::detail::driver::HardwareManager::HardwareManagerParameter* pOutValue,
    const nn::atk::SoundSystem::SoundSystemParam& parameter, int32_t subMixTotalChannelCount) {
    // TODO
}

size_t GetRequiredSoundSystemMemSize(const nn::atk::SoundSystem::SoundSystemParam& param,
                                     const nn::atk::detail::SoundInstanceConfig& config) {
    // TODO
}

int32_t GetChannelCount(int32_t voiceCount) {
    // TODO
}

size_t
GetRequiredSoundSystemMemSizeForMemoryPool(const nn::atk::SoundSystem::SoundSystemParam& param,
                                           const nn::atk::detail::SoundInstanceConfig& config) {
    // TODO
}

const int32_t AdditionalSubMixBusIndex{0};

int32_t ClampBusCountMax(int32_t busCountMax) {
    // TODO
}

int32_t ConvertAuxBusIndexToSubMixBusIndex(nn::atk::AuxBus bus) {
    // TODO
}

}  // anonymous namespace

namespace nn::atk {

bool SoundSystem::g_IsStreamOpenFailureHalt{true};
bool SoundSystem::g_IsTaskThreadEnabled{true};
bool SoundSystem::g_IsManagingMemoryPool{true};

int32_t SoundSystem::g_VoiceCountMax{96};
int32_t SoundSystem::g_UserEffectCount{10};

bool SoundSystem::g_IsSubMixEnabled{true};
bool SoundSystem::g_IsPresetSubMixEnabled{true};

bool SoundSystem::g_IsStereoModeEnabled{true};
bool SoundSystem::g_IsSoundThreadEnabled{true};

int32_t SoundSystem::g_BusCountMax{detail::DefaultBusCount};

SoundSystem::SoundSystemParam::SoundSystemParam() = default;

}  // namespace nn::atk
