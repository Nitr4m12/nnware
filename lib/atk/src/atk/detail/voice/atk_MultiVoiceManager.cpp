#include <nn/atk/atk_MultiVoiceManager.h>

namespace nn::atk::detail::driver {

MultiVoiceManager& MultiVoiceManager::GetInstance() {
    static MultiVoiceManager instance;
    return instance;
}

MultiVoiceManager::MultiVoiceManager() = default;

}  // namespace nn::atk::detail::driver
