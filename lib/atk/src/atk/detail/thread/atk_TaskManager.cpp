#include <nn/atk/atk_TaskManager.h>

namespace nn::atk::detail {

TaskManager& TaskManager::GetInstance() {
    static TaskManager instance;
    return instance;
}

TaskManager::TaskManager() = default;

void TaskManager::Initialize(bool isEnableProfiling) {
    m_TaskProfileLogger.SetProfilingEnabled(isEnableProfiling);
}

}  // namespace nn::atk::detail
