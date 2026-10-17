#include <nn/atk/atk_TaskManager.h>

namespace nn::atk::detail {

TaskManager& TaskManager::GetInstance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() = default;

}  // namespace nn::atk::detail
