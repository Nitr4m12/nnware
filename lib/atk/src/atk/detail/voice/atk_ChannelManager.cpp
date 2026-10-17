#include <nn/atk/atk_ChannelManager.h>

namespace nn::atk::detail::driver {

ChannelManager& ChannelManager::GetInstance() {
    static ChannelManager instance;
    return instance;
}

ChannelManager::ChannelManager() = default;

}  // namespace nn::atk::detail::driver
