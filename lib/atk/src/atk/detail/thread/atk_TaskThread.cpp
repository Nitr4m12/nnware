#include <nn/atk/atk_TaskThread.h>

namespace nn::atk::detail {

TaskThread& TaskThread::GetInstance() {
    static TaskThread instance;
    return instance;
}

TaskThread::TaskThread() = default;

}  // namespace nn::atk::detail
